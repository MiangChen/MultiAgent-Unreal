# =============================================================================
# Config Manager Module - Unified Configuration Management for UnrealZoo
# =============================================================================
#
# This module provides a centralized configuration management system that
# manages UnrealEnv path and other global settings for the UnrealZoo project.
#
# Usage:
#   from config.config_manager import config_manager
#   unreal_env_path = config_manager.get("unreal_env")
#
# =============================================================================

# Standard library imports
import os
import pprint
from pathlib import Path
from typing import Any, Dict

# Third-party library imports
import yaml


class ConfigManager:
    """
    全局配置管理器，用于管理 UnrealZoo 项目的配置。
    单例模式，确保全局唯一实例。
    """

    def __init__(self):
        self.config: Dict[str, Any] = {}
        self.config_path = Path(__file__).parent / "config_parameter.yaml"
        self.load()
        self._setup_environment()

    def load(self) -> None:
        """加载 YAML 配置文件"""
        if self.config_path.exists():
            with open(self.config_path, "r") as f:
                self.config = yaml.safe_load(f) or {}
        else:
            self.config = {}

        self._derive_paths()

    def _derive_paths(self) -> None:
        """计算所有派生路径"""
        # 自动计算项目根目录
        # config/config_manager.py -> config -> project_root
        project_root = Path(__file__).parent.parent.absolute()
        self.config["project_root"] = str(project_root)

        # 设置默认的 UnrealEnv 路径
        if "unreal_env" not in self.config:
            # 优先级: 配置文件 > 环境变量 > 默认路径
            default_path = os.path.expanduser("~/UnrealEnv")
            self.config["unreal_env"] = os.environ.get("UnrealEnv", default_path)

        # 设置默认的 UE Binary 相对路径
        if "ue_binary_path" not in self.config:
            self.config["ue_binary_path"] = "UnrealZoo_UE5_5_Linux_v1.0.5/Linux/UnrealZoo_UE5_5/Binaries/Linux/UnrealZoo_UE5_5"

    def _setup_environment(self) -> None:
        """设置环境变量，确保 unrealcv 能正确找到 UE Binary"""
        unreal_env = self.config.get("unreal_env")
        if unreal_env:
            os.environ["UnrealEnv"] = unreal_env

    def get(self, key: str, default: Any = None) -> Any:
        """
        安全地从配置中获取一个值。支持点状访问 (e.g., 'world.name')。
        
        Args:
            key: 配置键，支持点状访问
            default: 默认值，如果键不存在则返回
            
        Returns:
            配置值或默认值
        """
        keys = key.split(".")
        value = self.config
        try:
            for k in keys:
                value = value[k]
            return value
        except (KeyError, TypeError):
            return default

    def set(self, key: str, value: Any) -> None:
        """
        设置配置值。支持点状访问。
        
        Args:
            key: 配置键
            value: 配置值
        """
        keys = key.split(".")
        config = self.config
        for k in keys[:-1]:
            if k not in config:
                config[k] = {}
            config = config[k]
        config[keys[-1]] = value

        # 如果设置了 unreal_env，同步更新环境变量
        if key == "unreal_env":
            os.environ["UnrealEnv"] = value

    def get_ue_binary_full_path(self) -> str:
        """获取 UE Binary 的完整路径"""
        unreal_env = self.get("unreal_env")
        ue_binary = self.get("ue_binary_path")
        return os.path.join(unreal_env, ue_binary)

    def get_summary(self) -> str:
        """获取配置摘要"""
        summary = "=== UnrealZoo Configuration Summary ===\n"
        summary += f"Config file: {self.config_path}\n"
        summary += f"Config exists: {self.config_path.exists()}\n"
        summary += "-----------------------------------\n"
        summary += pprint.pformat(self.config)
        summary += "\n==================================="
        return summary

    def __repr__(self) -> str:
        return self.get_summary()


# 全局单例实例
config_manager = ConfigManager()
