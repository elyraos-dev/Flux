#!/usr/bin/env bash
#
# Upload a verified build artifact to Telegram.
#
# The decision to send is NOT made here — telegram_policy.sh makes it, and the workflow's `if:`
# enforces it before this step (and therefore before its secrets) exists. By the time this runs,
# sending has already been authorised. What is left is to do it safely:
#
#   - refuse to run without complete configuration, rather than calling the API and letting it
#     fail with something cryptic;
#   - refuse to upload anything that is not the artifact this build produced;
#   - never let the bot token reach the log, including via curl's own error messages.
#
# Failure here is reported, never fatal to the build: a published release is valid whether or
# not a chat was told about it, and a notification error must never be mistaken for a build
# error. The workflow keeps this non-blocking with a step-scoped `continue-on-error`, not a
# job-wide one.
#
# Environment:
#   TELEGRAM_BOT_TOKEN  (secret, required)  never printed
#   TELEGRAM_CHAT_ID    (secret, required)  never printed
#   TELEGRAM_THUMBNAIL  (optional)          path to a JPEG
#   TITLE, VERSION, COMMIT_MESSAGE, COMMIT_URL, RUN_URL   caption content
#
# Usage: telegram_bot.sh <artifact.zip>

set -uo pipefail

file="${1:-}"

err() { printf '::error::%s\n' "$1"; }

# Everything written to the log passes through here. curl can quote the URL it was given in an
# error message, and that URL contains the bot token, so redaction is applied to output rather
# than trusted to never be needed. GitHub also masks registered secrets, but that is a backstop
# owned by someone else; this script does not depend on it.
redact() {
	local text="$1"
	if [ -n "${TELEGRAM_BOT_TOKEN:-}" ]; then
		text="${text//${TELEGRAM_BOT_TOKEN}/***}"
	fi
	if [ -n "${TELEGRAM_CHAT_ID:-}" ]; then
		text="${text//${TELEGRAM_CHAT_ID}/***}"
	fi
	printf '%s' "${text}"
}

summary() {
	if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
		printf '%b\n' "$1" >>"${GITHUB_STEP_SUMMARY}"
	fi
}

fail_notification() {
	local headline="$1" detail="$2" action="${3:-}"
	err "Telegram notification failed: ${headline}"
	summary "### Telegram notification: failed\n"
	summary "- **Reason:** ${headline}"
	[ -n "${detail}" ] && summary "- **Detail:** ${detail}"
	[ -n "${action}" ] && summary "- **Action:** ${action}"
	summary "\nThe build and any published release are unaffected: this step only reports results."
	exit 1
}

# ── Configuration ────────────────────────────────────────────────────────────
# Check for presence, and say which one is missing — without echoing either value.
if [ -z "${TELEGRAM_BOT_TOKEN:-}" ]; then
	fail_notification "TELEGRAM_BOT_TOKEN is not configured" \
		"The notification gates allowed this run to send, but the bot token secret is empty or unset." \
		"Set the TELEGRAM_BOT_TOKEN repository secret, or set FLUX_TELEGRAM_ENABLED to false."
fi
if [ -z "${TELEGRAM_CHAT_ID:-}" ]; then
	fail_notification "TELEGRAM_CHAT_ID is not configured" \
		"The notification gates allowed this run to send, but the destination chat secret is empty or unset." \
		"Set the TELEGRAM_CHAT_ID repository secret, or set FLUX_TELEGRAM_ENABLED to false."
fi

# ── Artifact safety ──────────────────────────────────────────────────────────
# Only the artifact this build produced may be uploaded. The path is resolved and confined to
# the workspace so a crafted or relative argument cannot make this step exfiltrate a file from
# elsewhere on the runner (a key, a config, /etc/passwd) to a chat.
if [ -z "${file}" ]; then
	fail_notification "no artifact was passed to the notifier" \
		"telegram_bot.sh requires the artifact path as its first argument."
fi
if [ ! -f "${file}" ]; then
	fail_notification "artifact not found: ${file}" \
		"The upload step ran but the build artifact does not exist." \
		"Check that the packaging step produced the zip it reported."
fi

workspace="$(cd "${GITHUB_WORKSPACE:-$PWD}" && pwd -P)"
resolved="$(cd "$(dirname "${file}")" && pwd -P)/$(basename "${file}")"
case "${resolved}" in
"${workspace}"/*) ;;
*)
	fail_notification "artifact is outside the build workspace" \
		"Refusing to upload '${resolved}', which is not inside '${workspace}'." \
		"Only artifacts produced by this build may be sent."
	;;
esac

case "${resolved}" in
*.zip) ;;
*)
	fail_notification "artifact is not a flashable zip" \
		"Refusing to upload '${resolved##*/}': only the build's .zip artifact may be sent."
	;;
esac

# A truncated or half-written zip must not be presented to anyone as a build.
if ! unzip -tqq "${resolved}" >/dev/null 2>&1; then
	fail_notification "artifact is not a readable zip" \
		"'${resolved##*/}' failed an integrity test; it may be truncated or partially written." \
		"Do not distribute this artifact."
fi

checksum="$(sha256sum "${resolved}" | cut -d' ' -f1)" || checksum=""
if [ -z "${checksum}" ]; then
	fail_notification "could not checksum the artifact" \
		"sha256sum failed for '${resolved##*/}'."
fi

# ── Caption ──────────────────────────────────────────────────────────────────
msg="*${TITLE:-Flux}*
\\#ci\\_${VERSION:-0}
\`\`\`
${COMMIT_MESSAGE:-}
\`\`\`
[Commit](${COMMIT_URL:-})
[Workflow run](${RUN_URL:-})
"

# The thumbnail is optional and purely cosmetic; Telegram requires a JPEG. It is attached only
# when it exists, because a missing file made curl abort the whole upload with exit 26.
thumb_args=()
thumbnail="${TELEGRAM_THUMBNAIL:-}"
if [ -n "${thumbnail}" ] && [ -f "${thumbnail}" ]; then
	thumb_args=(-F thumb=@"${thumbnail}")
fi

# ── Send ─────────────────────────────────────────────────────────────────────
printf 'Uploading %s (sha256 %s) to Telegram...\n' "${resolved##*/}" "${checksum}"

body_file="$(mktemp)"
stderr_file="$(mktemp)"
trap 'rm -f "${body_file}" "${stderr_file}"' EXIT

# --fail is deliberately NOT used: Telegram reports application errors in a 200 body as often as
# via a status code, and the body is where the actionable description lives. Capture the status
# separately and decide afterwards.
http_code="$(
	curl -sS -o "${body_file}" -w '%{http_code}' \
		"https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendDocument" \
		-F document=@"${resolved}" \
		-F chat_id="${TELEGRAM_CHAT_ID}" \
		-F "disable_web_page_preview=true" \
		-F "parse_mode=markdownv2" \
		"${thumb_args[@]}" \
		-F caption="${msg}" \
		2>"${stderr_file}"
)" || curl_status=$?
curl_status="${curl_status:-0}"

curl_err="$(redact "$(cat "${stderr_file}")")"
body="$(redact "$(cat "${body_file}")")"

if [ "${curl_status}" -ne 0 ]; then
	fail_notification "could not reach the Telegram API (curl exit ${curl_status})" \
		"${curl_err:-no additional detail}" \
		"This is a transport failure, not a rejected message. Retry the notification later."
fi

description="$(printf '%s' "${body}" | sed -n 's/.*"description":"\([^"]*\)".*/\1/p')"
ok="$(printf '%s' "${body}" | sed -n 's/.*"ok":\([a-z]*\).*/\1/p')"

if [ "${ok}" = "true" ]; then
	printf 'Telegram notification sent.\n'
	summary "### Telegram notification: sent\n"
	summary "- **Artifact:** \`${resolved##*/}\`"
	summary "- **SHA-256:** \`${checksum}\`"
	exit 0
fi

# ── Actionable failure ───────────────────────────────────────────────────────
# Say what to check. "chat not found" in particular is almost never a bug in the workflow, and
# an operator reading this should not have to guess which of several things is wrong.
action=""
case "${http_code}:${description}" in
400:*"chat not found"*)
	action="Verify TELEGRAM_CHAT_ID names the intended destination; for a channel use its \
numeric chat id (usually -100…) or @username. Confirm the bot has been added to that \
destination and may post there, and that the destination is the intended Flux channel. \
Do not try other chat ids from CI."
	;;
400:*"not enough rights"* | 403:*)
	action="The bot reached the destination but may not post there. Grant it permission to \
send messages and documents in that chat/channel."
	;;
401:*)
	action="TELEGRAM_BOT_TOKEN was rejected. Confirm the secret holds a current bot token."
	;;
413:*)
	action="The artifact exceeds Telegram's upload limit for bots. Distribute it via the \
GitHub release instead."
	;;
429:*)
	action="Rate limited by Telegram. Retry the notification later."
	;;
*)
	action="Inspect the Telegram API response above."
	;;
esac

fail_notification \
	"Telegram API returned HTTP ${http_code}${description:+ ${description}}" \
	"Response: ${body:-<empty>}" \
	"${action}"
