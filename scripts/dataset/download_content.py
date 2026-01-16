#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
从 HuggingFace Hub 下载 UE5 Content 资源压缩包

支持：
- 只下载压缩包（不下载其他文件）
- 自动解压到指定目录
- 指定下载分支

Usage:
    python scripts/download_content.py
    python scripts/download_content.py --extract
    python scripts/download_content.py --output ~/Downloads --extract
    python scripts/download_content.py --branch dev
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

# 默认配置
SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_REPO_ID = "WindyLab/MultiAgent-Content"
DEFAULT_ARCHIVE_NAME = "Content.tar.gz"
DEFAULT_OUTPUT_DIR = PROJECT_ROOT / "unreal_project"
DEFAULT_BRANCH = "main"


def check_dependencies():
    """检查 huggingface_hub 是否安装"""
    try:
        from huggingface_hub import hf_hub_download
        return True
    except ImportError:
        return False


def install_dependencies():
    """安装 huggingface_hub"""
    print("正在安装 huggingface_hub...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "huggingface_hub"])


def download_archive(
    repo_id: str,
    filename: str,
    output_dir: Path,
    branch: str = "main",
    token: str = None,
) -> Path:
    """
    从 HuggingFace Hub 下载压缩包
    
    Args:
        repo_id: HuggingFace 仓库ID
        filename: 要下载的文件名
        output_dir: 下载目录
        branch: 分支名
        token: HuggingFace token（可选）
    
    Returns:
        下载的文件路径
    """
    from huggingface_hub import hf_hub_download
    
    print(f"📥 正在下载 {filename}...")
    print(f"   仓库: {repo_id}")
    print(f"   分支: {branch}")
    print(f"   目标目录: {output_dir}")
    print(f"   这可能需要一些时间，请耐心等待...")
    
    # 确保输出目录存在
    output_dir.mkdir(parents=True, exist_ok=True)
    
    try:
        local_path = hf_hub_download(
            repo_id=repo_id,
            filename=filename,
            repo_type="dataset",
            revision=branch,
            local_dir=output_dir,
            token=token,
        )
        
        downloaded_file = Path(local_path)
        file_size_mb = round(downloaded_file.stat().st_size / (1024 * 1024), 2)
        print(f"✅ 下载完成: {downloaded_file}")
        print(f"   大小: {file_size_mb} MB")
        
        # 清理 huggingface 缓存目录
        cache_dir = output_dir / ".cache"
        if cache_dir.exists():
            import shutil
            shutil.rmtree(cache_dir)
            print(f"🗑️  已清理缓存目录: {cache_dir}")
        
        return downloaded_file
        
    except Exception as e:
        print(f"❌ 下载失败: {e}")
        if "401" in str(e) or "403" in str(e):
            print("\n   这可能是私有仓库，请设置 HF_TOKEN 环境变量或使用 --token 参数")
        elif "404" in str(e):
            print(f"\n   文件 {filename} 不存在，请检查仓库内容")
        raise


def extract_archive(archive_path: Path, extract_dir: Path, delete_after: bool = False) -> Path:
    """
    解压压缩包
    
    Args:
        archive_path: 压缩包路径
        extract_dir: 解压目录
        delete_after: 解压后是否删除压缩包
    
    Returns:
        解压后的目录路径
    """
    print(f"📦 正在解压...")
    print(f"   压缩包: {archive_path}")
    print(f"   目标目录: {extract_dir}")
    
    # 确保目标目录存在
    extract_dir.mkdir(parents=True, exist_ok=True)
    
    # 根据文件扩展名选择解压方式
    suffix = archive_path.suffix.lower()
    name = archive_path.name.lower()
    
    try:
        if name.endswith('.tar.gz') or name.endswith('.tgz'):
            cmd = ["tar", "-xzvf", str(archive_path), "-C", str(extract_dir)]
            subprocess.run(cmd, capture_output=True, check=True)
        elif name.endswith('.tar'):
            cmd = ["tar", "-xvf", str(archive_path), "-C", str(extract_dir)]
            subprocess.run(cmd, capture_output=True, check=True)
        elif suffix == '.zip':
            import zipfile
            with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                zip_ref.extractall(extract_dir)
        else:
            raise ValueError(f"不支持的压缩格式: {archive_path.name}")
        
        print(f"✅ 解压完成")
        
        # 检查解压后的 Content 目录
        content_dir = extract_dir / "Content"
        if content_dir.exists():
            print(f"   Content 目录: {content_dir}")
        
        # 删除压缩包
        if delete_after:
            print(f"🗑️  正在删除压缩包...")
            archive_path.unlink()
            print(f"✅ 压缩包已删除")
        
        return extract_dir
        
    except subprocess.CalledProcessError as e:
        print(f"❌ 解压失败: {e}")
        raise
    except Exception as e:
        print(f"❌ 解压失败: {e}")
        raise


def list_repo_files(repo_id: str, branch: str = "main", token: str = None) -> list:
    """列出仓库中的文件"""
    from huggingface_hub import HfApi
    
    api = HfApi(token=token)
    try:
        files = api.list_repo_files(repo_id=repo_id, repo_type="dataset", revision=branch)
        return list(files)
    except Exception:
        return []


def main():
    parser = argparse.ArgumentParser(
        description="从 HuggingFace Hub 下载 UE5 Content 资源",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
示例:
  # 下载压缩包到默认位置（unreal_project/）
  python download_content.py

  # 下载并自动解压
  python download_content.py --extract

  # 下载到指定目录
  python download_content.py --output ~/Downloads

  # 下载并解压，然后删除压缩包
  python download_content.py --extract --delete-archive

  # 从指定分支下载
  python download_content.py --branch dev

  # 下载指定文件名
  python download_content.py --filename Content.tar.gz

默认输出目录: {DEFAULT_OUTPUT_DIR}
""",
    )
    
    parser.add_argument(
        "--repo",
        type=str,
        default=DEFAULT_REPO_ID,
        help=f"HuggingFace 仓库ID（默认: {DEFAULT_REPO_ID}）",
    )
    parser.add_argument(
        "--filename",
        type=str,
        default=DEFAULT_ARCHIVE_NAME,
        help=f"要下载的文件名（默认: {DEFAULT_ARCHIVE_NAME}）",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help=f"下载目录（默认: {DEFAULT_OUTPUT_DIR}）",
    )
    parser.add_argument(
        "--branch",
        type=str,
        default=DEFAULT_BRANCH,
        help=f"分支名（默认: {DEFAULT_BRANCH}）",
    )
    parser.add_argument(
        "--token",
        type=str,
        default=None,
        help="HuggingFace token（或设置 HF_TOKEN 环境变量）",
    )
    parser.add_argument(
        "--extract",
        action="store_true",
        help="下载后自动解压",
    )
    parser.add_argument(
        "--delete-archive",
        action="store_true",
        help="解压后删除压缩包（需配合 --extract 使用）",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="列出仓库中的文件（不下载）",
    )
    
    args = parser.parse_args()
    
    # 检查并安装依赖
    if not check_dependencies():
        print("huggingface_hub 未安装，正在安装...")
        install_dependencies()
    
    # 获取 token
    token = args.token or os.environ.get('HF_TOKEN') or os.environ.get('HUGGINGFACE_HUB_TOKEN')
    
    print("=" * 60)
    print("  HuggingFace Content 下载器")
    print("=" * 60)
    
    # 列出文件模式
    if args.list:
        print(f"📋 仓库 {args.repo} ({args.branch}) 中的文件:")
        files = list_repo_files(args.repo, args.branch, token)
        if files:
            for f in files:
                print(f"   - {f}")
        else:
            print("   (无法获取文件列表或仓库为空)")
        return 0
    
    # 确定输出目录
    output_dir = Path(args.output).expanduser().resolve() if args.output else DEFAULT_OUTPUT_DIR
    
    print(f"  仓库: {args.repo}")
    print(f"  分支: {args.branch}")
    print(f"  文件: {args.filename}")
    print(f"  输出: {output_dir}")
    print(f"  解压: {'是' if args.extract else '否'}")
    print("=" * 60)
    
    try:
        # 下载压缩包
        archive_path = download_archive(
            repo_id=args.repo,
            filename=args.filename,
            output_dir=output_dir,
            branch=args.branch,
            token=token,
        )
        
        # 解压
        if args.extract:
            extract_archive(
                archive_path=archive_path,
                extract_dir=output_dir,
                delete_after=args.delete_archive,
            )
        
        print("\n" + "=" * 60)
        print("✅ 完成!")
        if args.extract:
            content_dir = output_dir / "Content"
            if content_dir.exists():
                print(f"   Content 目录: {content_dir}")
        else:
            print(f"   压缩包: {archive_path}")
            print(f"\n   解压命令: tar -xzvf {archive_path} -C {output_dir}")
        print("=" * 60)
        return 0
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        return 1


if __name__ == "__main__":
    exit(main())