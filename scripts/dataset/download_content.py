#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Download UE5 Content archive from HuggingFace Hub

Features:
- Download archive only (no other files)
- Auto-extract to specified directory
- Specify download branch

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

SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_REPO_ID = "WindyLab/MultiAgent-Content"
DEFAULT_ARCHIVE_NAME = "Content.tar.gz"
DEFAULT_OUTPUT_DIR = PROJECT_ROOT / "unreal_project"
DEFAULT_BRANCH = "main"


def check_dependencies():
    """Check if huggingface_hub is installed"""
    try:
        from huggingface_hub import hf_hub_download
        return True
    except ImportError:
        return False


def install_dependencies():
    """Install huggingface_hub"""
    print("Installing huggingface_hub...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "huggingface_hub"])


def download_archive(
    repo_id: str,
    filename: str,
    output_dir: Path,
    branch: str = "main",
    token: str = None,
) -> Path:
    """
    Download archive from HuggingFace Hub
    
    Args:
        repo_id: HuggingFace repository ID
        filename: File to download
        output_dir: Download directory
        branch: Branch name
        token: HuggingFace token (optional)
    
    Returns:
        Path to downloaded file
    """
    from huggingface_hub import hf_hub_download
    
    print(f"📥 Downloading {filename}...")
    print(f"   Repo: {repo_id}")
    print(f"   Branch: {branch}")
    print(f"   Target dir: {output_dir}")
    print(f"   This may take a while, please be patient...")
    
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
        print(f"✅ Download complete: {downloaded_file}")
        print(f"   Size: {file_size_mb} MB")
        
        cache_dir = output_dir / ".cache"
        if cache_dir.exists():
            import shutil
            shutil.rmtree(cache_dir)
            print(f"🗑️  Cleaned cache dir: {cache_dir}")
        
        return downloaded_file
        
    except Exception as e:
        print(f"❌ Download failed: {e}")
        if "401" in str(e) or "403" in str(e):
            print("\n   This may be a private repo. Set HF_TOKEN env var or use --token")
        elif "404" in str(e):
            print(f"\n   File {filename} not found. Check repo contents")
        raise


def extract_archive(archive_path: Path, extract_dir: Path, delete_after: bool = False) -> Path:
    """
    Extract archive
    
    Args:
        archive_path: Path to archive
        extract_dir: Extraction directory
        delete_after: Delete archive after extraction
    
    Returns:
        Path to extracted directory
    """
    print(f"📦 Extracting...")
    print(f"   Archive: {archive_path}")
    print(f"   Target dir: {extract_dir}")
    
    extract_dir.mkdir(parents=True, exist_ok=True)
    
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
            raise ValueError(f"Unsupported archive format: {archive_path.name}")
        
        print(f"✅ Extraction complete")
        
        content_dir = extract_dir / "Content"
        if content_dir.exists():
            print(f"   Content dir: {content_dir}")
        
        if delete_after:
            print(f"🗑️  Deleting archive...")
            archive_path.unlink()
            print(f"✅ Archive deleted")
        
        return extract_dir
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Extraction failed: {e}")
        raise
    except Exception as e:
        print(f"❌ Extraction failed: {e}")
        raise


def list_repo_files(repo_id: str, branch: str = "main", token: str = None) -> list:
    """List files in the repository"""
    from huggingface_hub import HfApi
    
    api = HfApi(token=token)
    try:
        files = api.list_repo_files(repo_id=repo_id, repo_type="dataset", revision=branch)
        return list(files)
    except Exception:
        return []


def main():
    parser = argparse.ArgumentParser(
        description="Download UE5 Content assets from HuggingFace Hub",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
Examples:
  # Download archive to default location (unreal_project/)
  python download_content.py

  # Download and auto-extract
  python download_content.py --extract

  # Download to specified directory
  python download_content.py --output ~/Downloads

  # Download, extract, then delete archive
  python download_content.py --extract --delete-archive

  # Download from specified branch
  python download_content.py --branch dev

  # Download specified filename
  python download_content.py --filename Content.tar.gz

Default output dir: {DEFAULT_OUTPUT_DIR}
""",
    )
    
    parser.add_argument(
        "--repo",
        type=str,
        default=DEFAULT_REPO_ID,
        help=f"HuggingFace repo ID (default: {DEFAULT_REPO_ID})",
    )
    parser.add_argument(
        "--filename",
        type=str,
        default=DEFAULT_ARCHIVE_NAME,
        help=f"File to download (default: {DEFAULT_ARCHIVE_NAME})",
    )
    parser.add_argument(
        "--output",
        type=str,
        default=None,
        help=f"Download directory (default: {DEFAULT_OUTPUT_DIR})",
    )
    parser.add_argument(
        "--branch",
        type=str,
        default=DEFAULT_BRANCH,
        help=f"Branch name (default: {DEFAULT_BRANCH})",
    )
    parser.add_argument(
        "--token",
        type=str,
        default=None,
        help="HuggingFace token (or set HF_TOKEN env var)",
    )
    parser.add_argument(
        "--extract",
        action="store_true",
        help="Auto-extract after download",
    )
    parser.add_argument(
        "--delete-archive",
        action="store_true",
        help="Delete archive after extraction (requires --extract)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List files in the repo (no download)",
    )
    
    args = parser.parse_args()
    
    if not check_dependencies():
        print("huggingface_hub not installed, installing...")
        install_dependencies()
    
    token = args.token or os.environ.get('HF_TOKEN') or os.environ.get('HUGGINGFACE_HUB_TOKEN')
    
    print("=" * 60)
    print("  HuggingFace Content Downloader")
    print("=" * 60)
    
    if args.list:
        print(f"📋 Files in {args.repo} ({args.branch}):")
        files = list_repo_files(args.repo, args.branch, token)
        if files:
            for f in files:
                print(f"   - {f}")
        else:
            print("   (Unable to get file list or repo is empty)")
        return 0
    
    output_dir = Path(args.output).expanduser().resolve() if args.output else DEFAULT_OUTPUT_DIR
    
    print(f"  Repo: {args.repo}")
    print(f"  Branch: {args.branch}")
    print(f"  File: {args.filename}")
    print(f"  Output: {output_dir}")
    print(f"  Extract: {'Yes' if args.extract else 'No'}")
    print("=" * 60)
    
    try:
        archive_path = download_archive(
            repo_id=args.repo,
            filename=args.filename,
            output_dir=output_dir,
            branch=args.branch,
            token=token,
        )
        
        if args.extract:
            extract_archive(
                archive_path=archive_path,
                extract_dir=output_dir,
                delete_after=args.delete_archive,
            )
        
        print("\n" + "=" * 60)
        print("✅ Done!")
        if args.extract:
            content_dir = output_dir / "Content"
            if content_dir.exists():
                print(f"   Content dir: {content_dir}")
        else:
            print(f"   Archive: {archive_path}")
            print(f"\n   Extract command: tar -xzvf {archive_path} -C {output_dir}")
        print("=" * 60)
        return 0
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
