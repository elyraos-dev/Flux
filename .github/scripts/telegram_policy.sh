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
# Decide whether this run may notify Telegram, and why.
#
# This script decides; it never sends, and it never touches a secret. That separation is the
# point: the decision has to be inspectable and testable on a laptop, and it must be reachable
# without handing Telegram credentials to a step that might not be allowed to use them.
#
# The workflow consumes `should_send` in the upload step's `if:`, so the gate is evaluated by
# Actions *before* that step exists — which is what keeps secrets out of a pull_request run.
# The upload step also repeats the pull_request check literally, so the most important boundary
# is visible at the step itself and does not depend on this script being correct.
#
# Inputs (environment, all non-secret):
#   EVENT_NAME                      github.event_name
#   FLUX_TELEGRAM_ENABLED           repository variable: master switch
#   FLUX_TELEGRAM_BUILD_ENABLED     repository variable: development/push builds
#   FLUX_TELEGRAM_RELEASE_ENABLED   repository variable: releases
#   NOTIFY_TELEGRAM_INPUT           workflow_dispatch input `notify_telegram`
#
# Outputs (GITHUB_OUTPUT):
#   should_send  true|false
#   channel      build|release|none
#   reason       human-readable, safe to print
#
# Every gate defaults to "do not send": an unset variable, an unknown event and an absent input
# all resolve to skipped. Notification is opt-in, so a misconfiguration goes quiet rather than
# broadcasting.

set -euo pipefail

EVENT_NAME="${EVENT_NAME:-}"
FLUX_TELEGRAM_ENABLED="${FLUX_TELEGRAM_ENABLED:-}"
FLUX_TELEGRAM_BUILD_ENABLED="${FLUX_TELEGRAM_BUILD_ENABLED:-}"
FLUX_TELEGRAM_RELEASE_ENABLED="${FLUX_TELEGRAM_RELEASE_ENABLED:-}"
NOTIFY_TELEGRAM_INPUT="${NOTIFY_TELEGRAM_INPUT:-}"

should_send="false"
channel="none"
reason=""

# Repository variables arrive as strings. Accept only an explicit, lowercase-insensitive
# "true"; anything else (unset, empty, "1", "yes", "TRUE ") is not an opt-in. Being strict here
# means a typo disables notification instead of silently enabling it.
is_true() {
	[ "$(printf '%s' "${1:-}" | tr '[:upper:]' '[:lower:]')" = "true" ]
}

decide() {
	# Gate 1 — event. A pull_request may come from a fork and runs untrusted code; it must
	# never reach notification secrets, regardless of how the variables are set.
	if [ "${EVENT_NAME}" = "pull_request" ] || [ "${EVENT_NAME}" = "pull_request_target" ]; then
		reason="pull_request events are not allowed to access notification secrets"
		return
	fi

	# Gate 2 — master switch.
	if ! is_true "${FLUX_TELEGRAM_ENABLED}"; then
		reason="FLUX_TELEGRAM_ENABLED is not true"
		return
	fi

	# Gate 3 — per-channel switch, plus an explicit manual opt-in for releases.
	case "${EVENT_NAME}" in
	push)
		channel="build"
		if ! is_true "${FLUX_TELEGRAM_BUILD_ENABLED}"; then
			reason="FLUX_TELEGRAM_BUILD_ENABLED is false"
			return
		fi
		should_send="true"
		reason="development build notification is enabled for push events"
		;;
	workflow_dispatch)
		channel="release"
		# workflow_dispatch is not on its own a request to notify. Someone re-running a build
		# to check it still compiles has not asked to post to a channel.
		if ! is_true "${NOTIFY_TELEGRAM_INPUT}"; then
			reason="notify_telegram input is false"
			return
		fi
		if ! is_true "${FLUX_TELEGRAM_RELEASE_ENABLED}"; then
			reason="FLUX_TELEGRAM_RELEASE_ENABLED is false"
			return
		fi
		should_send="true"
		reason="release notification explicitly requested via notify_telegram"
		;;
	"")
		reason="no event name supplied"
		;;
	*)
		channel="none"
		reason="event '${EVENT_NAME}' is not a notification source"
		;;
	esac
}

decide

if [ -n "${GITHUB_OUTPUT:-}" ]; then
	{
		printf 'should_send=%s\n' "${should_send}"
		printf 'channel=%s\n' "${channel}"
		printf 'reason=%s\n' "${reason}"
	} >>"${GITHUB_OUTPUT}"
fi

# Report the decision now. When the answer is "skipped" nothing later runs to explain why, and
# a silent skip is indistinguishable from a broken workflow.
if [ "${should_send}" != "true" ] && [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
	{
		printf '### Telegram notification: skipped\n\n'
		printf -- '- **Reason:** %s\n' "${reason}"
		printf -- '- **Event:** %s\n' "${EVENT_NAME}"
		printf -- '- **Channel:** %s\n' "${channel}"
	} >>"${GITHUB_STEP_SUMMARY}"
fi

printf 'Telegram notification: %s\n' "$([ "${should_send}" = "true" ] && echo "enabled (channel: ${channel})" || echo "skipped")"
printf 'Reason: %s\n' "${reason}"
