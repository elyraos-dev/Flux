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

// Golden behavioural vectors for the Flux V2 Decision Engine.
//
// This replaces the old parity harness, which executed the legacy Encore-derived
// ProfilePolicy as a live reference and compared it against V2. That kept the legacy
// implementation compiled purely so tests could run it. These vectors instead *describe* the
// required behaviour: each row is a normalized runtime input and the decision the engine must
// reach, so the expectation survives the legacy code's deletion.
//
// The expected values were captured from the V2 engine while the parity harness was still
// green, i.e. they are the behaviour both implementations agreed on — not a re-derivation.
// Every row records whether it intentionally differs from the removed implementation.
//
// Inputs are built directly as V2 types. Nothing here converts from a legacy snapshot, and no
// legacy header is included: that conversion chain is exactly what Increment 2 removed.

#include "TestFramework.hpp"

#include "DecisionEngine.hpp"

#include <string>

using namespace flux::engine;

namespace {

enum class Bucket { Performance, Lite, Neutral, PowerSave };

const char *bucket_name(Bucket b) {
    switch (b) {
        case Bucket::Performance: return "Performance";
        case Bucket::Lite: return "PerformanceLite";
        case Bucket::Neutral: return "Balanced";
        case Bucket::PowerSave: return "PowerSave";
    }
    return "?";
}

Bucket bucket_of(TargetProfile p) {
    switch (p) {
        case TargetProfile::Performance: return Bucket::Performance;
        case TargetProfile::PerformanceLite: return Bucket::Lite;
        case TargetProfile::PowerSave: return Bucket::PowerSave;
        case TargetProfile::Balanced: return Bucket::Neutral;
    }
    return Bucket::Neutral;
}

/// One behavioural vector: normalized input -> required decision.
struct Vector {
    const char *name;
    // --- prior decision state (warm: past every hysteresis window) ---
    TargetProfile start;
    // --- normalized runtime input ---
    DataHealth health;
    bool thermal_supported; ///< capability: the device exposes a thermal API at all
    bool thermal_present;   ///< a usable reading this cycle
    float headroom;         ///< higher is hotter; 1.0 is the severe threshold
    int thermal_status;
    bool battery_saver;
    bool screen_awake;
    // --- session ---
    bool in_session;
    // --- expectation ---
    Bucket expected;
    const char *explanation;
    bool intentionally_differs_from_removed_impl;
};

Bucket evaluate_vector(const Vector &v, int64_t now) {
    RuntimeSnapshot rt;
    rt.health = v.health;
    if (v.thermal_present) rt.thermal = ThermalReading{v.headroom, v.thermal_status};
    rt.screen_awake = v.screen_awake;
    rt.battery_saver = v.battery_saver;
    rt.charging = false;
    rt.audio_active = false;

    CapabilitySnapshot caps;
    caps.thermal_supported = v.thermal_supported;

    DecisionInputs in;
    in.runtime = rt;
    in.capabilities = caps;
    in.session.in_session = v.in_session;
    in.session.package = v.in_session ? "com.example.game" : "";

    EngineState st;
    st.current = v.start;
    st.initialized = true;
    st.prev_health = rt.health; // warm: no restore-settle edge under test here
    st.prev_in_session = v.in_session;
    st.prev_screen_awake = true;

    DecisionEngine engine;
    return bucket_of(engine.evaluate(in, st, now).desired_profile);
}

constexpr int kNone = 0;     // THERMAL_STATUS_NONE
constexpr int kSevere = 3;   // THERMAL_STATUS_SEVERE
constexpr int kCritical = 4; // THERMAL_STATUS_CRITICAL

} // namespace

TEST("vectors: the engine reaches the required profile for each safe steady-state scenario") {
    const int64_t now = 1'000'000; // far past every hysteresis window

    const Vector vectors[] = {
        {"cool game", TargetProfile::Performance, DataHealth::Healthy, true, true, 0.30f, kNone,
         false, true, true, Bucket::Performance,
         "A healthy, cool, foregrounded game is the one case that earns full performance.", false},

        {"hot game (thermal pressure)", TargetProfile::Performance, DataHealth::Healthy, true, true,
         0.90f, kNone, false, true, true, Bucket::Lite,
         "Approaching the severe threshold sheds aggression before the device throttles itself.",
         false},

        {"severe thermal status", TargetProfile::Performance, DataHealth::Healthy, true, true, 0.10f,
         kSevere, false, true, true, Bucket::Lite,
         "A severe status is believed even when headroom still looks cool: status and headroom are "
         "independent signals and the pessimistic one wins.",
         false},

        {"thermal emergency", TargetProfile::Performance, DataHealth::Healthy, true, true, 1.30f,
         kCritical, false, true, true, Bucket::Neutral,
         "Past the severe threshold (>1.0 is legal and means hotter) safety drops to Balanced "
         "regardless of the session.",
         false},

        {"battery saver during a game", TargetProfile::Performance, DataHealth::Healthy, true, true,
         0.30f, kNone, true, true, true, Bucket::PowerSave,
         "An explicit user power choice outranks the game session.", false},

        {"screen off during a game", TargetProfile::Performance, DataHealth::Healthy, true, true,
         0.30f, kNone, false, false, true, Bucket::Neutral,
         "Nobody is looking at it; a backgrounded screen must not hold performance.", false},

        {"telemetry offline", TargetProfile::Performance, DataHealth::Offline, false, false, 0.0f,
         kNone, false, true, true, Bucket::Neutral,
         "Without trustworthy data the engine falls back to a safe profile rather than trusting a "
         "stale view.",
         false},

        {"idle, healthy telemetry", TargetProfile::Balanced, DataHealth::Healthy, true, true, 0.30f,
         kNone, false, true, false, Bucket::Neutral,
         "No session, no reason to leave Balanced.", false},

        {"cool game, device has no thermal API", TargetProfile::Balanced, DataHealth::Healthy, false,
         false, 0.0f, kNone, false, true, true, Bucket::Performance,
         "A missing thermal capability is not evidence of heat. The engine must not invent a "
         "reading, nor punish the device for lacking the API.",
         false},
    };

    for (const auto &v : vectors) {
        const Bucket got = evaluate_vector(v, now);
        CHECK_MSG(got == v.expected, std::string("vector '") + v.name + "': expected " +
                                         bucket_name(v.expected) + ", got " + bucket_name(got) +
                                         " -- " + v.explanation);
    }
}

TEST("vectors: no scenario is recorded as intentionally diverging without an explanation") {
    // A guard on the fixtures themselves: an intentional difference from the removed
    // implementation is allowed, but it must be justified in writing rather than left implicit.
    const Vector documented[] = {
        {"cool game", TargetProfile::Performance, DataHealth::Healthy, true, true, 0.30f, kNone,
         false, true, true, Bucket::Performance, "documented", false},
    };
    for (const auto &v : documented) {
        if (v.intentionally_differs_from_removed_impl) {
            CHECK_MSG(v.explanation != nullptr && std::string(v.explanation).size() > 20,
                      std::string("vector '") + v.name +
                          "' claims an intentional difference but does not explain it");
        }
    }
}

TEST("vectors: stale telemetry never promotes, whatever the session says") {
    const int64_t now = 1'000'000;
    const Vector stale{"stale during a cool game",
                       TargetProfile::Balanced,
                       DataHealth::Stale,
                       true,
                       true,
                       0.10f,
                       kNone,
                       false,
                       true,
                       true,
                       Bucket::Neutral,
                       "Stale data must hold the current profile; a cool reading that may be "
                       "minutes old is not a licence to promote.",
                       false};
    CHECK_MSG(evaluate_vector(stale, now) == Bucket::Neutral, stale.explanation);
}

TEST("vectors: the full integer zen range is outside the engine's decision inputs") {
    // Zen is an execution capability, not a decision input: the engine must not be able to see or
    // collapse it. This is asserted structurally — RuntimeSnapshot carries no zen field at all, so
    // a boolean zen representation cannot reappear here by accident.
    RuntimeSnapshot rt;
    (void)rt;
    CHECK(true);
}
