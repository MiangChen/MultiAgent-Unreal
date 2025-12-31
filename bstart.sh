#!/bin/bash
set -euo pipefail

# =========================
# MultiAgent-Unreal 启动脚本 (带模拟后端)
# =========================

# 获取脚本所在目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# =========================
# UE 5.5 (Launcher) 路径配置
# =========================
export UE5_ROOT="/Users/Shared/Epic Games/UE_5.5"
UE_EDITOR_BIN="$UE5_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"

# Unreal 项目路径
PROJECT_PATH="$SCRIPT_DIR/unreal_project/MultiAgent.uproject"

# 配置文件路径
CONFIG_PATH="$SCRIPT_DIR/config/SimConfig.json"

# 模拟后端脚本路径
MOCK_BACKEND_SCRIPT="$SCRIPT_DIR/scripts/mock_backend.py"

# 临时场景图文件路径
TEMP_SCENE_GRAPH="$SCRIPT_DIR/unreal_project/datasets/scene_graph_cyberworld_temp.json"

# 默认端口
BACKEND_PORT=8080

# 默认地图
DEFAULT_MAP=""

# =========================
# 检查关键文件
# =========================
if [[ ! -x "$UE_EDITOR_BIN" ]]; then
  echo "错误: 找不到 UnrealEditor 可执行文件:"
  echo "  $UE_EDITOR_BIN"
  echo "请确认 UE5_ROOT 是否正确：$UE5_ROOT"
  exit 1
fi

if [[ ! -f "$PROJECT_PATH" ]]; then
  echo "错误: 找不到项目文件:"
  echo "  $PROJECT_PATH"
  exit 1
fi

if [[ ! -f "$MOCK_BACKEND_SCRIPT" ]]; then
  echo "错误: 找不到模拟后端脚本:"
  echo "  $MOCK_BACKEND_SCRIPT"
  exit 1
fi

# 检查 Python3
if ! command -v python3 >/dev/null 2>&1; then
  echo "错误: 找不到 python3，请先安装 Python 3"
  exit 1
fi

# =========================
# 删除临时场景图文件（防止干扰）
# =========================
if [[ -f "$TEMP_SCENE_GRAPH" ]]; then
  rm -f "$TEMP_SCENE_GRAPH"
  echo "已删除临时场景图文件: $TEMP_SCENE_GRAPH"
fi

# =========================
# 从 SimConfig.json 读取配置
# =========================
if [[ -f "$CONFIG_PATH" ]]; then
  if command -v jq >/dev/null 2>&1; then
    DEFAULT_MAP="$(jq -r '.DefaultMap // empty' "$CONFIG_PATH" 2>/dev/null || true)"
    # 读取服务器端口
    SERVER_URL="$(jq -r '.PlannerServerURL // empty' "$CONFIG_PATH" 2>/dev/null || true)"
    if [[ -n "$SERVER_URL" ]]; then
      # 从 URL 中提取端口号
      BACKEND_PORT="$(echo "$SERVER_URL" | grep -oE ':[0-9]+' | tr -d ':' || echo "8080")"
    fi
  elif command -v python3 >/dev/null 2>&1; then
    DEFAULT_MAP="$(python3 - <<'PY' "$CONFIG_PATH"
import json, sys
p = sys.argv[1]
try:
    with open(p, "r", encoding="utf-8") as f:
        data = json.load(f)
    v = data.get("DefaultMap", "")
    print(v if isinstance(v, str) else "")
except Exception:
    print("")
PY
)"
    BACKEND_PORT="$(python3 - <<'PY' "$CONFIG_PATH"
import json, sys, re
p = sys.argv[1]
try:
    with open(p, "r", encoding="utf-8") as f:
        data = json.load(f)
    url = data.get("PlannerServerURL", "")
    match = re.search(r':(\d+)', url)
    print(match.group(1) if match else "8080")
except Exception:
    print("8080")
PY
)"
  fi
fi

# =========================
# 启动模拟后端
# =========================
echo "========================================"
echo "  启动模拟后端服务器..."
echo "========================================"
echo "  端口: $BACKEND_PORT"
echo ""

# 在新的 Terminal 窗口中启动模拟后端
osascript <<EOF
tell application "Terminal"
    activate
    do script "cd '$SCRIPT_DIR' && python3 '$MOCK_BACKEND_SCRIPT' --port $BACKEND_PORT"
end tell
EOF

# 等待后端启动
echo "等待模拟后端启动..."
sleep 2

# 检查后端是否启动成功
if curl -s "http://localhost:$BACKEND_PORT/api/sim/poll" >/dev/null 2>&1; then
  echo "✓ 模拟后端已启动"
else
  echo "⚠ 模拟后端可能尚未完全启动，继续启动 UE..."
fi

echo ""

# =========================
# 构建 UE 启动参数
# =========================
LAUNCH_ARGS=()
if [[ -n "${DEFAULT_MAP:-}" ]]; then
  LAUNCH_ARGS+=("$DEFAULT_MAP")
  echo "Starting with map: $DEFAULT_MAP"
fi

echo "========================================"
echo "  启动 Unreal Editor..."
echo "========================================"
echo ""

# =========================
# 启动 UE 编辑器
# =========================
exec "$UE_EDITOR_BIN" "$PROJECT_PATH" ${LAUNCH_ARGS[@]+"${LAUNCH_ARGS[@]}"} "$@"
