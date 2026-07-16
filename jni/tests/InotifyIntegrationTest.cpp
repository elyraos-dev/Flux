/*
 * Copyright (C) 2024-2026 FebriCahyaa
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

// Host tests for the inotify event predicates.
//
// InotifyWatcher still backs the JSON config watches (gamelist, config, device mitigation), so
// its event classification stays covered here.
//
// The telemetry cases this file used to carry — atomic rename delivery, queue overflow,
// malformed-update-retains-last-good, producer restart, clean shutdown under load — were not
// dropped: they moved to TelemetryRuntimeTest and the AtomicStatusWatcher tests, which exercise
// the V2 watcher that actually owns the status file. Nothing here parses telemetry any more, and
// the legacy reader it used to drive has been deleted.

#include "TestFramework.hpp"

#include <InotifyEvents.hpp>

TEST("inotify: IN_MOVED_TO counts as a content update") {
    // The important one: SynthesisCore replaces the status file with rename(), and a rename
    // never delivers IN_CLOSE_WRITE on the destination name — only IN_MOVED_TO.
    CHECK_MSG(is_content_update(IN_MOVED_TO), "an atomic rename must be seen as new content");
    CHECK(is_content_update(IN_CLOSE_WRITE));
    CHECK(is_content_update(IN_CLOSE_WRITE | IN_MOVED_TO));

    // Things that are not new content.
    CHECK(!is_content_update(IN_OPEN));
    CHECK(!is_content_update(IN_ACCESS));
    CHECK(!is_content_update(IN_MOVED_FROM));
    CHECK(!is_content_update(IN_DELETE));
}

TEST("inotify: removal and overflow are recognised") {
    CHECK(is_removal(IN_DELETE));
    CHECK(is_removal(IN_MOVED_FROM));
    CHECK(!is_removal(IN_MOVED_TO));
    CHECK(is_overflow(IN_Q_OVERFLOW));
}
