#!/usr/bin/env bash
set -e

if [ "$(id -u)" -eq 0 ]; then
  echo "[ERROR] 这个脚本禁止 root 执行"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INCLUDE_DIR="$PROJECT_ROOT/include"

mkdir -p "$INCLUDE_DIR"

clone_or_update() {
  local repo_url="$1"
  local dir_name="$2"
  local branch="${3:-master}"
  local target_dir="$INCLUDE_DIR/$dir_name"

  if [ -d "$target_dir/.git" ]; then
    echo "[INFO] $dir_name 已存在，跳过 clone"
  else
    git clone --depth=1 -b "$branch" "$repo_url" "$target_dir"
  fi
}

clone_or_update \
  https://github.com/ArthurSonzogni/FTXUI.git \
  FTXUI \
  v6.1.9
