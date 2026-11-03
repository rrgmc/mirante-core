#!/bin/sh
# Signs a macOS disk image, notarises it with an App Store Connect API key, and
# staples the ticket to it. The signing job uses this. See
# docs/decisions/0126-macos-signing-is-a-separate-job.md.
#
#   sh tools/notarise-macos.sh <disk image> <identity>
#
# Needs MIRANTE_NOTARY_KEY (the path of the .p8 file), MIRANTE_NOTARY_KEY_ID and
# MIRANTE_NOTARY_ISSUER.

set -eu

[ $# -eq 2 ] || { echo "usage: sh tools/notarise-macos.sh <disk image> <identity>" >&2; exit 2; }
DMG=$1
IDENTITY=$2

notary() {
    xcrun notarytool "$@" --key "$MIRANTE_NOTARY_KEY" \
        --key-id "$MIRANTE_NOTARY_KEY_ID" --issuer "$MIRANTE_NOTARY_ISSUER"
}

codesign --sign "$IDENTITY" --timestamp "$DMG"

# notarytool can exit with 0 for a rejected submission, so check the status it prints.
RESULT=$(notary submit "$DMG" --wait)
echo "$RESULT"
if ! echo "$RESULT" | grep -q "status: Accepted"; then
    ID=$(echo "$RESULT" | awk '/^ *id:/ { print $2; exit }')
    echo "Notarisation failed. Apple's log:" >&2
    notary log "$ID" >&2 || true
    exit 1
fi
# Apple's ticket server sometimes drops the connection, so the staple is tried
# a few times before the job fails.
attempt=1
until xcrun stapler staple "$DMG"; do
    if [ "$attempt" -ge 5 ]; then
        echo "Stapling failed $attempt times." >&2
        exit 1
    fi
    attempt=$((attempt + 1))
    echo "Stapling failed; trying again in 30 seconds." >&2
    sleep 30
done
spctl --assess --type open --context context:primary-signature --verbose "$DMG"
