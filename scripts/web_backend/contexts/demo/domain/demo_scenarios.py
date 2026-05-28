from __future__ import annotations

DEMO_SCENARIOS = {
    "air_search": {
        "id": "air_search",
        "name": "空中协同搜索",
        "icon": "🔍",
        "difficulty": 1,
        "difficulty_label": "入门",
        "description": "2 架 UAV 协同搜索城区中的红色货箱，体验基础的技能执行流程。",
        "robot_types": ["UAV-1", "UAV-2"],
        "visualization": {
            "kind": "timeline",
            "source": "skill_list",
            "key": "uav_search",
            "title": "Air Search Timeline",
            "subtitle": "UAV 直接执行搜索技能",
            "badge": "direct execute"
        },
        "mermaid": "",
        "steps": [
            {
                "title": "发送搜索任务",
                "description": "这是最直接的一类演示：后端不经过人工审核，直接把搜索技能下发给仿真端执行。点击下方按钮后，UAV-1 和 UAV-2 会同时起飞，并自动分区搜索城区中的红色货箱。",
                "ue_tip": "观察两架无人机同时升空。按 Tab 可切换到无人机视角。",
                "action_type": "skill_list",
                "action_key": "uav_search",
                "action_label": "发送搜索任务"
            },
            {
                "title": "观察执行",
                "description": "两架 UAV 会自动将城区分成左右两半进行搜索。UAV-1 负责西区，UAV-2 负责东区。你只需要观察它们的起飞、巡航、搜索与反馈回传过程。",
                "ue_tip": "按 Tab 切换到 UAV 视角观察巡飞路径。搜索完成后，右侧日志会显示回传消息。",
                "action_type": "info",
                "action_label": "任务已发送，观察执行中"
            },
            {
                "title": "📋 总结",
                "description": "搜索完成！你已体验了「直接执行」类型的技能。这种模式下，命令从后端直接发送到仿真端执行，无需人工确认。",
                "ue_tip": "按 0 或 V 返回 Spectator 全局视角。",
                "action_type": "info",
                "action_label": "演示完成 ✓"
            }
        ]
    },
    "fire_rescue": {
        "id": "fire_rescue",
        "name": "消防救援规划",
        "icon": "🚒",
        "difficulty": 2,
        "difficulty_label": "进阶",
        "description": "查看火灾救援任务图 (Task Graph)，学习 DAG 可视化和人工审核 (HITL) 流程。",
        "robot_types": ["UAV", "Quadruped", "Humanoid"],
        "visualization": {
            "kind": "mermaid"
        },
        "mermaid": """graph TD
    T1["T1: 无人机飞往火灾区域<br/>UAV → building_A_roof"]
    T2["T2: 扫描获取热成像数据<br/>UAV perceive(thermal_scan)"]
    T3["T3: 机器狗导航至入口<br/>Quadruped ×2"]
    T4["T4: 广播火灾位置<br/>UAV broadcast"]
    T5["T5: 执行灭火操作<br/>Quadruped ×2"]
    T6["T6: 疏散被困人员<br/>Humanoid"]
    T1 -->|sequential| T2
    T2 -->|sequential| T4
    T2 -.->|parallel| T3
    T4 -->|conditional: fire_info_shared| T5
    T3 -->|sequential| T5
    T2 -->|conditional: victim_locations| T6
    style T1 fill:#0d47a1,stroke:#42a5f5,color:#fff
    style T2 fill:#0d47a1,stroke:#42a5f5,color:#fff
    style T3 fill:#b71c1c,stroke:#ef5350,color:#fff
    style T4 fill:#0d47a1,stroke:#42a5f5,color:#fff
    style T5 fill:#b71c1c,stroke:#ef5350,color:#fff
    style T6 fill:#4a148c,stroke:#ab47bc,color:#fff""",
        "steps": [
            {
                "title": "任务图审核",
                "description": "下方是 AI 规划系统生成的消防救援任务图 (Mermaid 渲染)。\n\n任务间有三种依赖关系：\n• sequential (实线) — 必须先完成上游任务\n• parallel (虚线) — 可同时执行\n• conditional (标注条件) — 满足条件后触发\n\n点击下方按钮后，任务图会发送到 UE5。然后你可以直接在 UE5 中打开任务图、查看节点依赖，并完成第一次 HITL 审核。",
                "ue_tip": "先点击发送任务图。收到通知后，在 UE5 中按 Z 打开 Task Graph 模态窗口查看 DAG，并点击 Confirm 完成任务图审核。",
                "action_type": "task_graph",
                "action_key": "fire_rescue",
                "action_label": "发送消防救援任务图"
            },
            {
                "title": "技能分配审核",
                "description": "任务图确认后，后端会自动把粗粒度任务翻译成具备时序和指定机器人的技能分配 (Skill Allocation / 甘特图)。这一页只保留第二次 HITL 审核：查看技能时间线，确认每台机器人的执行顺序是否合理。",
                "ue_tip": "在 UE5 中按 N 打开 Skill Allocation 模态窗口，查看甘特图并点击 Confirm。确认后，无人机会直接起飞开始执行消防救援流程。",
                "action_type": "info",
                "action_label": "在 UE5 中按 N 审核甘特图"
            },
            {
                "title": "📋 总结",
                "description": "恭喜你走通了完整的连贯决策流！\n下发 Task Graph -> 人类审批 -> 自动计算甘特图 -> 下发 Skill Allocation -> 人类二次审批 -> 实际执行。",
                "ue_tip": "按 N 关闭甘特图窗口。接下来你可以去体验右下角的「全流程协作」感受更多机器人的大场面。",
                "action_type": "info",
                "action_label": "演示完成 ✓"
            }
        ]
    },
    "full_collaboration": {
        "id": "full_collaboration",
        "name": "全流程协作",
        "icon": "🤖",
        "difficulty": 3,
        "difficulty_label": "完整",
        "description": "体验完整的多机器人协作流程：查看技能分配甘特图 → 确认 → UAV 搜索 → UGV+Humanoid 装卸运输。",
        "robot_types": ["UAV-1", "UAV-2", "UGV-1", "Humanoid-1"],
        "visualization": {
            "kind": "timeline",
            "source": "skill_allocation",
            "key": "complete_test",
            "title": "Full Collaboration Timeline",
            "subtitle": "完整协作方案的技能分配时间线",
            "badge": "HITL review"
        },
        "mermaid": """gantt
    title Multi-Robot Collaboration Timeline
    dateFormat X
    axisFormat %s
    section UAV-1
        Takeoff           :a1, 0, 1
        Search West       :a2, 1, 2
        Return Home       :a3, 2, 3
    section UAV-2
        Takeoff           :b1, 0, 1
        Search East       :b2, 1, 2
        Return Home       :b3, 2, 3
    section UGV-1
        Navigate to Box   :c1, 2, 3
        Transport          :c2, 4, 5
    section Humanoid-1
        Navigate to Box    :d1, 2, 3
        Load to UGV        :d2, 3, 4
        Unload at Dest     :d3, 5, 6""",
        "steps": [
            {
                "title": "协作方案审核",
                "description": "下方是完整的多阶段协作方案时间线 (Mermaid 甘特图)：\n\n• 阶段0: UAV 起飞\n• 阶段1: UAV 协同搜索 RedBox\n• 阶段2: UGV/Humanoid 导航到目标，UAV 返航\n• 阶段3: Humanoid 将 RedBox 装到 UGV\n• 阶段4: 运输到目的地\n• 阶段5: Humanoid 卸载 RedBox\n\n点击下方按钮后，这份完整的技能分配会发送到 UE5。你可以直接在 UE5 中打开甘特图并完成审核确认。",
                "ue_tip": "先点击发送完整协作方案。收到通知后，在 UE5 中按 N 打开 Skill Allocation 模态窗口查看甘特图，并点击 Confirm 开始执行。",
                "action_type": "skill_allocation",
                "action_key": "complete_test",
                "action_label": "发送完整协作方案"
            },
            {
                "title": "观察执行",
                "description": "确认后机器人开始执行任务：UAV 起飞搜索、UGV 导航、Humanoid 搬运、装卸与运输。右侧通信日志会持续显示每个阶段的反馈，你可以把这一步当成完整演示的主观察页。",
                "ue_tip": "按 Tab/C 切换 Agent 视角，按 [ 切换高亮圈显示，按 P 暂停/恢复执行。",
                "action_type": "info",
                "action_label": "观察机器人执行"
            },
            {
                "title": "📋 总结",
                "description": "恭喜！你已完成全部 Demo！\n\n✅ 直接执行（Skill List）\n✅ 任务图审核（Task Graph + HITL）\n✅ 技能分配审核（Skill Allocation + Gantt Chart）\n✅ 多机器人协同执行（UAV/UGV/Humanoid）",
                "ue_tip": "现在你可以自由探索：用控制台发送更多命令，按 M 进入编辑模式，按 , 进入标注模式。",
                "action_type": "info",
                "action_label": "全部演示完成 🎉"
            }
        ]
    }
}


