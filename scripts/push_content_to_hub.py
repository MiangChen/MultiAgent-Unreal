#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
推送 UE5 Content 资源到 HuggingFace Hub

将本地 UE5 项目的 Content 目录或压缩包推送到 HuggingFace Hub Dataset 仓库。
"""

import argparse
import os
import sys
from pathlib import Path
from typing import Optional

try:
    from huggingface_hub import HfApi, upload_folder, upload_file, get_token
    from huggingface_hub.utils import HfHubHTTPError
    HF_HUB_AVAILABLE = True
    # 检查是否支持 upload_large_folder
    try:
        from huggingface_hub import upload_large_folder
        HAS_LARGE_FOLDER = True
    except ImportError:
        HAS_LARGE_FOLDER = False
except ImportError:
    HF_HUB_AVAILABLE = False
    HAS_LARGE_FOLDER = False
    print("警告: huggingface_hub 未安装，请运行: pip install huggingface_hub")

# 默认路径
CONTENT_DIR_DEFAULT = os.path.expanduser("~/Material/UE5_Projects/MultiAgent_Content")
ARCHIVE_DEFAULT = os.path.expanduser("~/Material/UE5_Projects/MultiAgent_Content.tar.gz")
DEFAULT_REPO_ID = "WindyLab/MultiAgent-Content"


def get_content_stats(content_dir: Path) -> dict:
    """获取 Content 目录统计信息"""
    stats = {
        "total_files": 0,
        "total_size_mb": 0,
        "uasset_count": 0,
        "umap_count": 0,
        "subdirs": [],
    }
    
    if not content_dir.exists():
        return stats
    
    # 统计子目录
    stats["subdirs"] = [d.name for d in content_dir.iterdir() if d.is_dir()]
    
    # 统计文件
    total_size = 0
    for file_path in content_dir.rglob("*"):
        if file_path.is_file():
            stats["total_files"] += 1
            total_size += file_path.stat().st_size
            
            if file_path.suffix == ".uasset":
                stats["uasset_count"] += 1
            elif file_path.suffix == ".umap":
                stats["umap_count"] += 1
    
    stats["total_size_mb"] = round(total_size / (1024 * 1024), 2)
    
    return stats


def generate_readme_for_archive(archive_path: Path) -> str:
    """生成压缩包模式的 README.md 内容"""
    file_size_mb = round(archive_path.stat().st_size / (1024 * 1024), 2)
    archive_name = archive_path.name
    
    return f"""---
license: mit
tags:
- unreal-engine
- ue5
- game-assets
- robotics
- multi-agent
- simulation
size_categories:
- 1G<n<10G
---

# MultiAgent UE5 Content

UE5 项目资源文件，用于多机器人仿真系统 [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal)。

## 资源信息

- **压缩包**: {archive_name}
- **大小**: {file_size_mb} MB

## 下载方法

### 方法 1: 使用 huggingface-cli 下载

```bash
# 安装 huggingface_hub
pip install huggingface_hub

# 下载压缩包
huggingface-cli download WindyLab/MultiAgent-Content {archive_name} --repo-type dataset --local-dir .
```

### 方法 2: 使用 Python 下载

```python
from huggingface_hub import hf_hub_download

hf_hub_download(
    repo_id="WindyLab/MultiAgent-Content",
    filename="{archive_name}",
    repo_type="dataset",
    local_dir="."
)
```

## 解压

```bash
tar -xzvf {archive_name}
```

## 配置软链接

解压后，在 UE5 项目中创建软链接：

```bash
# 假设解压到 ~/Material/UE5_Projects/MultiAgent_Content
ln -s ~/Material/UE5_Projects/MultiAgent_Content unreal_project/Content
```

## 相关项目

- [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal) - 多机器人仿真系统

## License

MIT License
"""


def generate_readme(content_dir: Path, stats: dict) -> str:
    """生成 README.md 内容"""
    return f"""---
license: mit
tags:
- unreal-engine
- ue5
- game-assets
- robotics
- multi-agent
- simulation
size_categories:
- 1G<n<10G
---

# MultiAgent UE5 Content

UE5 项目资源文件，用于多机器人仿真系统 [MultiAgent-Unreal](https://github.com/MiangChen/MultiAgent-Unreal)。

## 资源统计

- **总文件数**: {stats['total_files']}
- **总大小**: {stats['total_size_mb']} MB
- **UAsset 文件**: {stats['uasset_count']}
- **UMap 文件**: {stats['umap_count']}

## 目录结构

```
Content/
{chr(10).join(f"├── {d}/" for d in stats['subdirs'])}
```

## 使用方法

### 方法 1: 使用 huggingface-cli 下载

```bash
# 安装 huggingface_hub
pip install huggingface_hub

# 下载到指定目录
huggingface-cli download WINDY-Lab/MultiAgent-Content --repo-type dataset --local-dir ./Content
```

### 方法 2: 使用 Python 下载

```python
from huggingface_hub import snapshot_download

snapshot_download(
    repo_id="WINDY-Lab/MultiAgent-Content",
    repo_type="dataset",
    local_dir="./Content"
)
```

### 方法 3: 使用 Git LFS

```bash
git lfs install
git clone https://huggingface.co/datasets/WINDY-Lab/MultiAgent-Content Content
```

## 配置软链接

下载后，在 UE5 项目中创建软链接：

```bash
# 假设下载到 ~/Material/UE5_Projects/MultiAgent_Content
ln -s ~/Material/UE5_Projects/MultiAgent_Content unreal_project/Content
```

## 相关项目

- [MultiAgent-Unreal](https://github.com/MiangChen/MultiAgent-Unreal) - 多机器人仿真系统

## License

MIT License
"""


def push_archive(
    archive_path: str,
    repo_id: str,
    token: Optional[str] = None,
    private: bool = False,
    commit_message: Optional[str] = None,
    create_repo: bool = True,
    clear_repo: bool = False,
) -> str:
    """
    推送压缩包到 HuggingFace Hub
    
    Args:
        archive_path: 压缩包路径
        repo_id: HuggingFace 仓库ID
        token: HuggingFace 访问令牌
        private: 是否私有仓库
        commit_message: 提交消息
        create_repo: 如果仓库不存在是否创建
        clear_repo: 是否清空仓库现有内容
    
    Returns:
        仓库ID
    """
    if not HF_HUB_AVAILABLE:
        raise ImportError("huggingface_hub 未安装。请运行: pip install huggingface_hub")
    
    archive = Path(archive_path)
    if not archive.exists():
        raise FileNotFoundError(f"压缩包不存在: {archive}")
    
    file_size_mb = round(archive.stat().st_size / (1024 * 1024), 2)
    print(f"📦 压缩包信息:")
    print(f"   文件: {archive.name}")
    print(f"   大小: {file_size_mb} MB")
    
    # 创建 HfApi 实例
    api = HfApi(token=token)
    
    # 检查仓库是否存在
    repo_exists = False
    try:
        repo_info = api.repo_info(repo_id=repo_id, repo_type="dataset", token=token)
        repo_exists = True
        print(f"ℹ️  仓库已存在: {repo_id}")
    except Exception:
        print(f"ℹ️  仓库不存在: {repo_id}")
    
    # 如果仓库不存在，创建它
    if not repo_exists:
        if create_repo:
            try:
                api.create_repo(
                    repo_id=repo_id,
                    repo_type="dataset",
                    private=private,
                    exist_ok=True,
                )
                print(f"✅ 仓库已创建: {repo_id}")
            except Exception as e:
                raise ValueError(f"无法创建仓库 {repo_id}: {e}")
        else:
            raise ValueError(f"仓库 {repo_id} 不存在，请先创建或使用 --create-repo")
    
    # 清空仓库现有内容
    if clear_repo and repo_exists:
        print(f"🗑️  正在清空仓库现有内容...")
        try:
            # 获取仓库中的所有文件
            files = api.list_repo_files(repo_id=repo_id, repo_type="dataset")
            if files:
                # 删除所有文件（除了 .gitattributes）
                files_to_delete = [f for f in files if f != ".gitattributes"]
                if files_to_delete:
                    operations = [
                        {"path": f, "type": "delete"} for f in files_to_delete
                    ]
                    from huggingface_hub import CommitOperationDelete
                    delete_ops = [CommitOperationDelete(path_in_repo=f) for f in files_to_delete]
                    api.create_commit(
                        repo_id=repo_id,
                        repo_type="dataset",
                        operations=delete_ops,
                        commit_message="Clear repository for new upload",
                    )
                    print(f"   已删除 {len(files_to_delete)} 个文件")
        except Exception as e:
            print(f"⚠️  清空仓库失败: {e}")
            print(f"   继续上传...")
    
    # 生成 README.md
    readme_content = generate_readme_for_archive(archive)
    readme_path = archive.parent / "README_temp.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(readme_content)
    
    # 推送到 Hub
    if commit_message is None:
        commit_message = f"Upload {archive.name}"
    
    print(f"📤 正在上传压缩包到 Hub: {repo_id} ...")
    print(f"   这可能需要较长时间，请耐心等待...")
    
    try:
        # 先上传 README
        api.upload_file(
            path_or_fileobj=str(readme_path),
            path_in_repo="README.md",
            repo_id=repo_id,
            repo_type="dataset",
            commit_message="Update README.md",
        )
        print(f"✅ README.md 已上传")
        
        # 上传压缩包
        api.upload_file(
            path_or_fileobj=str(archive),
            path_in_repo=archive.name,
            repo_id=repo_id,
            repo_type="dataset",
            commit_message=commit_message,
        )
        
        # 清理临时文件
        readme_path.unlink(missing_ok=True)
        
        print(f"✅ 上传成功: https://huggingface.co/datasets/{repo_id}")
        return repo_id
        
    except HfHubHTTPError as e:
        readme_path.unlink(missing_ok=True)
        print(f"❌ 上传失败: {e}")
        if "403" in str(e) or "Forbidden" in str(e):
            print(f"   提示: 请检查 token 权限，确保有写入权限")
        raise
    except Exception as e:
        readme_path.unlink(missing_ok=True)
        print(f"❌ 上传失败: {e}")
        raise


def push_content(
    content_dir: str,
    repo_id: str,
    token: Optional[str] = None,
    version: str = "1.0.0",
    private: bool = False,
    commit_message: Optional[str] = None,
    create_repo: bool = True,
    force: bool = False,
) -> str:
    """
    推送 Content 目录到 HuggingFace Hub
    
    Args:
        content_dir: Content 目录路径
        repo_id: HuggingFace 仓库ID
        token: HuggingFace 访问令牌
        version: 版本号
        private: 是否私有仓库
        commit_message: 提交消息
        create_repo: 如果仓库不存在是否创建
        force: 强制推送（删除远程多余文件）
    
    Returns:
        仓库ID
    """
    if not HF_HUB_AVAILABLE:
        raise ImportError("huggingface_hub 未安装。请运行: pip install huggingface_hub")
    
    content_path = Path(content_dir)
    if not content_path.exists():
        raise FileNotFoundError(f"Content 目录不存在: {content_path}")
    
    # 获取统计信息
    stats = get_content_stats(content_path)
    print(f"📊 Content 统计:")
    print(f"   目录: {', '.join(stats['subdirs']) if stats['subdirs'] else '无'}")
    print(f"   文件数: {stats['total_files']}")
    print(f"   大小: {stats['total_size_mb']} MB")
    print(f"   UAsset: {stats['uasset_count']}, UMap: {stats['umap_count']}")
    
    # 创建 HfApi 实例
    api = HfApi(token=token)
    
    # 检查仓库是否存在
    repo_exists = False
    try:
        repo_info = api.repo_info(repo_id=repo_id, repo_type="dataset", token=token)
        repo_exists = True
        print(f"ℹ️  仓库已存在: {repo_id}")
    except Exception as e:
        repo_exists = False
        print(f"ℹ️  仓库不存在或无法访问: {repo_id}")
    
    # 如果仓库不存在，创建它
    if not repo_exists:
        if create_repo:
            try:
                api.create_repo(
                    repo_id=repo_id,
                    repo_type="dataset",
                    private=private,
                    exist_ok=True,  # 改为 True，如果已存在则忽略
                )
                print(f"✅ 仓库已创建: {repo_id}")
            except Exception as e:
                raise ValueError(f"无法创建仓库 {repo_id}: {e}")
        else:
            raise ValueError(f"仓库 {repo_id} 不存在，请先创建或使用 --create-repo")
    
    # 生成 README.md（如果不存在）
    readme_file = content_path / "README.md"
    if not readme_file.exists():
        readme_content = generate_readme(content_path, stats)
        with open(readme_file, "w", encoding="utf-8") as f:
            f.write(readme_content)
        print(f"✅ 已创建 README.md")
    
    # 推送到 Hub
    if commit_message is None:
        commit_message = f"Update UE5 Content v{version}" if repo_exists else f"Upload UE5 Content v{version}"
    
    print(f"📤 正在推送到 Hub: {repo_id} ...")
    print(f"   提交消息: {commit_message}")
    print(f"   这可能需要较长时间，请耐心等待...")
    
    delete_patterns = "*" if force else None
    
    try:
        # 对于大文件夹，使用 upload_large_folder（支持断点续传）
        if HAS_LARGE_FOLDER and stats["total_size_mb"] > 1000:  # 大于 1GB 使用大文件上传
            print(f"   使用大文件上传模式 (upload_large_folder)...")
            # upload_large_folder 使用 HfApi 实例的 token
            api.upload_large_folder(
                folder_path=str(content_path),
                repo_id=repo_id,
                repo_type="dataset",
                ignore_patterns=[
                    ".git",
                    ".git/**",
                    "__pycache__",
                    "*.pyc",
                    ".DS_Store",
                    "Thumbs.db",
                ],
            )
        else:
            upload_folder(
                folder_path=str(content_path),
                repo_id=repo_id,
                repo_type="dataset",
                token=token,
                commit_message=commit_message,
                ignore_patterns=[
                    ".git",
                    ".git/**",
                    "__pycache__",
                    "*.pyc",
                    ".DS_Store",
                    "Thumbs.db",
                ],
                delete_patterns=delete_patterns,
            )
        
        print(f"✅ 推送成功: https://huggingface.co/datasets/{repo_id}")
        return repo_id
        
    except HfHubHTTPError as e:
        print(f"❌ 推送失败: {e}")
        if "403" in str(e) or "Forbidden" in str(e):
            print(f"   提示: 请检查 token 权限，确保有写入权限")
        raise
    except Exception as e:
        print(f"❌ 推送失败: {e}")
        raise


def main():
    parser = argparse.ArgumentParser(
        description="推送 UE5 Content 资源到 HuggingFace Hub",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 上传压缩包（推荐）
  python push_content_to_hub.py --archive ~/Material/UE5_Projects/MultiAgent_Content.tar.gz

  # 上传压缩包并清空仓库现有内容
  python push_content_to_hub.py --archive ~/Material/UE5_Projects/MultiAgent_Content.tar.gz --clear

  # 使用默认路径上传文件夹
  python push_content_to_hub.py

  # 指定 Content 目录
  python push_content_to_hub.py --content-dir ~/Material/UE5_Projects/MultiAgent_Content

  # 指定仓库ID
  python push_content_to_hub.py --repo-id WindyLab/MultiAgent-Content

  # 强制同步（删除远程多余文件）
  python push_content_to_hub.py --force
""",
    )
    
    parser.add_argument(
        "--archive",
        type=str,
        default=None,
        help=f"压缩包路径（如 .tar.gz, .zip）。指定此参数时上传压缩包而非文件夹",
    )
    parser.add_argument(
        "--content-dir",
        type=str,
        default=CONTENT_DIR_DEFAULT,
        help=f"Content 目录路径（默认: {CONTENT_DIR_DEFAULT}）",
    )
    parser.add_argument(
        "--repo-id",
        type=str,
        default=DEFAULT_REPO_ID,
        help=f"HuggingFace 仓库ID（默认: {DEFAULT_REPO_ID}）",
    )
    parser.add_argument(
        "--token",
        type=str,
        default=None,
        help="HuggingFace 访问令牌（优先使用环境变量 HF_TOKEN）",
    )
    parser.add_argument(
        "--version",
        type=str,
        default="1.0.0",
        help="版本号（默认: 1.0.0）",
    )
    parser.add_argument(
        "--private",
        action="store_true",
        help="创建私有仓库",
    )
    parser.add_argument(
        "--commit-message",
        type=str,
        default=None,
        help="提交消息",
    )
    parser.add_argument(
        "--no-create-repo",
        action="store_true",
        help="如果仓库不存在，不自动创建",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="强制同步（删除远程多余文件）",
    )
    parser.add_argument(
        "--clear",
        action="store_true",
        help="清空仓库现有内容后再上传（仅用于 --archive 模式）",
    )
    
    args = parser.parse_args()
    
    try:
        # 获取 token
        token = args.token
        if token is None:
            token = os.getenv("HF_TOKEN") or os.getenv("HUGGINGFACE_HUB_TOKEN")
        if token is None:
            print("ℹ️  尝试读取本地 HuggingFace 缓存 Token...")
            token = get_token()
        if token is None:
            raise ValueError(
                "未找到 HuggingFace token。\n"
                "请先运行 'python -c \"from huggingface_hub import login; login()\"' 登录\n"
                "或设置 HF_TOKEN 环境变量。"
            )
        
        # 根据参数选择上传模式
        if args.archive:
            # 上传压缩包模式
            repo_id = push_archive(
                archive_path=args.archive,
                repo_id=args.repo_id,
                token=token,
                private=args.private,
                commit_message=args.commit_message,
                create_repo=not args.no_create_repo,
                clear_repo=args.clear,
            )
        else:
            # 上传文件夹模式
            repo_id = push_content(
                content_dir=args.content_dir,
                repo_id=args.repo_id,
                token=token,
                version=args.version,
                private=args.private,
                commit_message=args.commit_message,
                create_repo=not args.no_create_repo,
                force=args.force,
            )
        
        print("\n" + "=" * 60)
        print("📊 推送完成!")
        print(f"   仓库: {repo_id}")
        print(f"   URL: https://huggingface.co/datasets/{repo_id}")
        print("=" * 60)
        return 0
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit(main())
