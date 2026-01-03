#!/usr/bin/env bash
set -e

if [ "$(id -u)" -ne 0 ]; then
  echo "[ERROR] 这个脚本必须用 root 执行"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_FILE="$SCRIPT_DIR/deps.list"

mapfile -t DEPS < <(grep -vE '^\s*#|^\s*$' "$DEPS_FILE")

dnf makecache -y
dnf install -y "${DEPS[@]}"
