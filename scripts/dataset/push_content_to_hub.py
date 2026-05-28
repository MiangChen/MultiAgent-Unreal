#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
推送 UE5 Content 资源到 HuggingFace Hub

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
    print("警告: huggingface_hub 未安装，请运行: pip install huggingface_hub")

# 默认路径
SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_CONTENT_DIR = PROJECT_ROOT / "unreal_project" / "Content"
DEFAULT_REPO_ID = "WindyLab/MultiAgent-Content"
DEFAULT_ARCHIVE_NAME = "Content.tar.gz"


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
    print(f"📦 正在打包 Content 文件夹...")
    print(f"   源目录: {content_dir}")
    print(f"   目标文件: {archive_path}")
    
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
        print(f"✅ 打包完成: {file_size_mb} MB")
        return archive_path
        
    except subprocess.CalledProcessError as e:
        print(f"❌ 打包失败: {e.stderr}")
        raise


def generate_readme(archive_name: str, file_size_mb: float, stats: Optional[dict] = None) -> str:
    """生成 README.md 内容"""
    stats_section = ""
    if stats:
        stats_section = f"""
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

UE5 项目资源文件，用于多机器人仿真系统 [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal)。

## 资源信息

- **压缩包**: {archive_name}
- **大小**: {file_size_mb} MB
{stats_section}
## 下载方法

### 方法 1: 使用下载脚本（推荐）

```bash
# 在项目根目录运行
python scripts/download_content.py
```

### 方法 2: 使用 huggingface-cli 下载

```bash
# 安装 huggingface_hub
pip install huggingface_hub

# 下载压缩包
huggingface-cli download WindyLab/MultiAgent-Content {archive_name} --repo-type dataset --local-dir .
```

### 方法 3: 使用 Python 下载

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
tar -xzvf {archive_name} -C unreal_project/
```

## 相关项目

- [MultiAgent-Unreal](https://github.com/WindyLab/MultiAgent-Unreal) - 多机器人仿真系统

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
        raise ImportError("huggingface_hub 未安装。请运行: pip install huggingface_hub")
    
    if not archive_path.exists():
        raise FileNotFoundError(f"压缩包不存在: {archive_path}")
    
    file_size_mb = round(archive_path.stat().st_size / (1024 * 1024), 2)
    print(f"📦 压缩包信息:")
    print(f"   文件: {archive_path.name}")
    print(f"   大小: {file_size_mb} MB")
    
    # 创建 HfApi 实例
    api = HfApi(token=token)
    
    # 检查仓库是否存在
    repo_exists = False
    try:
        api.repo_info(repo_id=repo_id, repo_type="dataset", token=token)
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
                    print(f"   已删除 {len(files_to_delete)} 个文件")
        except Exception as e:
            print(f"⚠️  清空仓库失败: {e}")
            print(f"   继续上传...")
    
    # 生成 README.md
    readme_content = generate_readme(archive_path.name, file_size_mb, stats)
    readme_path = archive_path.parent / "README_temp.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(readme_content)
    
    # 推送到 Hub
    if commit_message is None:
        commit_message = f"Upload {archive_path.name}"
    
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
            path_or_fileobj=str(archive_path),
            path_in_repo=archive_path.name,
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


def main():
    parser = argparse.ArgumentParser(
        description="推送 UE5 Content 资源到 HuggingFace Hub",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
示例:
  # 使用项目内的 Content 文件夹（默认）
  python scripts/push_content_to_hub.py

  # 指定其他 Content 文件夹
  python scripts/push_content_to_hub.py --content-dir ~/Material/UE5_Projects/MultiAgent_Content

  # 直接上传已有的压缩包
  python scripts/push_content_to_hub.py --archive ~/Material/UE5_Projects/Content.tar.gz

  # 上传并清空仓库现有内容
  python scripts/push_content_to_hub.py --clear

默认 Content 目录: {DEFAULT_CONTENT_DIR}
""",
    )
    
    # 输入源（二选一）
    source_group = parser.add_mutually_exclusive_group()
    source_group.add_argument(
        "--content-dir",
        type=str,
        default=None,
        help=f"Content 文件夹路径（默认: {DEFAULT_CONTENT_DIR}）",
    )
    source_group.add_argument(
        "--archive",
        type=str,
        default=None,
        help="已有的压缩包路径（跳过打包步骤）",
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
        "--clear",
        action="store_true",
        help="清空仓库现有内容后再上传",
    )
    parser.add_argument(
        "--keep-archive",
        action="store_true",
        help="上传后保留压缩包（默认会删除自动生成的压缩包）",
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
        
        archive_path: Path
        stats: Optional[dict] = None
        auto_created_archive = False
        
        if args.archive:
            # 直接使用已有压缩包
            archive_path = Path(args.archive).expanduser().resolve()
            if not archive_path.exists():
                raise FileNotFoundError(f"压缩包不存在: {archive_path}")
        else:
            # 从 Content 文件夹打包
            content_dir = Path(args.content_dir).expanduser().resolve() if args.content_dir else DEFAULT_CONTENT_DIR
            
            if not content_dir.exists():
                raise FileNotFoundError(f"Content 目录不存在: {content_dir}")
            
            # 获取统计信息
            print(f"📊 正在分析 Content 目录...")
            stats = get_content_stats(content_dir)
            print(f"   文件数: {stats['total_files']}")
            print(f"   大小: {stats['total_size_mb']} MB")
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
            print(f"🗑️  正在删除临时压缩包...")
            archive_path.unlink(missing_ok=True)
            print(f"✅ 临时文件已清理")
        
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