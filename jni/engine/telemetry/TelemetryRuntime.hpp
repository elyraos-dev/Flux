/*
 * Copyright (C) 2026 FebriCahyaa
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include "AtomicStatusWatcher.hpp"
#include "TelemetryIngestor.hpp"
#include "TelemetryStore.hpp"

namespace flux::telemetry {

/**
 * @brief The live telemetry authority: watcher -> ingestor -> decoder -> freshness -> store.
 *
 * This is the composition root that makes the V2 pipeline the *only* source of telemetry in
 * the running daemon. It owns the pieces and their lifetimes, and it is the single object the
 * daemon holds; readers ask it for a published snapshot and never touch a global cache.
 *
 * Threading: AtomicStatusWatcher delivers on its own worker thread, while the daemon calls
 * tick() from the main loop. TelemetryIngestor is explicitly not thread-safe, so both paths
 * are serialised here under one mutex. The store itself is thread-safe for readers, so get()
 * takes no lock of ours.
 *
 * Startup ordering is delegated to AtomicStatusWatcher, which registers the directory watch
 * *before* the first read: an update that lands during startup is therefore either seen by
 * that read or still queued in the kernel for the worker to drain. It cannot fall through the
 * gap between the two.
 *
 * The clock is injected so freshness can be tested deterministically; it must be monotonic.
 */
class TelemetryRuntime {
public:
    using Clock = std::function<int64_t()>;
    /// Invoked after each delivery so the daemon can re-evaluate immediately instead of
    /// waiting for its next timeout. Never called while the ingest lock is held.
    using WakeCallback = std::function<void()>;

    TelemetryRuntime(
        std::string directory, std::string filename, Clock clock, WakeCallback on_update,
        FreshnessConfig freshness_config = {}
    );

    /** Start watching. Returns false if the watch could not be established. */
    bool start();

    /** Stop the watcher and join its thread. Safe to call twice. */
    void stop();

    /**
     * @brief Age the published health as time passes without a new snapshot.
     *
     * A dead producer emits no events, so nothing would otherwise move telemetry from Healthy
     * to Stale/Unavailable. The daemon's periodic tick is what makes that transition happen.
     */
    void tick();

    /** The current published telemetry, or nullopt if nothing valid has ever been published. */
    [[nodiscard]] std::optional<PublishedTelemetry> get() const { return store_.get(); }

    /// Returned by value: the watcher hands back a copy, so a reference here would dangle.
    [[nodiscard]] std::string last_error() const { return watcher_.last_error(); }

private:
    void on_delivery(std::optional<std::string> content, bool uncertain);

    // Declaration order is the construction order and matters: the ingestor refers to the
    // store, and the watcher's callback refers to this object, so the watcher is destroyed
    // (and its thread joined) first.
    TelemetryStore store_;
    std::mutex ingest_mtx_;
    TelemetryIngestor ingestor_;
    Clock clock_;
    WakeCallback on_update_;
    AtomicStatusWatcher watcher_;
};

} // namespace flux::telemetry
