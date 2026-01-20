#!/bin/bash
# 将 Content 目录上传到 Hugging Face (纯 Git LFS 方式)

REPO_ID="WindyLab/MultiAgent-Content"
CONTENT_DIR="unreal_project/Content"

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}=== 上传 Content 到 Hugging Face (Git LFS) ===${NC}"
echo ""

# 检查 Content 目录是否存在
if [ ! -d "$CONTENT_DIR" ]; then
    echo -e "${RED}错误: Content 目录不存在${NC}"
    exit 1
fi

# 检查是否已经是 Git 仓库
if [ -d "$CONTENT_DIR/.git" ]; then
    echo -e "${YELLOW}Content 目录已经是 Git 仓库${NC}"
    cd $CONTENT_DIR
    
    # 显示状态
    echo ""
    git status --short
    echo ""
    
    read -p "提交并推送更改? [y/N]: " confirm
    if [ "$confirm" != "y" ]; then
        echo "已取消"
        exit 0
    fi
    
    # 提交更改
    read -p "输入提交信息: " commit_msg
    if [ -z "$commit_msg" ]; then
        commit_msg="Update content $(date +%Y-%m-%d)"
    fi
    
    git add .
    git commit -m "$commit_msg"
    git push
    
    echo -e "${GREEN}✅ 推送完成！${NC}"
    
else
    echo -e "${YELLOW}首次初始化 Git 仓库并上传...${NC}"
    echo ""
    echo "⚠️  这将："
    echo "  1. 删除现有 HF 仓库中的 tar.gz"
    echo "  2. 将 Content 目录转换为 Git LFS 仓库"
    echo "  3. 推送所有文件到 Hugging Face"
    echo ""
    read -p "确认继续? [y/N]: " confirm
    
    if [ "$confirm" != "y" ]; then
        echo "已取消"
        exit 0
    fi
    
    cd $CONTENT_DIR
    
    # 初始化 Git
    echo -e "${YELLOW}初始化 Git 仓库...${NC}"
    git init
    git lfs install
    
    # 配置 Git LFS 追踪规则
    echo -e "${YELLOW}配置 Git LFS...${NC}"
    
    # UE 资产文件
    git lfs track "*.uasset"
    git lfs track "*.umap"
    git lfs track "*.uexp"
    git lfs track "*.ubulk"
    git lfs track "*.uplugin"
    git lfs track "*.upk"
    git lfs track "*.pak"
    
    # 纹理和图像
    git lfs track "*.png"
    git lfs track "*.jpg"
    git lfs track "*.jpeg"
    git lfs track "*.tga"
    git lfs track "*.exr"
    git lfs track "*.hdr"
    git lfs track "*.dds"
    
    # 3D 模型
    git lfs track "*.fbx"
    git lfs track "*.obj"
    git lfs track "*.blend"
    
    # 音频
    git lfs track "*.wav"
    git lfs track "*.mp3"
    git lfs track "*.ogg"
    
    # 视频
    git lfs track "*.mp4"
    git lfs track "*.mov"
    
    # 添加 .gitattributes
    git add .gitattributes
    
    # 添加远程仓库
    git remote add origin https://huggingface.co/datasets/$REPO_ID
    
    # 拉取现有内容（如 README）
    echo -e "${YELLOW}拉取现有仓库内容...${NC}"
    git pull origin main --allow-unrelated-histories || git pull origin master --allow-unrelated-histories || true
    
    # 删除 tar.gz（如果存在）
    if [ -f "Content.tar.gz" ]; then
        echo -e "${YELLOW}删除旧的 tar.gz 文件...${NC}"
        git rm Content.tar.gz
    fi
    
    # 添加所有文件
    echo -e "${YELLOW}添加文件...${NC}"
    git add .
    
    # 提交
    echo -e "${YELLOW}提交更改...${NC}"
    git commit -m "Migrate to pure Git LFS (remove tar.gz)"
    
    # 推送
    echo -e "${YELLOW}推送到 Hugging Face...${NC}"
    git push -u origin main || git push -u origin master
    
    echo ""
    echo -e "${GREEN}✅ 完成！Content 目录现在是一个 Git LFS 仓库${NC}"
    echo ""
    echo "后续更新只需："
    echo "  cd $CONTENT_DIR"
    echo "  git add ."
    echo "  git commit -m 'Update message'"
    echo "  git push"
    echo ""
    echo "或者运行: ./scripts/upload_content_to_hf.sh"
fi
