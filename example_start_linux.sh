#!/bin/bash

# 设置引擎根目录变量
export UE5_ROOT="/path_to_your_ue/UnrealEngine" # 换成你自己的目录

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Unreal 项目路径
PROJECT_PATH="$SCRIPT_DIR/unreal_project/MultiAgent.uproject"

# 启动 UE5 编辑器并打开项目
"$UE5_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_PATH" "$@" -ResX=1280 -ResY=720