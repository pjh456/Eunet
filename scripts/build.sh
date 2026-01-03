#!/usr/bin/env bash
set -e

# -----------------------------
# 基础信息
# -----------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# -----------------------------
# step 0: 不能用 root 跑
# -----------------------------
if [ "$(id -u)" -eq 0 ]; then
  echo "[ERROR] build.sh 不允许以 root 执行"
  exit 1
fi

# -----------------------------
# step 1: 系统依赖（自动 sudo）
# -----------------------------
echo "[STEP 1] 安装系统依赖（需要 sudo）"
sudo bash "$SCRIPT_DIR/install_system_deps.sh"

# -----------------------------
# step 2: third_party
# -----------------------------
echo "[STEP 2] 拉取第三方依赖"
bash "$SCRIPT_DIR/fetch_third_party.sh"

# -----------------------------
# step 3: cmake 构建
# -----------------------------
echo "[STEP 3] CMake 构建"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"

echo
echo "[OK] 构建完成"
