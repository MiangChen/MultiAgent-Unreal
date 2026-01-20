#!/bin/bash
# 快速设置 Python 环境

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}=== Python 环境设置 (uv) ===${NC}"
echo ""

# 检查 uv 是否安装
if ! command -v uv &> /dev/null; then
    echo -e "${YELLOW}uv 未安装，正在安装...${NC}"
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$PATH"
    echo ""
fi

cd "$PROJECT_ROOT" || exit 1

# 检查虚拟环境是否存在
if [ -d ".venv" ]; then
    echo -e "${YELLOW}虚拟环境已存在${NC}"
    read -p "是否重新创建? [y/N]: " recreate
    if [ "$recreate" = "y" ] || [ "$recreate" = "Y" ]; then
        echo -e "${YELLOW}删除现有虚拟环境...${NC}"
        rm -rf .venv
    else
        echo -e "${BLUE}更新依赖...${NC}"
        uv pip install -r requirements.txt
        echo -e "${GREEN}✅ 依赖已更新${NC}"
        exit 0
    fi
fi

# 创建虚拟环境
echo -e "${BLUE}创建虚拟环境...${NC}"
uv venv

# 安装依赖
echo ""
echo -e "${BLUE}安装依赖...${NC}"
uv pip install -r requirements.txt

echo ""
echo -e "${GREEN}✅ Python 环境设置完成！${NC}"
echo ""
echo "激活虚拟环境:"
echo "  source .venv/bin/activate"
echo ""
echo "或使用快捷脚本:"
echo "  source scripts/activate_env.sh"
echo ""
echo "退出虚拟环境:"
echo "  deactivate"
