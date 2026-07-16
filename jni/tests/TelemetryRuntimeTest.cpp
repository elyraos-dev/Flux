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

// Host tests for TelemetryRuntime: the composition root that makes the V2 pipeline the single
// live telemetry authority. These drive the REAL kernel inotify against a temp directory (the
// atomic-rename path is a kernel behaviour and is worth testing for real), with an injected
// clock so freshness is deterministic. No root required.

#include "TestFramework.hpp"

#include "TelemetryRuntime.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>

#include <unistd.h>

using namespace flux::telemetry;

namespace {

// A temporary directory that cleans itself up.
class TempDir {
public:
    TempDir() {
        char tmpl[] = "/tmp/flux_rt_XXXXXX";
        const char *d = ::mkdtemp(tmpl);
        path_ = (d != nullptr) ? d : "";
    }
    ~TempDir() {
        if (!path_.empty()) {
            const std::string cmd = "rm -rf '" + path_ + "'";
            const int rc = std::system(cmd.c_str());
            (void)rc;
        }
    }
    [[nodiscard]] const std::string &path() const { return path_; }

private:
    std::string path_;
};

std::string valid_snapshot(uint64_t sequence, const std::string &pkg = "com.example.game", int zen = 0) {
    return "schema_version 2\n"
           "sequence " + std::to_string(sequence) + "\n"
           "updated_elapsed_ms 1000\n"
           "daemon_pid 123\n"
           "foreground_available 1\n"
           "focused_package " + pkg + "\n"
           "focused_pid 4242\n"
           "focused_uid 10123\n"
           "screen_available 1\n"
           "screen_awake 1\n"
           "power_available 1\n"
           "battery_saver 0\n"
           "charging_available 1\n"
           "charging_state 0\n"
           "thermal_available 1\n"
           "thermal_valid 1\n"
           "thermal_headroom 0.40\n"
           "thermal_status 0\n"
           "thermal_sample_elapsed_ms 1000\n"
           "thermal_age_ms 0\n"
           "audio_available 1\n"
           "audio_active 0\n"
           "zen_available 1\n"
           "zen_mode " + std::to_string(zen) + "\n"
           "kernel_is_gki 1\n";
}

// Replace the target the way SynthesisCore does: write a temp file, then rename() over the
// target. A rename delivers IN_MOVED_TO on the destination name and never IN_CLOSE_WRITE.
void atomically_replace(const std::string &dir, const std::string &name, const std::string &content) {
    const std::string tmp = dir + "/." + name + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out << content;
    }
    ::rename(tmp.c_str(), (dir + "/" + name).c_str());
}

// Wait until `pred` holds or the budget runs out. Keeps the tests fast when they pass and
// bounded when they fail, without sleeping a fixed amount every run.
template <typename Pred> bool wait_until(Pred pred, int budget_ms = 3000) {
    for (int waited = 0; waited < budget_ms; waited += 10) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return pred();
}

} // namespace

// --- Startup ---------------------------------------------------------------

TEST("runtime: an already-present status file is reconciled at start") {
    TempDir dir;
    CHECK(!dir.path().empty());
    atomically_replace(dir.path(), "status", valid_snapshot(1));

    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK_MSG(rt.start(), "start must succeed: " + rt.last_error());

    const auto published = rt.get();
    CHECK_MSG(published.has_value(), "an existing file must be picked up by the initial reconcile");
    CHECK_EQ(published->snapshot.sequence, 1u);
    CHECK_EQ(published->snapshot.focused_package, std::string("com.example.game"));
    rt.stop();
}

TEST("runtime: no status file at startup leaves telemetry unavailable, then it appears") {
    TempDir dir;
    int64_t now = 0;
    std::atomic<int> wakes{0};
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, [&wakes] { ++wakes; });
    CHECK(rt.start());

    // Nothing published yet: the daemon must treat this as unavailable, not as zeroes.
    CHECK_MSG(!rt.get().has_value(), "a missing file must not publish a snapshot");

    atomically_replace(dir.path(), "status", valid_snapshot(1));
    CHECK_MSG(wait_until([&] { return rt.get().has_value(); }), "a later-appearing file must be picked up");
    CHECK_EQ(rt.get()->snapshot.sequence, 1u);
    CHECK(wakes.load() > 0);
    rt.stop();
}

// --- Atomic update ---------------------------------------------------------

TEST("runtime: an atomic rename is delivered and becomes the live snapshot") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));

    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());
    CHECK_EQ(rt.get()->snapshot.sequence, 1u);

    now = 100;
    atomically_replace(dir.path(), "status", valid_snapshot(2));
    CHECK(wait_until([&] { return rt.get()->snapshot.sequence == 2u; }));
    rt.stop();
}

TEST("runtime: rapid replacements end on the newest snapshot") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));

    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());

    for (uint64_t seq = 2; seq <= 12; ++seq) {
        now += 10;
        atomically_replace(dir.path(), "status", valid_snapshot(seq));
    }
    // Events may coalesce, but the final state must be the newest content, never an older one.
    CHECK(wait_until([&] { return rt.get()->snapshot.sequence == 12u; }));
    rt.stop();
}

// --- Malformed input must not destroy the last good snapshot ---------------

TEST("runtime: a malformed replacement never clobbers the last complete snapshot") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(7, "com.good.game", 3));

    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());
    CHECK_EQ(rt.get()->snapshot.sequence, 7u);

    now = 50;
    atomically_replace(dir.path(), "status", "schema_version 2\nsequence NOT_A_NUMBER\n");

    // Give the watcher a chance to deliver the bad content, then prove the good one survived
    // intact — including the exact zen mode, which must not collapse to a boolean.
    (void)wait_until([&] { return false; }, 150);
    const auto published = rt.get();
    CHECK_MSG(published.has_value(), "the previous snapshot must survive a malformed update");
    CHECK_MSG(published->snapshot.sequence == 7u, "no partially-parsed fields may be published");
    CHECK_EQ(published->snapshot.focused_package, std::string("com.good.game"));
    CHECK_MSG(published->snapshot.zen_mode == 3, "exact integer zen mode must be preserved");
    rt.stop();
}

TEST("runtime: a valid update after a malformed one is accepted") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));
    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());

    atomically_replace(dir.path(), "status", "garbage\n");
    (void)wait_until([&] { return false; }, 100);

    now = 60;
    atomically_replace(dir.path(), "status", valid_snapshot(2));
    CHECK(wait_until([&] { return rt.get()->snapshot.sequence == 2u; }));
    rt.stop();
}

// --- Deletion / recreation -------------------------------------------------

TEST("runtime: target deletion keeps the last snapshot, recreation is observed") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));
    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());

    ::unlink((dir.path() + "/status").c_str());
    (void)wait_until([&] { return false; }, 100);
    // A deleted producer file is not a reason to forget what we last knew.
    CHECK_MSG(rt.get().has_value(), "deletion must not erase the last good snapshot");

    now = 100;
    atomically_replace(dir.path(), "status", valid_snapshot(5));
    CHECK(wait_until([&] { return rt.get()->snapshot.sequence == 5u; }));
    rt.stop();
}

// --- Freshness -------------------------------------------------------------

TEST("runtime: health ages to stale then unavailable when the producer goes quiet") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));
    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());
    CHECK(rt.get().has_value());

    // No new snapshot arrives; only time passes. The tick is what must surface that.
    now = 6000;
    rt.tick();
    CHECK_MSG(rt.get()->health == TelemetryHealth::Stale, "5s+ without a sample must read Stale");

    now = 20000;
    rt.tick();
    CHECK_MSG(rt.get()->health == TelemetryHealth::Unavailable, "15s+ must read Unavailable");

    // The snapshot itself is retained — only the health degrades.
    CHECK_EQ(rt.get()->snapshot.sequence, 1u);
    rt.stop();
}

// --- Lifecycle -------------------------------------------------------------

TEST("runtime: stop is deterministic while replacements are in flight") {
    TempDir dir;
    atomically_replace(dir.path(), "status", valid_snapshot(1));
    int64_t now = 0;
    TelemetryRuntime rt(dir.path(), "status", [&now] { return now; }, nullptr);
    CHECK(rt.start());

    std::atomic<bool> writing{true};
    std::thread writer([&] {
        for (uint64_t seq = 2; writing.load() && seq < 200; ++seq) {
            atomically_replace(dir.path(), "status", valid_snapshot(seq));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    rt.stop();  // must join the worker before closing descriptors; ASan/TSan would catch a race
    writing.store(false);
    writer.join();

    rt.stop();  // idempotent
}
