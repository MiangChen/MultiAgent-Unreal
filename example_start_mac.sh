#!/bin/bash

# 设置引擎根目录变量
export UE5_ROOT="/path_to_your_ue/UnrealEngine"

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Unreal 项目路径
PROJECT_PATH="$SCRIPT_DIR/unreal_project/MultiAgent.uproject"

# 配置文件路径
CONFIG_PATH="$SCRIPT_DIR/config/simulation.json"

# 默认地图
DEFAULT_MAP=""

# 从 SimConfig.json 读取地图路径
if [ -f "$CONFIG_PATH" ]; then
    # 使用 jq 解析 JSON (如果安装了)，否则用 grep/sed
    if command -v jq &> /dev/null; then
        DEFAULT_MAP=$(jq -r '.DefaultMap // empty' "$CONFIG_PATH")
    else
        # 简单的 grep 方式提取
        DEFAULT_MAP=$(grep -o '"DefaultMap"[[:space:]]*:[[:space:]]*"[^"]*"' "$CONFIG_PATH" | sed 's/.*: *"\([^"]*\)"/\1/')
    fi
fi

# 构建启动参数
LAUNCH_ARGS=""
if [ -n "$DEFAULT_MAP" ]; then
    LAUNCH_ARGS="$DEFAULT_MAP"
    echo "Starting with map: $DEFAULT_MAP"
fi

# 启动 UE5 编辑器并打开项目
"$UE5_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$PROJECT_PATH" $LAUNCH_ARGS "$@"