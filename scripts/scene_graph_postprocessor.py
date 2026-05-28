#!/usr/bin/env python3
"""
Scene Graph Post-Processor

This script processes scene_graph_cyberworld.json to automatically add
category and default property fields based on node type.

"""

import json
import argparse
import sys
from typing import Optional, Dict, Any

# Type to Category mapping
TYPE_CATEGORY_MAP: Dict[str, str] = {
    # Building types (category: "building")
    "building": "building",
    "residence": "building",
    "hospital": "building",
    "warehouse": "building",
    
    # Trans_Facility types (category: "trans_facility")
    "bridge": "trans_facility",
    "street_segment": "trans_facility",
    "road_segment": "trans_facility",
    "intersection": "trans_facility",
    
    # Prop types (category: "prop")
    "antenna": "prop",
    "watertower": "prop",
    "statue": "prop",
    "tree": "prop",
    "flowerbed": "prop",
    "garbage": "prop",
    "rubbish": "prop",
    "dustbin": "prop",
    "streetlight": "prop",
    "trafficlight": "prop",
    "mailbox": "prop",
}

# Default fields for Building and Trans_Facility categories
BUILDING_TRANS_DEFAULT_FIELDS: Dict[str, Any] = {
    "status": "undiscovered",
    "visibility": "high",
    "wind_condition": "weak",
    "congestion": "none",
    "is_fire": False,
    "is_spill": False,
}

# Default fields for Prop category
PROP_DEFAULT_FIELDS: Dict[str, Any] = {
    "is_abnormal": False,
}


def classify_type(node_type: str) -> Optional[str]:
    """
    Classify a node type into a category.
    
    Args:
        node_type: The type field value from the node's properties
        
    Returns:
        The category string ("building", "trans_facility", "prop") or None if unknown

    """
    return TYPE_CATEGORY_MAP.get(node_type)


def inject_fields(properties: Dict[str, Any], category: str) -> Dict[str, Any]:
    """
    Inject default fields into properties based on category.
    Does not overwrite existing fields.
    
    Args:
        properties: The node's properties dictionary
        category: The category ("building", "trans_facility", "prop")
        
    Returns:
        Updated properties dictionary
        
    """
    # Add category field if not exists
    if "category" not in properties:
        properties["category"] = category
    
    # Determine which default fields to use
    if category in ("building", "trans_facility"):
        default_fields = BUILDING_TRANS_DEFAULT_FIELDS
    elif category == "prop":
        default_fields = PROP_DEFAULT_FIELDS
    else:
        return properties
    
    # Inject default fields without overwriting existing ones
    for key, value in default_fields.items():
        if key not in properties:
            properties[key] = value
    
    return properties


def process_node(node: Dict[str, Any]) -> Dict[str, Any]:
    """
    Process a single scene graph node.
    
    Args:
        node: The scene graph node dictionary
        
    Returns:
        The processed node (modified in place)
        
    """
    # Get properties, create if not exists
    properties = node.get("properties", {})
    
    # Get the type field
    node_type = properties.get("type")
    if not node_type:
        return node
    
    # Classify the type
    category = classify_type(node_type)
    if category is None:
        return node
    
    # Inject default fields
    inject_fields(properties, category)
    
    return node


def process_scene_graph(input_path: str, output_path: Optional[str] = None) -> Dict[str, int]:
    """
    Process an entire scene graph JSON file.
    
    Args:
        input_path: Path to the input JSON file
        output_path: Path to the output JSON file (defaults to input_path)
        
    Returns:
        Statistics dictionary with processing counts

    """
    if output_path is None:
        output_path = input_path
    
    # Statistics
    stats = {
        "total_nodes": 0,
        "building_count": 0,
        "trans_facility_count": 0,
        "prop_count": 0,
        "unknown_count": 0,
    }
    
    # Read JSON file
    try:
        with open(input_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {input_path}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Failed to parse JSON: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Process nodes
    nodes = data.get("nodes", [])
    stats["total_nodes"] = len(nodes)
    
    for node in nodes:
        properties = node.get("properties", {})
        node_type = properties.get("type")
        
        if node_type:
            category = classify_type(node_type)
            if category == "building":
                stats["building_count"] += 1
            elif category == "trans_facility":
                stats["trans_facility_count"] += 1
            elif category == "prop":
                stats["prop_count"] += 1
            else:
                stats["unknown_count"] += 1
        else:
            stats["unknown_count"] += 1
        
        # Process the node
        process_node(node)
    
    # Write output JSON with formatting
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent='\t', ensure_ascii=False)
    except IOError as e:
        print(f"Error: Failed to write output file: {e}", file=sys.stderr)
        sys.exit(1)
    
    return stats


def main():
    """
    Command-line entry point.

    """
    parser = argparse.ArgumentParser(
        description="Post-process scene graph JSON files to add category and default fields."
    )
    parser.add_argument(
        "input_file",
        help="Path to the input JSON file"
    )
    parser.add_argument(
        "-o", "--output",
        dest="output_file",
        default=None,
        help="Path to the output JSON file (defaults to overwriting input file)"
    )
    
    args = parser.parse_args()
    
    print(f"Processing: {args.input_file}")
    
    stats = process_scene_graph(args.input_file, args.output_file)
    
    # Output statistics
    print("\n=== Processing Statistics ===")
    print(f"Total nodes processed: {stats['total_nodes']}")
    print(f"  - Building:       {stats['building_count']}")
    print(f"  - Trans_Facility: {stats['trans_facility_count']}")
    print(f"  - Prop:           {stats['prop_count']}")
    print(f"  - Unknown/Other:  {stats['unknown_count']}")
    
    output_file = args.output_file or args.input_file
    print(f"\nOutput written to: {output_file}")


if __name__ == "__main__":
    main()
