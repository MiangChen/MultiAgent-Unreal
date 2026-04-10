#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Push UE5 Content assets to HuggingFace Hub

支持两种模式：
1. 指定 Content 文件夹，自动打包后上传（默认使用项目内的 Content）
2. 指定已有的压缩包直接上传
"""

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Optional

try:
    from huggingface_hub import HfApi, upload_file, get_token
    from huggingface_hub.utils import HfHubHTTPError
    HF_HUB_AVAILABLE = True
except ImportError:
    HF_HUB_AVAILABLE = False
    print("Warning: huggingface_hub not installed. Run: pip install huggingface_hub")

# 默认路径
SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_CONTENT_DIR = PROJECT_ROOT / "unreal_project" / "Content"
DEFAULT_REPO_ID = "WindyLab/MultiAgent-Content"
DEFAULT_ARCHIVE_NAME = "Content.tar.gz"


def get_content_stats(content_dir: Path) -> dict:
    """Get Content directory statistics"""
    stats = {
        "total_files": 0,
        "total_size_mb": 0,
        "uasset_count": 0,
        "umap_count": 0,
        "subdirs": [],
    }
    
    if not content_dir.exists():
        return stats
    
    # 统计子目录（排除隐藏目录）
    stats["subdirs"] = [d.name for d in content_dir.iterdir() 
                        if d.is_dir() and not d.name.startswith('.')]
    
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


def create_archive(content_dir: Path, archive_path: Path) -> Path:
    """
    将 Content 文件夹打包为 tar.gz
    
    Args:
        content_dir: Content 文件夹路径
        archive_path: 输出压缩包路径
    
    Returns:
        压缩包路径
    """
    print(f"📦 Packing Content folder...")
    print(f"   Source dir: {content_dir}")
    print(f"   Target file: {archive_path}")
    
    # 使用 tar 命令打包，排除不必要的文件
    parent_dir = content_dir.parent
    folder_name = content_dir.name
    
    cmd = [
        "tar", "-czvf", str(archive_path),
        "-C", str(parent_dir),
        "--exclude", ".cache",
        "--exclude", ".git",
        "--exclude", "*.pyc",
        "--exclude", "__pycache__",
        "--exclude", ".DS_Store",
        folder_name
    ]
    
    try:
        # 使用 subprocess 运行，隐藏详细输出
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=True
        )
        
        file_size_mb = round(archive_path.stat().st_size / (1024 * 1024), 2)
        print(f"✅ Packing complete: {file_size_mb} MB")
        return archive_path
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Packing failed: {e.stderr}")
        raise


def generate_readme(archive_name: str, file_size_mb: float, stats: Optional[dict] = None) -> str:
    """Generate README.md content"""
    stats_section = ""
    if stats:
        stats_section = f"""
## Asset Statistics

- **Total files**: {stats['total_files']}
- **Total size**: {stats['total_size_mb']} MB
- **UAsset files**: {stats['uasset_count']}
- **UMap files**: {stats['umap_count']}

## Directory Structure

```
Content/
{chr(10).join(f"├── {d}/" for d in stats['subdirs'])}
```
"""
    
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

UE5 project assets for the multi-robot simulation system [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal).

## Asset Info

- **Archive**: {archive_name}
- **Size**: {file_size_mb} MB
{stats_section}
## Download

### Method 1: Download script (recommended)

```bash
# 在项目根目录运行
python scripts/download_content.py
```

### Method 2: Download with huggingface-cli

```bash
# 安装 huggingface_hub
pip install huggingface_hub

# 下载压缩包
huggingface-cli download WindyLab/MultiAgent-Content {archive_name} --repo-type dataset --local-dir .
```

### Method 3: Download with Python

```python
from huggingface_hub import hf_hub_download

hf_hub_download(
    repo_id="WindyLab/MultiAgent-Content",
    filename="{archive_name}",
    repo_type="dataset",
    local_dir="."
)
```

## Extract

```bash
tar -xzvf {archive_name} -C unreal_project/
```

## Related Projects

- [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal) - Multi-robot simulation system

## License

MIT License
"""


def push_archive(
    archive_path: Path,
    repo_id: str,
    token: Optional[str] = None,
    private: bool = False,
    commit_message: Optional[str] = None,
    create_repo: bool = True,
    clear_repo: bool = False,
    stats: Optional[dict] = None,
) -> str:
    """
    推送压缩包到 HuggingFace Hub
    """
    if not HF_HUB_AVAILABLE:
        raise ImportError("huggingface_hub not installed. Run: pip install huggingface_hub")
    
    if not archive_path.exists():
        raise FileNotFoundError(f"Archive not found: {archive_path}")
    
    file_size_mb = round(archive_path.stat().st_size / (1024 * 1024), 2)
    print(f"📦 Archive info:")
    print(f"   File: {archive_path.name}")
    print(f"   Size: {file_size_mb} MB")
    
    # 创建 HfApi 实例
    api = HfApi(token=token)
    
    # 检查仓库是否存在
    repo_exists = False
    try:
        api.repo_info(repo_id=repo_id, repo_type="dataset", token=token)
        repo_exists = True
        print(f"ℹ️  Repo already exists: {repo_id}")
    except Exception:
        print(f"ℹ️  Repo does not exist: {repo_id}")
    
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
                print(f"✅ Repo created: {repo_id}")
            except Exception as e:
                raise ValueError(f"Cannot create repo {repo_id}: {e}")
        else:
            raise ValueError(f"Repo {repo_id}  does not exist, create it first or use --create-repo")
    
    # 清空仓库现有内容
    if clear_repo and repo_exists:
        print(f"🗑️  Clearing existing repo content...")
        try:
            files = api.list_repo_files(repo_id=repo_id, repo_type="dataset")
            if files:
                files_to_delete = [f for f in files if f != ".gitattributes"]
                if files_to_delete:
                    from huggingface_hub import CommitOperationDelete
                    delete_ops = [CommitOperationDelete(path_in_repo=f) for f in files_to_delete]
                    api.create_commit(
                        repo_id=repo_id,
                        repo_type="dataset",
                        operations=delete_ops,
                        commit_message="Clear repository for new upload",
                    )
                    print(f"   Deleted {len(files_to_delete)} files")
        except Exception as e:
            print(f"⚠️  Failed to clear repo: {e}")
            print(f"   Continuing upload...")
    
    # 生成 README.md
    readme_content = generate_readme(archive_path.name, file_size_mb, stats)
    readme_path = archive_path.parent / "README_temp.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(readme_content)
    
    # 推送到 Hub
    if commit_message is None:
        commit_message = f"Upload {archive_path.name}"
    
    print(f"📤 Uploading archive to Hub: {repo_id} ...")
    print(f"   This may take a while, please be patient...")
    
    try:
        # 先上传 README
        api.upload_file(
            path_or_fileobj=str(readme_path),
            path_in_repo="README.md",
            repo_id=repo_id,
            repo_type="dataset",
            commit_message="Update README.md",
        )
        print(f"✅ README.md uploaded")
        
        # 上传压缩包
        api.upload_file(
            path_or_fileobj=str(archive_path),
            path_in_repo=archive_path.name,
            repo_id=repo_id,
            repo_type="dataset",
            commit_message=commit_message,
        )
        
        # 清理临时文件
        readme_path.unlink(missing_ok=True)
        
        print(f"✅ Upload successful: https://huggingface.co/datasets/{repo_id}")
        return repo_id
        
    except HfHubHTTPError as e:
        readme_path.unlink(missing_ok=True)
        print(f"❌ Upload failed: {e}")
        if "403" in str(e) or "Forbidden" in str(e):
            print(f"   Hint: Check token permissions, ensure write access")
        raise
    except Exception as e:
        readme_path.unlink(missing_ok=True)
        print(f"❌ Upload failed: {e}")
        raise


def main():
    parser = argparse.ArgumentParser(
        description="Push UE5 Content assets to HuggingFace Hub",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Examples:
  # Use project Content folder (default)
  python scripts/push_content_to_hub.py

  # Specify another Content folder
  python scripts/push_content_to_hub.py --content-dir ~/Material/UE5_Projects/MultiAgent_Content

  # Upload an existing archive directly
  python scripts/push_content_to_hub.py --archive ~/Material/UE5_Projects/Content.tar.gz

  # Upload and clear existing repo content
  python scripts/push_content_to_hub.py --clear

Default Content dir: {DEFAULT_CONTENT_DIR}
""",
    )
    
    # 输入源（二选一）
    source_group = parser.add_mutually_exclusive_group()
    source_group.add_argument(
        "--content-dir",
        type=str,
        default=None,
        help=f"Content folder path (default: {DEFAULT_CONTENT_DIR}）",
    )
    source_group.add_argument(
        "--archive",
        type=str,
        default=None,
        help="Existing archive path (skip packing step)",
    )
    
    parser.add_argument(
        "--repo-id",
        type=str,
        default=DEFAULT_REPO_ID,
        help=f"HuggingFace repo ID (default: {DEFAULT_REPO_ID}）",
    )
    parser.add_argument(
        "--token",
        type=str,
        default=None,
        help="HuggingFace access token (env var HF_TOKEN takes priority)",
    )
    parser.add_argument(
        "--private",
        action="store_true",
        help="Create private repo",
    )
    parser.add_argument(
        "--commit-message",
        type=str,
        default=None,
        help="Commit message",
    )
    parser.add_argument(
        "--no-create-repo",
        action="store_true",
        help="Do not auto-create repo if it does not exist",
    )
    parser.add_argument(
        "--clear",
        action="store_true",
        help="Clear existing repo content before uploading",
    )
    parser.add_argument(
        "--keep-archive",
        action="store_true",
        help="Keep archive after upload (auto-generated archives are deleted by default)",
    )
    
    args = parser.parse_args()
    
    try:
        # 获取 token
        token = args.token
        if token is None:
            token = os.getenv("HF_TOKEN") or os.getenv("HUGGINGFACE_HUB_TOKEN")
        if token is None:
            print("ℹ️  Trying to read local HuggingFace cached token...")
            token = get_token()
        if token is None:
            raise ValueError(
                "HuggingFace token not found.\n"
                "Please run 'python -c \"from huggingface_hub import login; login()\"' to login\n"
                "or set the HF_TOKEN environment variable."
            )
        
        archive_path: Path
        stats: Optional[dict] = None
        auto_created_archive = False
        
        if args.archive:
            # 直接使用已有压缩包
            archive_path = Path(args.archive).expanduser().resolve()
            if not archive_path.exists():
                raise FileNotFoundError(f"Archive not found: {archive_path}")
        else:
            # 从 Content 文件夹打包
            content_dir = Path(args.content_dir).expanduser().resolve() if args.content_dir else DEFAULT_CONTENT_DIR
            
            if not content_dir.exists():
                raise FileNotFoundError(f"Content directory not found: {content_dir}")
            
            # 获取统计信息
            print(f"📊 Analyzing Content directory...")
            stats = get_content_stats(content_dir)
            print(f"   Files: {stats['total_files']}")
            print(f"   Size: {stats['total_size_mb']} MB")
            print(f"   UAsset: {stats['uasset_count']}, UMap: {stats['umap_count']}")
            
            # 在临时目录创建压缩包
            temp_dir = Path(tempfile.gettempdir())
            archive_path = temp_dir / DEFAULT_ARCHIVE_NAME
            
            create_archive(content_dir, archive_path)
            auto_created_archive = True
        
        # 上传压缩包
        repo_id = push_archive(
            archive_path=archive_path,
            repo_id=args.repo_id,
            token=token,
            private=args.private,
            commit_message=args.commit_message,
            create_repo=not args.no_create_repo,
            clear_repo=args.clear,
            stats=stats,
        )
        
        # 清理自动生成的压缩包
        if auto_created_archive and not args.keep_archive:
            print(f"🗑️  Deleting temporary archive...")
            archive_path.unlink(missing_ok=True)
            print(f"✅ Temporary files cleaned up")
        
        print("\n" + "=" * 60)
        print("📊 Push complete!")
        print(f"   Repo: {repo_id}")
        print(f"   URL: https://huggingface.co/datasets/{repo_id}")
        print("=" * 60)
        return 0
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit(main())