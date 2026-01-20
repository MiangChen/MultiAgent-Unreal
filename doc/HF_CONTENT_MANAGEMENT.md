# Hugging Face Content 管理指南

本项目使用 Hugging Face + Git LFS 管理 UE5 Content 资产。

## 前置要求

```bash
# 安装 Git LFS
sudo apt install git-lfs

# 安装 huggingface_hub
pip3 install --user --break-system-packages huggingface_hub

# 登录 Hugging Face
python3 -c "from huggingface_hub import login; login()"
```

## 团队成员：下载/更新 Content

### 首次克隆

```bash
./scripts/setup_hf_content.sh
```

这会将 `WindyLab/MultiAgent-Content` 克隆到 `unreal_project/Content/`

### 更新到最新版本

```bash
# 默认更新
./scripts/setup_hf_content.sh

# 或明确指定更新
./scripts/setup_hf_content.sh --update

# 强制更新（丢弃本地更改）
./scripts/setup_hf_content.sh --force
```

### 查看仓库状态

```bash
# 查看当前状态和是否有更新
./scripts/setup_hf_content.sh --status
```

### 查看提交历史

```bash
# 查看最近 10 次提交（默认）
./scripts/setup_hf_content.sh --log

# 查看最近 20 次提交
./scripts/setup_hf_content.sh --log 20
```

或者手动：

```bash
cd unreal_project/Content
git log --oneline
git log --stat  # 查看每次提交的文件变更
```

### 回滚到特定版本

```bash
# 使用脚本（交互式）
./scripts/setup_hf_content.sh --rollback
```

或者手动：

```bash
cd unreal_project/Content
git log --oneline  # 找到目标 commit hash
git checkout <commit-hash>
```

### 清理并重新克隆

如果遇到问题，可以清理并重新克隆：

```bash
./scripts/setup_hf_content.sh --clean
```

### 脚本帮助

查看所有可用选项：

```bash
./scripts/setup_hf_content.sh --help
```

## 维护者：上传 Content

### 首次上传（迁移）

```bash
./scripts/upload_content_to_hf.sh
```

这会：
1. 将 Content 目录初始化为 Git 仓库
2. 配置 Git LFS 追踪规则
3. 删除旧的 tar.gz
4. 推送所有文件到 Hugging Face

### 日常更新

修改 Content 后：

```bash
./scripts/upload_content_to_hf.sh
```

或者手动：

```bash
cd unreal_project/Content
git add .
git commit -m "描述你的更改"
git push
```

## Git LFS 追踪的文件类型

- **UE 资产**: `*.uasset`, `*.umap`, `*.uexp`, `*.ubulk`, `*.uplugin`, `*.upk`, `*.pak`
- **纹理**: `*.png`, `*.jpg`, `*.jpeg`, `*.tga`, `*.exr`, `*.hdr`, `*.dds`
- **模型**: `*.fbx`, `*.obj`, `*.blend`
- **音频**: `*.wav`, `*.mp3`, `*.ogg`
- **视频**: `*.mp4`, `*.mov`

## 优势

✅ **增量更新**: 只下载/上传变化的文件  
✅ **版本历史**: 完整的提交历史，可回滚  
✅ **分支管理**: 支持创建分支测试新资产  
✅ **协作友好**: 多人可以同时工作，Git 自动合并  
✅ **节省带宽**: 不需要每次都传输 5GB

## 常见问题

### Q: 如何查看 LFS 文件大小？

```bash
cd unreal_project/Content
git lfs ls-files -s
```

### Q: 如何只下载部分文件？

```bash
# 使用 sparse checkout
git clone --filter=blob:none --sparse https://huggingface.co/datasets/WindyLab/MultiAgent-Content
cd MultiAgent-Content
git sparse-checkout set Map/SimpleCity
```

### Q: 推送失败怎么办？

```bash
# 检查是否有冲突
cd unreal_project/Content
git status

# 拉取最新更改
git pull --rebase

# 解决冲突后
git push
```

### Q: 如何创建分支测试新资产？

```bash
cd unreal_project/Content
git checkout -b feature/new-map
# 修改文件...
git add .
git commit -m "Add new map"
git push -u origin feature/new-map
```

## 仓库信息

- **仓库**: https://huggingface.co/datasets/WindyLab/MultiAgent-Content
- **类型**: Dataset (Git LFS)
- **大小**: ~5GB
