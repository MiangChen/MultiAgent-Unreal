#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Download HuggingFace Dataset
Downloads WindyLab/MultiAgent-Content dataset and places it in unreal_project/Content

Usage:
    python3 scripts/download_hf_dataset.py
    python3 scripts/download_hf_dataset.py --repo WindyLab/MultiAgent-Content
    python3 scripts/download_hf_dataset.py --output unreal_project/Content
"""

import argparse
import os
import sys
from pathlib import Path

def check_dependencies():
    """Check if huggingface_hub is installed"""
    try:
        from huggingface_hub import snapshot_download
        return True
    except ImportError:
        return False

def install_dependencies():
    """Install huggingface_hub"""
    print("Installing huggingface_hub...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "huggingface_hub"])

def download_dataset(repo_id: str, output_dir: str, token: str = None):
    """
    Download dataset from HuggingFace Hub
    
    Args:
        repo_id: HuggingFace repository ID (e.g., WindyLab/MultiAgent-Content)
        output_dir: Local directory to save the dataset
        token: HuggingFace token for private repos (optional)
    """
    from huggingface_hub import snapshot_download
    
    # Create output directory if it doesn't exist
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"Downloading {repo_id} to {output_dir}...")
    print("This may take a while depending on the dataset size...")
    
    try:
        local_dir = snapshot_download(
            repo_id=repo_id,
            repo_type="dataset",
            local_dir=output_dir,
            token=token,
            local_dir_use_symlinks=False
        )
        print(f"\n✓ Download complete!")
        print(f"  Dataset saved to: {local_dir}")
        return local_dir
    except Exception as e:
        print(f"\n✗ Download failed: {e}")
        if "401" in str(e) or "403" in str(e):
            print("\nThis might be a private repository.")
            print("Try setting HF_TOKEN environment variable or use --token argument.")
        raise

def main():
    parser = argparse.ArgumentParser(
        description='Download HuggingFace dataset for MultiAgent-Unreal'
    )
    parser.add_argument(
        '--repo', 
        type=str, 
        default='WindyLab/MultiAgent-Content',
        help='HuggingFace repository ID (default: WindyLab/MultiAgent-Content)'
    )
    parser.add_argument(
        '--output', 
        type=str, 
        default='unreal_project/Content',
        help='Output directory (default: unreal_project/Content)'
    )
    parser.add_argument(
        '--token',
        type=str,
        default=None,
        help='HuggingFace token for private repos (or set HF_TOKEN env var)'
    )
    args = parser.parse_args()
    
    # Get script directory and project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    # Resolve output path relative to project root
    output_dir = project_root / args.output
    
    print("=" * 60)
    print("  HuggingFace Dataset Downloader")
    print("=" * 60)
    print(f"  Repository: {args.repo}")
    print(f"  Output:     {output_dir}")
    print("=" * 60)
    
    # Check and install dependencies
    if not check_dependencies():
        print("\nhuggingface_hub not found. Installing...")
        install_dependencies()
    
    # Get token from argument or environment
    token = args.token or os.environ.get('HF_TOKEN')
    
    # Download dataset
    try:
        download_dataset(args.repo, str(output_dir), token)
        print("\nDone!")
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
