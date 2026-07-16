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

#include "TelemetryRuntime.hpp"

#include <utility>

namespace flux::telemetry {

TelemetryRuntime::TelemetryRuntime(
    std::string directory, std::string filename, Clock clock, WakeCallback on_update,
    FreshnessConfig freshness_config
)
    : ingestor_(store_, freshness_config), clock_(std::move(clock)),
      on_update_(std::move(on_update)),
      watcher_(
          std::move(directory), std::move(filename),
          [this](std::optional<std::string> content, bool uncertain) {
              on_delivery(std::move(content), uncertain);
          }
      ) {}

void TelemetryRuntime::on_delivery(std::optional<std::string> content, bool uncertain) {
    {
        std::lock_guard<std::mutex> lk(ingest_mtx_);
        ingestor_.ingest(std::move(content), uncertain, clock_());
    }
    // Wake outside the lock: the daemon's callback may re-enter and read the store, and it
    // must never be able to block the watcher thread behind our own ingest lock.
    if (on_update_) on_update_();
}

bool TelemetryRuntime::start() { return watcher_.start(); }

void TelemetryRuntime::stop() { watcher_.stop(); }

void TelemetryRuntime::tick() {
    std::lock_guard<std::mutex> lk(ingest_mtx_);
    ingestor_.tick(clock_());
}

} // namespace flux::telemetry
