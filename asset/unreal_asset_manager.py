#!/usr/bin/env python3
"""
UnrealZoo Asset Manager - 资产扫描和索引

功能:
1. 解析 .utoc 文件提取资产列表（无需启动 UE）
2. 扫描配置文件中定义的 agent

用法:
  python asset/unreal_asset_manager.py --utoc     # 解析 utoc 文件
  python asset/unreal_asset_manager.py --scan     # 扫描配置文件
"""

import json
import re
import yaml
import argparse
from pathlib import Path
from datetime import datetime


def load_config():
    """从 config/config.yaml 加载配置"""
    config_path = Path(__file__).parent.parent / "config" / "config.yaml"
    if config_path.exists():
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
            return config if config else {}
    return {}


def parse_utoc(utoc_path):
    """解析 .utoc 文件提取资产列表"""
    print(f"解析: {utoc_path}")
    
    with open(utoc_path, 'rb') as f:
        data = f.read()
    
    file_size = len(data)
    print(f"文件大小: {file_size / 1024 / 1024:.1f} MB")
    
    # 字符串表通常在文件后半部分
    # 从 90% 位置开始搜索以提高效率
    search_start = int(file_size * 0.85)
    string_section = data[search_start:]
    
    # 提取以 null 结尾的可打印字符串
    strings = re.findall(rb'[\x20-\x7e]{4,}\x00', string_section)
    
    assets = {"uasset": [], "umap": [], "ubulk": []}
    for s in strings:
        try:
            text = s[:-1].decode('utf-8')
            if text.endswith('.uasset'):
                assets["uasset"].append(text)
            elif text.endswith('.umap'):
                assets["umap"].append(text)
            elif text.endswith('.ubulk'):
                assets["ubulk"].append(text)
        except:
            pass
    
    # 分类
    blueprints = sorted(set([a for a in assets['uasset'] if a.startswith('BP_')]))
    static_meshes = sorted(set([a for a in assets['uasset'] if a.startswith('SM_')]))
    materials = sorted(set([a for a in assets['uasset'] if a.startswith('M_') or a.startswith('MI_')]))
    textures = sorted(set([a for a in assets['uasset'] if a.startswith('T_')]))
    skeletal_meshes = sorted(set([a for a in assets['uasset'] if a.startswith('SK_')]))
    animations = sorted(set([a for a in assets['uasset'] if a.startswith('A_') or a.startswith('Anim')]))
    maps = sorted(set([a.replace('.umap', '') for a in assets['umap']]))
    
    return {
        "_meta": {
            "source": str(utoc_path),
            "scan_time": datetime.now().isoformat(),
            "description": "从 .utoc 文件提取的资产索引"
        },
        "summary": {
            "total_uasset": len(assets['uasset']),
            "total_umap": len(assets['umap']),
            "total_ubulk": len(assets['ubulk']),
            "blueprints": len(blueprints),
            "static_meshes": len(static_meshes),
            "skeletal_meshes": len(skeletal_meshes),
            "materials": len(materials),
            "textures": len(textures),
            "animations": len(animations)
        },
        "maps": maps,
        "blueprints": blueprints,
        "static_meshes": static_meshes,
        "skeletal_meshes": skeletal_meshes,
        "materials": materials[:200],  # 限制数量
        "textures": textures[:200],
        "animations": animations[:200]
    }


def scan_config_files():
    """扫描配置文件中定义的资产"""
    import gym_unrealcv
    
    gym_path = Path(gym_unrealcv.__file__).parent
    setting_dir = gym_path / "envs" / "setting"
    
    result = {
        "agents": {},
        "maps": [],
        "configs": []
    }
    
    for json_file in setting_dir.rglob("*.json"):
        try:
            with open(json_file, 'r') as f:
                config = json.load(f)
            
            config_name = json_file.name
            result["configs"].append(config_name)
            
            if "env_name" in config:
                map_name = config["env_name"]
                if map_name not in result["maps"]:
                    result["maps"].append(map_name)
            
            if "agents" in config:
                for agent_type, agent_config in config["agents"].items():
                    class_names = agent_config.get("class_name", [])
                    names = agent_config.get("name", [])
                    
                    for i, class_name in enumerate(class_names):
                        if class_name not in result["agents"]:
                            result["agents"][class_name] = {
                                "class_name": class_name,
                                "agent_type": agent_type,
                                "instances": [],
                                "found_in": []
                            }
                        
                        if i < len(names):
                            inst = names[i]
                            if inst not in result["agents"][class_name]["instances"]:
                                result["agents"][class_name]["instances"].append(inst)
                        
                        if config_name not in result["agents"][class_name]["found_in"]:
                            result["agents"][class_name]["found_in"].append(config_name)
                            
        except Exception as e:
            print(f"解析 {json_file} 失败: {e}")
    
    return result


def save_json(data, output_path):
    """保存到 JSON"""
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    print(f"已保存: {output_path}")


def main():
    parser = argparse.ArgumentParser(description='UnrealZoo Asset Manager')
    parser.add_argument('--utoc', action='store_true', help='解析 .utoc 文件')
    parser.add_argument('--scan', action='store_true', help='扫描配置文件')
    parser.add_argument('-o', '--output', default=None, help='输出文件')
    args = parser.parse_args()
    
    config = load_config()
    env_path = config.get('env_path', '')
    
    if args.utoc:
        # 查找 utoc 文件
        # 默认路径 - 使用主 utoc 文件（最大的那个）
        default_utoc = Path("/home/ubuntu/UnrealEnv/UnrealZoo_UE5_5_Linux_v1.0.5/Linux/UnrealZoo_UE5_5/Content/Paks/UnrealZoo_UE5_5-Linux.utoc")
        
        if env_path:
            # 查找最大的 utoc 文件
            utoc_files = list(Path(env_path).rglob("*.utoc"))
            if utoc_files:
                utoc_files = sorted(utoc_files, key=lambda x: x.stat().st_size, reverse=True)
        else:
            utoc_files = []
        
        if not utoc_files and default_utoc.exists():
            utoc_files = [default_utoc]
        
        if not utoc_files:
            print("未找到 .utoc 文件")
            return
        
        utoc_path = utoc_files[0]
        result = parse_utoc(utoc_path)
        
        output = args.output or "asset/utoc_index.json"
        save_json(result, output)
        
        # 打印摘要
        print("\n" + "=" * 60)
        print("UTOC 资产索引")
        print("=" * 60)
        for k, v in result["summary"].items():
            print(f"  {k}: {v}")
        print(f"\n地图 ({len(result['maps'])}):")
        for m in result['maps'][:20]:
            print(f"    {m}")
        if len(result['maps']) > 20:
            print(f"    ... 还有 {len(result['maps'])-20} 个")
    
    elif args.scan:
        result = scan_config_files()
        output = args.output or "asset/config_index.json"
        save_json(result, output)
        
        print("\n" + "=" * 60)
        print("配置扫描完成")
        print("=" * 60)
        print(f"地图: {len(result['maps'])}")
        print(f"Agent 类: {len(result['agents'])}")
        print(f"配置文件: {len(result['configs'])}")
    
    else:
        parser.print_help()
        print("\n示例:")
        print("  python asset/unreal_asset_manager.py --utoc")
        print("  python asset/unreal_asset_manager.py --scan")


if __name__ == "__main__":
    main()
