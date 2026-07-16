/*
 * Copyright (C) 2024-2026 Rem01Gaming
 * Copyright (C) 2024-2026 FebriCahyaa
 *
 * Adapted from Encore Tweaks (https://github.com/Rem01Gaming/encore).
 * Modified by the Flux project.
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

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

#include <Flux.hpp>
#include <TelemetrySnapshot.hpp> // flux::telemetry::TelemetryHealth (canonical)

/**
 * @file ProfilePolicy.hpp
 * @brief Pure profile-selection policy.
 *
 * Deliberately has no dependency on sysfs, shell, logging or the Android framework: it maps
 * (telemetry, session, config, clock) to (profile, reason). That makes every safety rule in
 * here — thermal direction, hysteresis, priority ordering, the audio guard — executable and
 * assertable on a host machine, which is where they are now tested.
 */

/**
 * @brief Why a profile transition happened.
 *
 * Recorded on every transition. Without this, a user reporting "it dropped out of
 * performance" gives no way to tell a thermal downgrade from a battery-saver switch from a
 * telemetry outage.
 */
enum class TransitionReason {
    None,
    Startup,
    GameStarted,
    GameEnded,
    ThermalPressure,      ///< headroom crossed the enter threshold
    ThermalEmergency,     ///< at or past severe throttling; immediate conservative move
    ThermalRecovered,     ///< headroom fell back below the exit threshold
    BatterySaverEnabled,
    BatterySaverDisabled,
    ScreenOff,
    ScreenOn,
    TelemetryStale,       ///< late; hold, never promote
    TelemetryOffline,     ///< gone; fall back to safe
    TelemetryRestored,
    ChargingStateChanged,
    ConfigurationReloaded,
    UserOverride,
    ShutdownRequested,
};

const char *transition_reason_string(TransitionReason reason);
const char *profile_mode_string(FluxProfileMode mode);



/** Policy state that persists between evaluations. */
struct PolicyState {
    FluxProfileMode current      = PERFCOMMON;
    TransitionReason last_reason = TransitionReason::None;

    int64_t last_thermal_switch_ms = 0;

    // Edge detection, so a *change* in battery saver / screen / charging can be named as the
    // reason rather than inferred.
    bool prev_battery_saver = false;
    bool prev_screen_awake  = true;
    bool prev_charging      = false;
    bool had_telemetry      = false;
};

/** The outcome of one evaluation. */
struct PolicyDecision {
    FluxProfileMode profile = PERFCOMMON;
    TransitionReason reason = TransitionReason::None;
    bool changed            = false;

    /// True when the reason is a safety rule. Safety decisions ignore the audio guard.
    bool safety_driven = false;
};

/** One recorded transition, for diagnostics. */
struct TransitionRecord {
    FluxProfileMode from    = PERFCOMMON;
    FluxProfileMode to      = PERFCOMMON;
    TransitionReason reason = TransitionReason::None;
    int64_t monotonic_ms    = 0;

    std::string package;
    float thermal_headroom = 0.0f;
    bool thermal_valid     = false;
    /// Canonical V2 health. There is deliberately no second health enum in Flux.
    flux::telemetry::TelemetryHealth health = flux::telemetry::TelemetryHealth::Unavailable;

    bool applied      = true;  ///< false when the apply step reported failure
    std::string apply_error;
};

/**
 * @brief Bounded transition history.
 *
 * Bounded on purpose: this runs for the uptime of the device, and an unbounded log of every
 * profile change is a slow memory leak.
 */
class TransitionHistory {
public:
    explicit TransitionHistory(size_t capacity = 64) : capacity_(capacity) {}

    void record(const TransitionRecord &record) {
        records_.push_back(record);
        while (records_.size() > capacity_) records_.pop_front();
    }

    [[nodiscard]] const std::deque<TransitionRecord> &records() const { return records_; }
    [[nodiscard]] size_t size() const { return records_.size(); }

private:
    size_t capacity_;
    std::deque<TransitionRecord> records_;
};

