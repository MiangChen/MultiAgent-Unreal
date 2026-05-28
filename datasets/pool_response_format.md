## Core Directives & Planning Logic

1. Generate a complete task graph as a JSON object, including both the `task_graph` and a `meta` summary.
2. Nodes in the graph MUST follow the atomic task format. Edges define control and data flow.
3. In shared_skill_groups, each group lists skills that must be executed by the same robot. Within a group, every entry "T<task_id>.<skill_index>" must come from a different task, where <skill_index> is the 0-based index of that skill in the task's required_skills list. All tasks within a single group MUST be parameter-ready at the same time (do not mix concrete and 'tbd' tasks). All skills executed by the same robot must be placed in a single group and cannot be split across multiple groups.

### Result:

```json
{
  "meta": {
    "description": "A very concise one-sentence summary of the overall plan.",
    "shared_skill_groups": [
      ["T<task_id>.<skill_index>", "T<task_id>.<skill_index>"],
      ["T1.0", "T2.1", "T3.0", "..."]
    ]
  },
  "task_graph": {
    "nodes": [
      {
        "task_id": "T1",
        "description": "A clear, concise description of the task.",
        "location": "<The specific, standardized location_identifier>",
        "required_skills": [
          {
            "skill_name": "skill_with_specific_parameters",
            "assigned_robot_type": ["robot_type_1", "robot_type_2"],
            "assigned_robot_count": "<integer>"
          }
        ],
        "produces": ["fact_name1", "fact_name2"]
      },
      {
        "task_id": "T2",
        "description": "A task that runs if a condition is met.",
        "location": "tbd:location_identifier",
        "required_skills": [
          {
            "...": "...",
            "assigned_robot_count": "<tbd:integer>"
          }
        ]
      }
    ],
    "edges": [
      {
        "from": "T1",
        "to": "T2",
        "type": "conditional",
        "condition": "fact_name1 != null"
      }
    ]
  }
}
```
