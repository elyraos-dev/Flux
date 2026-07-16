#!/usr/bin/env bash
#
# Copyright (C) 2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Schema-v2 compatibility proof for the pinned SynthesisCore artifact.
#
# scripts/verify-synthesiscore.sh already refuses to package an artifact whose bytes do not
# match dependencies/synthesiscore.lock. That proves *integrity* — the bytes are the pinned
# bytes — but it takes the lock's own `commit` and `schema_provided` fields on trust. A lock
# is an assertion; this script checks the assertion against the artifact itself.
#
# It answers three questions the lock cannot answer about itself:
#
#   1. was this APK really built from the commit the lock names?   (embedded build metadata)
#   2. does this APK really speak the v2 wire contract?            (embedded string table)
#   3. does Flux's decoder really require v2, with no v1 path?     (source constants)
#
# ## On reading the literal schema number out of the APK
#
# The obvious proof would be to read `TelemetryContract.SCHEMA_VERSION` out of classes.dex and
# assert it equals 2. That is deliberately NOT attempted:
#
#   - `SCHEMA_VERSION` is a Kotlin `const val`, i.e. a compile-time constant. R8 inlines it
#     directly into its call sites as a `const/4` instruction operand. There is no reliable
#     constant-pool entry left to read, and any value recovered would come from disassembling
#     bytecode whose shape changes with every toolchain release.
#   - Integers are not in the dex string table, so no string-level extraction is possible.
#
# What *does* survive R8 verbatim is the wire contract's key strings, because they are emitted
# into the telemetry file as text. Those keys are the schema: an artifact that emits every v2
# key, from the commit the lock pins, is a v2 artifact. That is the proof used below, and it
# rests on bytes rather than on the lock's own claims or on the file's name.
#
# Belt and braces: even if a wrong-schema artifact somehow shipped, Flux's decoder rejects any
# `schema_version` outside [kSchemaMin, kSchemaMax] at runtime and degrades to Offline rather
# than misreading it. Check (3) proves that gate is really set to v2-only.
#
# Exit codes: 0 proven; 1 a proof failed.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${REPO_ROOT}"

LOCK_FILE="dependencies/synthesiscore.lock"
APK="prebuilt/synthesiscore.apk"
SNAPSHOT_HPP="jni/engine/telemetry/TelemetrySnapshot.hpp"

red() { printf '\033[31m%s\033[0m\n' "$1"; }
green() { printf '\033[32m%s\033[0m\n' "$1"; }
info() { printf '\033[36m•\033[0m %s\n' "$1"; }
head2() { printf '\n\033[1m%s\033[0m\n' "$1"; }

FAILURES=0
fail() {
	red "FAIL: $1"
	FAILURES=$((FAILURES + 1))
}

lock_field() {
	local value
	value="$(grep -E "^$1=" "${LOCK_FILE}" | head -n1 | cut -d= -f2-)"
	if [ -z "${value}" ]; then
		red "ERROR: lock is missing required field '$1'"
		exit 1
	fi
	printf '%s' "${value}"
}

for f in "${LOCK_FILE}" "${APK}" "${SNAPSHOT_HPP}"; do
	if [ ! -f "${f}" ]; then
		red "ERROR: required input missing: ${f}"
		exit 1
	fi
done

LOCK_SHA="$(lock_field sha256)"
LOCK_COMMIT="$(lock_field commit)"
LOCK_PROVIDED="$(lock_field schema_provided)"
LOCK_MIN="$(lock_field schema_min)"
LOCK_MAX="$(lock_field schema_max)"

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

# ── Proof 1: the artifact is the pinned artifact, built from the pinned commit ──
head2 "Proof 1: artifact identity and build provenance"

ACTUAL_SHA="$(sha256sum "${APK}" | cut -d' ' -f1)"
if [ "${ACTUAL_SHA}" = "${LOCK_SHA}" ]; then
	green "  bytes match the lock (sha256 ${ACTUAL_SHA})"
else
	fail "checksum mismatch: lock says ${LOCK_SHA}, artifact is ${ACTUAL_SHA}"
fi

# The Android Gradle Plugin embeds the source revision it built from into the APK. This is the
# artifact's own account of its origin, independent of the file's name and of the lock.
VCS_INFO="META-INF/version-control-info.textproto"
if unzip -qo "${APK}" "${VCS_INFO}" -d "${WORK}" 2>/dev/null && [ -f "${WORK}/${VCS_INFO}" ]; then
	APK_REVISION="$(sed -n 's/.*revision:[[:space:]]*"\([0-9a-f]\{40\}\)".*/\1/p' "${WORK}/${VCS_INFO}" | head -1)"
	if [ -z "${APK_REVISION}" ]; then
		fail "${VCS_INFO} carries no readable revision"
	elif [ "${APK_REVISION}" = "${LOCK_COMMIT}" ]; then
		green "  built from the pinned commit ${APK_REVISION}"
		green "  (read from the APK's own embedded build metadata, not from the lock or filename)"
	else
		fail "the artifact was built from a different commit than the lock pins"
		fail "  lock:     ${LOCK_COMMIT}"
		fail "  artifact: ${APK_REVISION}"
	fi
else
	fail "the APK carries no ${VCS_INFO}, so its build provenance cannot be proven from its"
	fail "  bytes. Re-publish SynthesisCore with AGP version-control info enabled."
fi

# ── Proof 2: the artifact speaks the v2 wire contract ──────────────────────────
head2 "Proof 2: wire contract carried by the artifact"

unzip -qo "${APK}" 'classes*.dex' -d "${WORK}" || {
	fail "the APK contains no dex"
	exit 1
}

DEX_STRINGS="${WORK}/dex.strings"
: >"${DEX_STRINGS}"
for dex in "${WORK}"/classes*.dex; do
	strings -a "${dex}" >>"${DEX_STRINGS}"
done
info "$(wc -l <"${DEX_STRINGS}") strings across $(find "${WORK}" -name 'classes*.dex' | wc -l) dex file(s)"

# Every key of telemetry contract v2. These are emitted as literal text into the status file,
# so they survive R8 verbatim and are readable from the artifact's bytes.
V2_KEYS=(
	schema_version
	thermal_headroom
	thermal_valid
	thermal_status
	thermal_available
	thermal_age_ms
	thermal_sample_elapsed_ms
	charging_state
	battery_saver
	screen_awake
	screen_available
	focused_package
	focused_pid
	focused_uid
)

MISSING_KEYS=0
for key in "${V2_KEYS[@]}"; do
	if grep -qxF "${key}" "${DEX_STRINGS}"; then
		green "  emits: ${key}"
	else
		fail "the artifact does not carry the v2 contract key '${key}'"
		MISSING_KEYS=$((MISSING_KEYS + 1))
	fi
done
if [ "${MISSING_KEYS}" -eq 0 ]; then
	green "  all ${#V2_KEYS[@]} v2 contract keys are present in the artifact"
fi

# ── Proof 3: Flux requires v2 and has no v1 path ───────────────────────────────
head2 "Proof 3: Flux's accepted schema range"

SRC_MIN="$(sed -n 's/^inline constexpr int kSchemaMin = \([0-9]*\);.*/\1/p' "${SNAPSHOT_HPP}" | head -1)"
SRC_MAX="$(sed -n 's/^inline constexpr int kSchemaMax = \([0-9]*\);.*/\1/p' "${SNAPSHOT_HPP}" | head -1)"

if [ -z "${SRC_MIN}" ] || [ -z "${SRC_MAX}" ]; then
	fail "could not read kSchemaMin/kSchemaMax from ${SNAPSHOT_HPP}"
else
	info "decoder accepts schema ${SRC_MIN}..${SRC_MAX}; lock declares ${LOCK_MIN}..${LOCK_MAX}"
	if [ "${SRC_MIN}" != "${LOCK_MIN}" ] || [ "${SRC_MAX}" != "${LOCK_MAX}" ]; then
		fail "the lock's accepted schema range disagrees with the decoder's. One of them is lying"
		fail "  to whoever reads it; they must be changed together."
	else
		green "  the lock and the decoder agree on the accepted range"
	fi

	# The point of the cutover: v1 must not be consumable by the production decoder.
	if [ "${SRC_MIN}" -le 1 ]; then
		fail "the decoder still accepts schema ${SRC_MIN} — a schema-v1 production dependency"
		fail "  remains. Increment 2 removed the v1 implementation; the gate must not admit v1."
	else
		green "  no schema-v1 production dependency: the decoder rejects anything below v2"
	fi

	if [ "${LOCK_PROVIDED}" -lt "${SRC_MIN}" ] || [ "${LOCK_PROVIDED}" -gt "${SRC_MAX}" ]; then
		fail "the pinned artifact emits schema ${LOCK_PROVIDED}, outside the decoder's ${SRC_MIN}..${SRC_MAX}"
	else
		green "  the pinned artifact's schema ${LOCK_PROVIDED} is inside the decoder's range"
	fi
fi

head2 "═══ Result ═══"
if [ "${FAILURES}" -ne 0 ]; then
	red "${FAILURES} proof(s) failed."
	red "SynthesisCore schema-v2 compatibility: NOT PROVEN"
	exit 1
fi
green "SynthesisCore schema-v2 compatibility: PROVEN"
green "  - the shipped artifact is byte-identical to the pinned release"
green "  - the artifact's own metadata confirms the commit the lock pins"
green "  - the artifact carries the complete v2 wire contract"
green "  - Flux's decoder requires v2 and admits no v1"
