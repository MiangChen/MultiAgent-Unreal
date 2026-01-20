#!/bin/bash
# Hugging Face Content 管理脚本 (Git LFS 方式)
# 支持首次克隆、增量更新、版本回滚等功能

REPO_ID="WindyLab/MultiAgent-Content"
CONTENT_DIR="unreal_project/Content"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -u, --update        更新到最新版本（默认）"
    echo "  -s, --status        显示仓库状态"
    echo "  -l, --log [N]       显示最近 N 次提交（默认 10）"
    echo "  -r, --rollback      回滚到指定提交"
    echo "  -c, --clean         清理并重新克隆"
    echo "  -f, --force         强制更新（丢弃本地更改）"
    echo ""
    echo "示例:"
    echo "  $0                  # 首次克隆或更新"
    echo "  $0 --status         # 查看状态"
    echo "  $0 --log 20         # 查看最近 20 次提交"
    echo "  $0 --force          # 强制更新"
    exit 0
}

echo -e "${GREEN}=== Hugging Face Content 管理工具 (Git LFS) ===${NC}"
echo ""

# 检查 Git LFS
if ! command -v git-lfs &> /dev/null; then
    echo -e "${RED}错误: 未安装 Git LFS${NC}"
    echo "请运行: sudo apt install git-lfs"
    exit 1
fi

# 进入项目根目录
cd "$PROJECT_ROOT" || exit 1

# 解析命令行参数
ACTION="update"
FORCE=false
LOG_COUNT=10

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -u|--update)
            ACTION="update"
            shift
            ;;
        -s|--status)
            ACTION="status"
            shift
            ;;
        -l|--log)
            ACTION="log"
            if [[ -n $2 && $2 =~ ^[0-9]+$ ]]; then
                LOG_COUNT=$2
                shift
            fi
            shift
            ;;
        -r|--rollback)
            ACTION="rollback"
            shift
            ;;
        -c|--clean)
            ACTION="clean"
            shift
            ;;
        -f|--force)
            FORCE=true
            shift
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            show_help
            ;;
    esac
done

# 显示仓库状态
show_status() {
    if [ ! -d "$CONTENT_DIR/.git" ]; then
        echo -e "${YELLOW}Content 目录尚未初始化${NC}"
        return 1
    fi
    
    cd "$CONTENT_DIR" || exit 1
    
    echo -e "${BLUE}=== 仓库状态 ===${NC}"
    echo "路径: $(pwd)"
    echo "分支: $(git branch --show-current)"
    echo "远程: $(git remote get-url origin)"
    echo ""
    
    # 检查是否有未提交的更改
    if ! git diff-index --quiet HEAD -- 2>/dev/null; then
        echo -e "${YELLOW}未提交的更改:${NC}"
        git status --short
    else
        echo -e "${GREEN}工作目录干净${NC}"
    fi
    
    echo ""
    echo "提交数: $(git rev-list --count HEAD)"
    echo "最新提交: $(git log -1 --format='%h - %s (%cr)')"
    echo "LFS 文件: $(git lfs ls-files 2>/dev/null | wc -l) 个"
    
    # 检查是否有远程更新
    echo ""
    echo -e "${YELLOW}检查远程更新...${NC}"
    git fetch origin --quiet
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})
    
    if [ "$LOCAL" = "$REMOTE" ]; then
        echo -e "${GREEN}✅ 已是最新版本${NC}"
    else
        echo -e "${YELLOW}⚠️  有新版本可用，运行 '$0 --update' 更新${NC}"
    fi
}

# 显示提交历史
show_log() {
    if [ ! -d "$CONTENT_DIR/.git" ]; then
        echo -e "${RED}Content 目录尚未初始化${NC}"
        exit 1
    fi
    
    cd "$CONTENT_DIR" || exit 1
    
    echo -e "${BLUE}=== 最近 $LOG_COUNT 次提交 ===${NC}"
    git log --oneline --graph --decorate -n "$LOG_COUNT"
}

# 回滚到指定版本
rollback_version() {
    if [ ! -d "$CONTENT_DIR/.git" ]; then
        echo -e "${RED}Content 目录尚未初始化${NC}"
        exit 1
    fi
    
    cd "$CONTENT_DIR" || exit 1
    
    echo -e "${BLUE}=== 版本回滚 ===${NC}"
    echo "最近的提交:"
    git log --oneline -n 10
    echo ""
    read -p "请输入要回滚到的提交 hash: " commit_hash
    
    if [ -z "$commit_hash" ]; then
        echo -e "${RED}已取消${NC}"
        exit 0
    fi
    
    echo ""
    echo -e "${YELLOW}将回滚到: $commit_hash${NC}"
    read -p "确认继续? [y/N]: " confirm
    
    if [ "$confirm" != "y" ]; then
        echo -e "${RED}已取消${NC}"
        exit 0
    fi
    
    git checkout "$commit_hash"
    echo -e "${GREEN}✅ 已回滚到 $commit_hash${NC}"
}

# 清理并重新克隆
clean_and_clone() {
    echo -e "${YELLOW}⚠️  警告: 这将删除现有的 Content 目录并重新克隆${NC}"
    read -p "确认继续? [y/N]: " confirm
    
    if [ "$confirm" != "y" ]; then
        echo -e "${RED}已取消${NC}"
        exit 0
    fi
    
    if [ -d "$CONTENT_DIR" ]; then
        echo -e "${YELLOW}删除现有目录...${NC}"
        rm -rf "$CONTENT_DIR"
    fi
    
    ACTION="update"
}

# 执行更新
do_update() {
    # 检查 Content 目录是否已经是 Git 仓库
    if [ -d "$CONTENT_DIR/.git" ]; then
        echo -e "${YELLOW}检测到现有 Git 仓库，执行更新...${NC}"
        cd "$CONTENT_DIR" || exit 1
        
        # 显示当前状态
        echo ""
        echo "当前分支: $(git branch --show-current)"
        echo "远程仓库: $(git remote get-url origin)"
        echo ""
        
        # 检查是否有未提交的更改
        if ! git diff-index --quiet HEAD -- 2>/dev/null; then
            if [ "$FORCE" = true ]; then
                echo -e "${YELLOW}强制更新: 丢弃本地更改${NC}"
                git reset --hard HEAD
                git clean -fd
            else
                echo -e "${YELLOW}警告: 检测到未提交的更改${NC}"
                git status --short
                echo ""
                read -p "是否暂存这些更改? [y/N]: " stash_choice
                if [ "$stash_choice" = "y" ] || [ "$stash_choice" = "Y" ]; then
                    git stash push -m "Auto stash before pull $(date +%Y%m%d_%H%M%S)"
                    echo -e "${GREEN}已暂存更改${NC}"
                else
                    echo -e "${RED}请先处理未提交的更改，或使用 --force 强制更新${NC}"
                    exit 1
                fi
            fi
        fi
        
        # 拉取最新更改
        echo -e "${YELLOW}拉取最新更改...${NC}"
        
        # 获取更新前的提交
        OLD_COMMIT=$(git rev-parse HEAD)
        
        if git pull origin main; then
            NEW_COMMIT=$(git rev-parse HEAD)
            
            if [ "$OLD_COMMIT" = "$NEW_COMMIT" ]; then
                echo -e "${GREEN}✅ 已是最新版本${NC}"
            else
                echo ""
                echo -e "${GREEN}✅ 更新完成！${NC}"
                echo ""
                echo -e "${BLUE}更新内容:${NC}"
                git log --oneline "$OLD_COMMIT".."$NEW_COMMIT"
            fi
        else
            echo -e "${RED}更新失败${NC}"
            exit 1
        fi
        
    else
        echo -e "${YELLOW}首次克隆仓库...${NC}"
        
        # 确保父目录存在
        mkdir -p "$(dirname "$CONTENT_DIR")"
        
        # 克隆仓库
        echo "克隆地址: https://huggingface.co/datasets/$REPO_ID"
        if git clone https://huggingface.co/datasets/$REPO_ID "$CONTENT_DIR"; then
            cd "$CONTENT_DIR" || exit 1
            
            # 配置 Git LFS
            git lfs install
            
            echo ""
            echo -e "${GREEN}✅ 克隆完成！${NC}"
            echo ""
            echo "Content 目录现在是一个 Git 仓库，可以："
            echo "  - 更新: $0 --update"
            echo "  - 查看状态: $0 --status"
            echo "  - 查看历史: $0 --log"
            echo "  - 手动拉取: cd $CONTENT_DIR && git pull"
        else
            echo -e "${RED}克隆失败${NC}"
            echo ""
            echo "可能的原因:"
            echo "  1. 网络连接问题"
            echo "  2. 需要 Hugging Face 认证（私有仓库）"
            echo ""
            echo "如果是私有仓库，请先登录:"
            echo "  pip install huggingface_hub"
            echo "  python3 -c \"from huggingface_hub import login; login()\""
            exit 1
        fi
    fi
    
    echo ""
    echo -e "${BLUE}=== 仓库信息 ===${NC}"
    cd "$CONTENT_DIR" || exit 1
    echo "路径: $(pwd)"
    echo "提交数: $(git rev-list --count HEAD)"
    echo "最新提交: $(git log -1 --format='%h - %s (%cr)')"
    echo "LFS 文件: $(git lfs ls-files 2>/dev/null | wc -l) 个"
    
    # 显示磁盘使用
    CONTENT_SIZE=$(du -sh . 2>/dev/null | cut -f1)
    echo "磁盘占用: $CONTENT_SIZE"
}

# 执行相应的操作
case $ACTION in
    update)
        do_update
        ;;
    status)
        show_status
        ;;
    log)
        show_log
        ;;
    rollback)
        rollback_version
        ;;
    clean)
        clean_and_clone
        do_update
        ;;
    *)
        echo -e "${RED}未知操作: $ACTION${NC}"
        show_help
        ;;
esac