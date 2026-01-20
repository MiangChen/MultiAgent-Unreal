#!/bin/bash
# 激活 Python 虚拟环境

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VENV_DIR="$PROJECT_ROOT/.venv"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [ ! -d "$VENV_DIR" ]; then
    echo -e "${RED}错误: 虚拟环境不存在${NC}"
    echo ""
    echo "请先创建虚拟环境:"
    echo "  uv venv"
    echo "  uv pip install -r requirements.txt"
    echo ""
    echo "或运行快速设置:"
    echo "  ./scripts/setup_python_env.sh"
    exit 1
fi

echo -e "${GREEN}激活 Python 虚拟环境...${NC}"
source "$VENV_DIR/bin/activate"

echo -e "${GREEN}✅ 虚拟环境已激活${NC}"
echo ""
echo "Python 版本: $(python --version)"
echo "虚拟环境: $VENV_DIR"
echo ""
echo "退出虚拟环境: deactivate"
