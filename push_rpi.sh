#!/bin/bash
# push.sh - Copy file from host to Raspberry Pi /usr/bin
# Usage: ./push.sh <path-to-file> <host> [user] [dest]
# Example: ./push.sh ./myprog 172.29.238.22 root /usr/bin/

set -euo pipefail

# Check number of arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <path-to-file> <host> [user] [dest]"
    echo "  <path-to-file> : path to the local file"
    echo "  <host>         : target IP address or hostname (mandatory)"
    echo "  [user]         : target user (default: root)"
    echo "  [dest]         : destination directory on target (default: /usr/bin/)"
    exit 1
fi

SRC="$1"
HOST="${2:-"192.168.13.88"}"
USER="${3:-vantien}"
DEST="${4:-~/graduation_project}"

# Check if source file exists and is readable
if [ ! -f "$SRC" ]; then
    echo "Error: source file '$SRC' does not exist or is not a regular file."
    exit 2
fi

if [ ! -r "$SRC" ]; then
    echo "Error: source file '$SRC' is not readable."
    exit 3
fi

# Perform the copy using scp
echo "Copying '$SRC' to ${USER}@${HOST}:${DEST}..."
scp -v "$SRC" "${USER}@${HOST}:${DEST}"

if [ $? -eq 0 ]; then
    echo "File copy completed successfully."
else
    echo "File copy failed."
    exit 4
fi