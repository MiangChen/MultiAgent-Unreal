# =============================================================================
# Config Manager Module - Unified Configuration Management for UnrealZoo
# =============================================================================
#
# This module provides a centralized configuration management system that
# manages global settings for the UnrealZoo project.
#
# Usage:
#   from config.config_manager import config_manager
#   ue_binary = config_manager.get("ue_binary_path")
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
        # 自动计算项目根目录
        self.project_root = Path(__file__).parent.parent.absolute()
        self.load()

    def load(self) -> None:
        """加载 YAML 配置文件"""
        if self.config_path.exists():
            with open(self.config_path, "r") as f:
                self.config = yaml.safe_load(f) or {}
        else:
            self.config = {}

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

    def get_ue_binary_path(self) -> str:
        """获取 UE Binary 的绝对路径"""
        return self.get("ue_binary_path")

    def load_env_config(self) -> Dict[str, Any]:
        """
        加载环境 JSON 配置文件。
        根据 env.task 和 env.map 自动拼接路径: {task}/{map}.json
        
        Returns:
            环境配置字典
        """
        import json

        task = self.get("env.task", "Track")
        map_name = self.get("env.map", "Grass_Hills")
        config_path = f"{task}/{map_name}.json"

        full_path = self.project_root / "gym_unrealcv" / "envs" / "setting" / config_path

        print(f"📄 加载环境配置: {full_path}")
        with open(full_path, "r") as f:
            return json.load(f)

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
