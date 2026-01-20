#!/bin/bash
# MultiAgent-Unreal 项目统一设置脚本
# 自动检测并设置 Python 环境和 Content 资源

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 显示帮助
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help              显示此帮助信息"
    echo "  -a, --all               完整设置（Python + Content）"
    echo "  -p, --python            仅设置 Python 环境"
    echo "  -c, --content           仅设置 Content 资源"
    echo "  -u, --update            更新所有（Python 依赖 + Content）"
    echo "  --update-python         仅更新 Python 依赖"
    echo "  --update-content        仅更新 Content 资源"
    echo "  -s, --status            显示项目状态"
    echo "  -f, --force             强制重新设置"
    echo ""
    echo "示例:"
    echo "  $0                      # 自动检测并设置缺失的部分"
    echo "  $0 --all                # 完整设置"
    echo "  $0 --update             # 更新所有"
    echo "  $0 --status             # 查看状态"
    exit 0
}

# 检查 Python 环境
check_python_env() {
    if [ -d "$PROJECT_ROOT/.venv" ]; then
        return 0
    else
        return 1
    fi
}

# 检查 Content 目录
check_content() {
    if [ -d "$PROJECT_ROOT/unreal_project/Content/.git" ]; then
        return 0
    else
        return 1
    fi
}

# 检查 uv
check_uv() {
    if command -v uv &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# 显示状态
show_status() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}          ${BLUE}MultiAgent-Unreal 项目状态${NC}              ${CYAN}║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    # Python 环境
    echo -e "${BLUE}🐍 Python 环境${NC}"
    if check_uv; then
        UV_VERSION=$(uv --version 2>/dev/null)
        echo -e "  uv: ${GREEN}✓${NC} 已安装 ($UV_VERSION)"
    else
        echo -e "  uv: ${RED}✗${NC} 未安装"
    fi
    
    if check_python_env; then
        PYTHON_VERSION=$(source "$PROJECT_ROOT/.venv/bin/activate" && python --version 2>/dev/null)
        echo -e "  虚拟环境: ${GREEN}✓${NC} 已创建 ($PYTHON_VERSION)"
        
        # 检查依赖
        if [ -f "$PROJECT_ROOT/.venv/bin/python" ]; then
            NUMPY_VERSION=$(source "$PROJECT_ROOT/.venv/bin/activate" && python -c "import numpy; print(numpy.__version__)" 2>/dev/null)
            if [ -n "$NUMPY_VERSION" ]; then
                echo -e "  依赖: ${GREEN}✓${NC} 已安装 (numpy $NUMPY_VERSION)"
            else
                echo -e "  依赖: ${YELLOW}⚠${NC} 部分缺失"
            fi
        fi
    else
        echo -e "  虚拟环境: ${RED}✗${NC} 未创建"
    fi
    
    echo ""
    
    # Content 资源
    echo -e "${BLUE}🎮 Content 资源${NC}"
    if check_content; then
        cd "$PROJECT_ROOT/unreal_project/Content" || exit 1
        COMMIT_COUNT=$(git rev-list --count HEAD 2>/dev/null)
        LATEST_COMMIT=$(git log -1 --format='%h - %s' 2>/dev/null)
        LFS_COUNT=$(git lfs ls-files 2>/dev/null | wc -l)
        CONTENT_SIZE=$(du -sh . 2>/dev/null | cut -f1)
        
        echo -e "  仓库: ${GREEN}✓${NC} 已克隆"
        echo -e "  提交数: $COMMIT_COUNT"
        echo -e "  最新: $LATEST_COMMIT"
        echo -e "  LFS 文件: $LFS_COUNT 个"
        echo -e "  磁盘占用: $CONTENT_SIZE"
        
        # 检查是否有更新
        git fetch origin --quiet 2>/dev/null
        LOCAL=$(git rev-parse @ 2>/dev/null)
        REMOTE=$(git rev-parse @{u} 2>/dev/null)
        
        if [ "$LOCAL" = "$REMOTE" ]; then
            echo -e "  状态: ${GREEN}✓${NC} 最新版本"
        else
            echo -e "  状态: ${YELLOW}⚠${NC} 有新版本可用"
        fi
        
        cd "$PROJECT_ROOT" || exit 1
    else
        echo -e "  仓库: ${RED}✗${NC} 未克隆"
    fi
    
    echo ""
    echo -e "${CYAN}═════════════════════════���══════════════════════════════${NC}"
}

# 设置 Python 环境
setup_python() {
    echo -e "${BLUE}🐍 设置 Python 环境...${NC}"
    echo ""
    
    if check_python_env && [ "$FORCE" != "true" ]; then
        echo -e "${GREEN}✓ Python 环境已存在${NC}"
        return 0
    fi
    
    "$SCRIPT_DIR/setup_python_env.sh"
}

# 设置 Content
setup_content() {
    echo -e "${BLUE}🎮 设置 Content 资源...${NC}"
    echo ""
    
    if check_content && [ "$FORCE" != "true" ]; then
        echo -e "${GREEN}✓ Content 已存在${NC}"
        return 0
    fi
    
    "$SCRIPT_DIR/setup_hf_content.sh"
}

# 更新 Python 依赖
update_python() {
    echo -e "${BLUE}🐍 更新 Python 依赖...${NC}"
    echo ""
    
    if ! check_python_env; then
        echo -e "${RED}错误: Python 环境未创建${NC}"
        echo "请先运行: $0 --python"
        exit 1
    fi
    
    if ! check_uv; then
        export PATH="$HOME/.local/bin:$PATH"
    fi
    
    cd "$PROJECT_ROOT" || exit 1
    uv pip install -r requirements.txt --upgrade
    echo -e "${GREEN}✓ Python 依赖已更新${NC}"
}

# 更新 Content
update_content() {
    echo -e "${BLUE}🎮 更新 Content 资源...${NC}"
    echo ""
    
    if ! check_content; then
        echo -e "${RED}错误: Content 未克隆${NC}"
        echo "请先运行: $0 --content"
        exit 1
    fi
    
    "$SCRIPT_DIR/setup_hf_content.sh" --update
}

# 自动设置（智能检测）
auto_setup() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}       ${GREEN}MultiAgent-Unreal 自动设置${NC}                  ${CYAN}║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    local need_python=false
    local need_content=false
    
    # 检测需要设置的部分
    if ! check_python_env; then
        echo -e "${YELLOW}⚠ 检测到 Python 环境未设置${NC}"
        need_python=true
    else
        echo -e "${GREEN}✓ Python 环境已就绪${NC}"
    fi
    
    if ! check_content; then
        echo -e "${YELLOW}⚠ 检测到 Content 资源未下载${NC}"
        need_content=true
    else
        echo -e "${GREEN}✓ Content 资源已就绪${NC}"
    fi
    
    # 如果都已设置
    if [ "$need_python" = false ] && [ "$need_content" = false ]; then
        echo ""
        echo -e "${GREEN}✅ 所有组件已就绪！${NC}"
        echo ""
        echo "提示:"
        echo "  - 更新: $0 --update"
        echo "  - 状态: $0 --status"
        return 0
    fi
    
    echo ""
    echo -e "${BLUE}开始自动设置...${NC}"
    echo ""
    
    # 设置 Python
    if [ "$need_python" = true ]; then
        setup_python
        echo ""
    fi
    
    # 设置 Content
    if [ "$need_content" = true ]; then
        setup_content
        echo ""
    fi
    
    echo -e "${GREEN}✅ 设置完成！${NC}"
    echo ""
    echo "下一步:"
    echo "  1. 激活 Python 环境: source .venv/bin/activate"
    echo "  2. 编译项目: ./compile.sh"
    echo "  3. 运行 UE5 编辑器"
}

# 解析参数
ACTION="auto"
FORCE="false"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -a|--all)
            ACTION="all"
            shift
            ;;
        -p|--python)
            ACTION="python"
            shift
            ;;
        -c|--content)
            ACTION="content"
            shift
            ;;
        -u|--update)
            ACTION="update-all"
            shift
            ;;
        --update-python)
            ACTION="update-python"
            shift
            ;;
        --update-content)
            ACTION="update-content"
            shift
            ;;
        -s|--status)
            ACTION="status"
            shift
            ;;
        -f|--force)
            FORCE="true"
            shift
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            show_help
            ;;
    esac
done

# 执行操作
cd "$PROJECT_ROOT" || exit 1

case $ACTION in
    auto)
        auto_setup
        ;;
    all)
        setup_python
        echo ""
        setup_content
        ;;
    python)
        setup_python
        ;;
    content)
        setup_content
        ;;
    update-all)
        update_python
        echo ""
        update_content
        ;;
    update-python)
        update_python
        ;;
    update-content)
        update_content
        ;;
    status)
        show_status
        ;;
    *)
        echo -e "${RED}未知操作: $ACTION${NC}"
        show_help
        ;;
esac
