#!/bin/bash
set -euo pipefail

# =========================
# MultiAgent-Unreal 启动脚本 (带模拟后端) - Linux 版
# =========================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# UE5 路径配置
export UE5_ROOT="/home/zhugb/Software/UnrealEngine"
UE_EDITOR_BIN="$UE5_ROOT/Engine/Binaries/Linux/UnrealEditor"

# 项目路径
PROJECT_PATH="$SCRIPT_DIR/unreal_project/MultiAgent.uproject"
MOCK_BACKEND_SCRIPT="$SCRIPT_DIR/scripts/mock_backend.py"

# 端口配置
BACKEND_PORT=8081

# =========================
# 检查文件
# =========================
if [[ ! -x "$UE_EDITOR_BIN" ]]; then
  echo "错误: 找不到 UnrealEditor: $UE_EDITOR_BIN"
  exit 1
fi

if [[ ! -f "$PROJECT_PATH" ]]; then
  echo "错误: 找不到项目文件: $PROJECT_PATH"
  exit 1
fi

# =========================
# 启动模拟后端 (后台运行)
# =========================
echo "========================================"
echo "  启动模拟后端服务器 (端口: $BACKEND_PORT)"
echo "========================================"

# 在新终端窗口启动 (gnome-terminal)
if command -v gnome-terminal >/dev/null 2>&1; then
  gnome-terminal -- bash -c "cd '$SCRIPT_DIR' && python3 '$MOCK_BACKEND_SCRIPT' --port $BACKEND_PORT; exec bash"
elif command -v xterm >/dev/null 2>&1; then
  xterm -e "cd '$SCRIPT_DIR' && python3 '$MOCK_BACKEND_SCRIPT' --port $BACKEND_PORT" &
else
  # 没有图形终端，后台运行
  echo "在后台启动模拟后端..."
  python3 "$MOCK_BACKEND_SCRIPT" --port $BACKEND_PORT &
  BACKEND_PID=$!
  echo "模拟后端 PID: $BACKEND_PID"
fi

sleep 2

# 检查后端是否启动
if curl -s "http://localhost:$BACKEND_PORT/" >/dev/null 2>&1; then
  echo "✓ 模拟后端已启动: http://localhost:$BACKEND_PORT"
else
  echo "⚠ 模拟后端可能尚未完全启动"
fi

echo ""
echo "========================================"
echo "  启动 Unreal Editor..."
echo "========================================"

# 启动 UE5
exec "$UE_EDITOR_BIN" "$PROJECT_PATH" "$@" -ResX=1280 -ResY=720
