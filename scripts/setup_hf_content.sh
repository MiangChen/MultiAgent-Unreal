#!/bin/bash
# Hugging Face Content 管理脚本 (纯 Git LFS 方式)

REPO_ID="WindyLab/MultiAgent-Content"
CONTENT_DIR="unreal_project/Content"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}=== Hugging Face Content 管理工具 (Git LFS) ===${NC}"
echo ""

# 检查 Git LFS
if ! command -v git-lfs &> /dev/null; then
    echo -e "${RED}错误: 未安装 Git LFS${NC}"
    echo "请运行: sudo apt install git-lfs"
    exit 1
fi

# 检查 Content 目录是否已经是 Git 仓库
if [ -d "$CONTENT_DIR/.git" ]; then
    echo -e "${YELLOW}检测到现有 Git 仓库，执行更新...${NC}"
    cd $CONTENT_DIR
    
    # 显示当前状态
    echo ""
    echo "当前分支: $(git branch --show-current)"
    echo "远程仓库: $(git remote get-url origin)"
    echo ""
    
    # 检查是否有未提交的更改
    if ! git diff-index --quiet HEAD --; then
        echo -e "${YELLOW}警告: 检测到未提交的更改${NC}"
        git status --short
        echo ""
        read -p "是否暂存这些更改? [y/N]: " stash_choice
        if [ "$stash_choice" = "y" ]; then
            git stash push -m "Auto stash before pull $(date +%Y%m%d_%H%M%S)"
            echo -e "${GREEN}已暂存更改${NC}"
        fi
    fi
    
    # 拉取最新更改
    echo -e "${YELLOW}拉取最新更改...${NC}"
    git pull
    
    echo -e "${GREEN}✅ 更新完成！${NC}"
    
else
    echo -e "${YELLOW}首次克隆仓库...${NC}"
    
    # 确保父目录存在
    mkdir -p $(dirname $CONTENT_DIR)
    
    # 克隆仓库
    git clone https://huggingface.co/datasets/$REPO_ID $CONTENT_DIR
    
    cd $CONTENT_DIR
    
    # 配置 Git LFS
    git lfs install
    
    echo ""
    echo -e "${GREEN}✅ 克隆完成！${NC}"
    echo ""
    echo "Content 目录现在是一个 Git 仓库，可以："
    echo "  - 运行此脚本更新: ./scripts/setup_hf_content.sh"
    echo "  - 手动拉取: cd $CONTENT_DIR && git pull"
    echo "  - 查看历史: cd $CONTENT_DIR && git log"
fi

echo ""
echo "仓库信息:"
cd $CONTENT_DIR
echo "  提交数: $(git rev-list --count HEAD)"
echo "  最新提交: $(git log -1 --format='%h - %s (%cr)')"
echo "  LFS 文件: $(git lfs ls-files | wc -l) 个"
