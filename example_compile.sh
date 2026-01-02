#!/bin/bash
# MultiAgent 项目一键编译脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 路径配置
UE_ROOT="/home/zhugb/Software/UnrealEngine"
PROJECT_ROOT="/home/zhugb/Documents/unreal_projects/MultiAgent-Unreal"
PROJECT_FILE="${PROJECT_ROOT}/unreal_project/MultiAgent.uproject"
BUILD_SCRIPT="${UE_ROOT}/Engine/Build/BatchFiles/Linux/Build.sh"

# 默认配置
TARGET="MultiAgentEditor"
PLATFORM="Linux"
CONFIG="Development"

# 帮助信息
show_help() {
    echo "用法: ./compile.sh [选项]"
    echo ""
    echo "选项:"
    echo "  -c, --config <配置>    编译配置: Debug, Development, Shipping (默认: Development)"
    echo "  -r, --rebuild          完全重新编译"
    echo "  -g, --game             只编译游戏目标 (不含编辑器)"
    echo "  -h, --help             显示帮助信息"
    echo ""
    echo "示例:"
    echo "  ./compile.sh                    # 默认编译"
    echo "  ./compile.sh -c Debug           # Debug 配置编译"
    echo "  ./compile.sh -r                 # 完全重新编译"
    echo "  ./compile.sh -g -c Shipping     # 编译发布版游戏"
}

# 解析参数
REBUILD=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--config)
            CONFIG="$2"
            shift 2
            ;;
        -r|--rebuild)
            REBUILD=true
            shift
            ;;
        -g|--game)
            TARGET="MultiAgent"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 验证配置
if [[ ! "$CONFIG" =~ ^(Debug|Development|Shipping)$ ]]; then
    echo -e "${RED}错误: 无效的配置 '$CONFIG'${NC}"
    echo "有效配置: Debug, Development, Shipping"
    exit 1
fi

# 检查路径
if [ ! -f "$BUILD_SCRIPT" ]; then
    echo -e "${RED}错误: 找不到 UE5 编译脚本: $BUILD_SCRIPT${NC}"
    exit 1
fi

if [ ! -f "$PROJECT_FILE" ]; then
    echo -e "${RED}错误: 找不到项目文件: $PROJECT_FILE${NC}"
    exit 1
fi

# 完全重新编译
if [ "$REBUILD" = true ]; then
    echo -e "${YELLOW}清理中间文件...${NC}"
    rm -rf "${PROJECT_ROOT}/unreal_project/Intermediate"
    rm -rf "${PROJECT_ROOT}/unreal_project/Binaries"
fi

# 开始编译
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  MultiAgent 编译${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "目标: ${YELLOW}${TARGET}${NC}"
echo -e "平台: ${YELLOW}${PLATFORM}${NC}"
echo -e "配置: ${YELLOW}${CONFIG}${NC}"
echo ""

START_TIME=$(date +%s)

"$BUILD_SCRIPT" "$TARGET" "$PLATFORM" "$CONFIG" \
    -Project="$PROJECT_FILE" \
    -WaitMutex

EXIT_CODE=$?
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  编译成功! (耗时: ${DURATION}秒)${NC}"
    echo -e "${GREEN}========================================${NC}"
else
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}  编译失败! (退出码: ${EXIT_CODE})${NC}"
    echo -e "${RED}========================================${NC}"
fi

exit $EXIT_CODE