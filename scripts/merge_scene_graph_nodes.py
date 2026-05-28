#!/usr/bin/env python3
"""
从 backup 文件中提取非 building/intersection/street_segment 节点，
添加到目标文件中，并修复它们的 id 和 label。

label 格式: Type-序号 (首字母大写，序号是同类型节点的编号)
"""

import json
import sys
from pathlib import Path
from collections import defaultdict


def merge_nodes(backup_file: str, target_file: str, output_file: str = None):
    """合并节点"""
    
    # 读取 backup 文件
    with open(backup_file, 'r', encoding='utf-8') as f:
        backup_data = json.load(f)
    
    # 读取目标文件
    with open(target_file, 'r', encoding='utf-8') as f:
        target_data = json.load(f)
    
    # 获取目标文件中的最大 id
    max_id = 0
    for node in target_data['nodes']:
        try:
            node_id = int(node['id'])
            max_id = max(max_id, node_id)
        except (ValueError, TypeError):
            pass
    
    print(f"目标文件当前最大 id: {max_id}")
    
    # 排除的类型
    excluded_types = {'building', 'intersection', 'street_segment'}
    
    # 从 backup 中提取非排除类型的节点
    other_nodes = []
    for node in backup_data['nodes']:
        node_type = node['properties'].get('type', '')
        if node_type not in excluded_types:
            other_nodes.append(node)
    
    print(f"从 backup 中提取了 {len(other_nodes)} 个其他类型节点")
    
    # 按类型分组统计，用于生成正确的 label 序号
    type_counters = defaultdict(int)
    
    # 为每个节点分配新的 id 和 label
    new_id = max_id + 1
    for node in other_nodes:
        node_type = node['properties'].get('type', '')
        
        # 分配新 id
        node['id'] = str(new_id)
        new_id += 1
        
        # 生成 label (首字母大写 + 序号)
        type_counters[node_type] += 1
        label_type = node_type.capitalize()
        node['properties']['label'] = f"{label_type}-{type_counters[node_type]}"
    
    # 添加到目标数据
    target_data['nodes'].extend(other_nodes)
    
    # 输出统计信息
    print(f"\n添加的节点类型统计:")
    for node_type, count in sorted(type_counters.items()):
        print(f"  - {node_type}: {count} 个")
    
    print(f"\n目标文件节点总数: {len(target_data['nodes'])}")
    
    # 写入输出文件
    if output_file is None:
        output_file = target_file
    
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(target_data, f, indent='\t', ensure_ascii=False)
    
    print(f"\n已保存到: {output_file}")


def main():
    backup_file = "unreal_project/datasets/scene_graph_cyberworld backup.json"
    target_file = "unreal_project/datasets/scene_graph_cyberworld.json"
    
    if len(sys.argv) > 1:
        backup_file = sys.argv[1]
    if len(sys.argv) > 2:
        target_file = sys.argv[2]
    
    output_file = sys.argv[3] if len(sys.argv) > 3 else None
    
    if not Path(backup_file).exists():
        print(f"错误: backup 文件不存在 - {backup_file}")
        sys.exit(1)
    
    if not Path(target_file).exists():
        print(f"错误: 目标文件不存在 - {target_file}")
        sys.exit(1)
    
    merge_nodes(backup_file, target_file, output_file)


if __name__ == "__main__":
    main()
