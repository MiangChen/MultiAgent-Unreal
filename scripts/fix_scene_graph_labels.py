#!/usr/bin/env python3
"""
修复 scene_graph_cyberworld.json 中的 label 问题：
1. 为 building 类型的节点添加 label (格式: Building-序号)
2. 修复 intersection 的 label (格式: Intersection-序号，序号是同类型节点的编号)
3. 修复 street_segment 的 label (格式: Street_segment-序号，序号是同类型节点的编号)
"""

import json
import sys
from pathlib import Path


def fix_labels(input_file: str, output_file: str = None):
    """修复 scene graph 中的 label"""
    
    # 读取 JSON 文件
    with open(input_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    nodes = data.get('nodes', [])
    
    # 按类型分组统计
    type_counters = {
        'building': 0,
        'intersection': 0,
        'street_segment': 0
    }
    
    # 遍历所有节点，修复 label
    for node in nodes:
        properties = node.get('properties', {})
        node_type = properties.get('type', '')
        
        if node_type == 'building':
            # building 需要添加 label
            type_counters['building'] += 1
            properties['label'] = f"Building-{type_counters['building']}"
            
        elif node_type == 'intersection':
            # intersection 需要修复 label 序号
            type_counters['intersection'] += 1
            properties['label'] = f"Intersection-{type_counters['intersection']}"
            
        elif node_type == 'street_segment':
            # street_segment 需要修复 label 序号
            type_counters['street_segment'] += 1
            properties['label'] = f"Street_segment-{type_counters['street_segment']}"
    
    # 输出统计信息
    print(f"处理完成:")
    print(f"  - Building: {type_counters['building']} 个节点已添加 label")
    print(f"  - Intersection: {type_counters['intersection']} 个节点已修复 label")
    print(f"  - Street_segment: {type_counters['street_segment']} 个节点已修复 label")
    
    # 写入输出文件
    if output_file is None:
        output_file = input_file
    
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent='\t', ensure_ascii=False)
    
    print(f"\n已保存到: {output_file}")


def main():
    # 默认文件路径
    default_input = "unreal_project/datasets/scene_graph_cyberworld.json"
    
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    else:
        input_file = default_input
    
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    if not Path(input_file).exists():
        print(f"错误: 文件不存在 - {input_file}")
        sys.exit(1)
    
    fix_labels(input_file, output_file)


if __name__ == "__main__":
    main()
