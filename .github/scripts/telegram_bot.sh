#!/bin/env bash

msg="*$TITLE*
\\#ci\\_$VERSION
\`\`\`
$COMMIT_MESSAGE
\`\`\`
[Commit]($COMMIT_URL)
[Workflow run]($RUN_URL)
"

file="$1"

if [ ! -f "$file" ]; then
	echo "error: File not found: $file" >&2
	exit 1
fi

# The thumbnail is optional and purely cosmetic. Telegram requires a JPEG here, and the asset
# is not guaranteed to exist in every checkout, so it is attached only when the file is present.
# Previously this pointed at a hard-coded path that no longer exists, and curl aborted the whole
# upload with exit 26 (read error) when the file was missing. Set TELEGRAM_THUMBNAIL to a real
# JPEG to re-enable it.
thumb_args=()
thumbnail="${TELEGRAM_THUMBNAIL:-}"
if [ -n "$thumbnail" ] && [ -f "$thumbnail" ]; then
	thumb_args=(-F thumb=@"$thumbnail")
fi

curl -sS "https://api.telegram.org/bot$BOT_TOKEN/sendDocument" \
	-F document=@"$file" \
	-F chat_id="$CHAT_ID" \
	-F "disable_web_page_preview=true" \
	-F "parse_mode=markdownv2" \
	"${thumb_args[@]}" \
	-F caption="$msg"
