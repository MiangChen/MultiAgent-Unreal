#!/usr/bin/env python3
"""
MultiAgent-Unreal Interactive Demo Guide
=========================================

引导演示模块，可被 mock_backend.py 导入使用。

当作为模块导入时：
  - 提供 DEMO_SCENARIOS、DEMO_HTML_TEMPLATE
  - 提供 handle_demo_action() 处理函数

当单独运行时：
  - 启动独立服务器，通过 HTTP 代理调用 mock_backend

用法 (集成模式 - 推荐):
  # mock_backend.py 内部 import demo_guided
  python3 scripts/mock_backend.py --port 8081
  # 浏览器访问 http://localhost:8081  (Console tab)
  # 浏览器访问 http://localhost:8081/demo  (Demo tab)

用法 (独立模式):
  python3 scripts/mock_backend.py --port 8081
  python3 scripts/demo_guided.py --port 8082 --backend http://localhost:8081
"""

import argparse
import json
import sys
from datetime import datetime

# ========== 引导演示场景 ==========
DEMO_SCENARIOS = {
    "air_search": {
        "id": "air_search",
        "name": "空中协同搜索",
        "icon": "🔍",
        "difficulty": 1,
        "difficulty_label": "入门",
        "description": "2 架 UAV 协同搜索城区中的红色货箱，体验基础的技能执行流程。",
        "robot_types": ["UAV-1", "UAV-2"],
        "mermaid": "",
        "steps": [
            {
                "title": "起飞准备",
                "description": "UAV-1 和 UAV-2 同时起飞，升至默认巡航高度。起飞后无人机将悬停在出发点上空。",
                "ue_tip": "观察两架无人机同时升空。按 Tab 可切换到无人机视角。",
                "action_type": "skill_list",
                "action_key": "uav_search",
                "action_label": "发送搜索任务"
            },
            {
                "title": "区域搜索",
                "description": "两架 UAV 将城区分成左右两半进行搜索。UAV-1 负责西区，UAV-2 负责东区。目标是找到红色货箱 (RedBox)。",
                "ue_tip": "按 Tab 切换到 UAV 视角观察巡飞路径。搜索完成后控制台会收到反馈消息。",
                "action_type": "info",
                "action_label": "（任务已在上一步发送，等待完成）"
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
        "robot_types": ["UAV", "RobotDog", "Humanoid"],
        "mermaid": """graph TD
    T1["T1: 无人机飞往火灾区域<br/>UAV → building_A_roof"]
    T2["T2: 扫描获取热成像数据<br/>UAV perceive(thermal_scan)"]
    T3["T3: 机器狗导航至入口<br/>RobotDog ×2"]
    T4["T4: 广播火灾位置<br/>UAV broadcast"]
    T5["T5: 执行灭火操作<br/>RobotDog ×2"]
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
                "title": "查看任务流程图",
                "description": "下方是 AI 规划系统生成的消防救援任务图 (Mermaid 渲染)。\n\n任务间有三种依赖关系：\n• sequential (实线) — 必须先完成上游任务\n• parallel (虚线) — 可同时执行\n• conditional (标注条件) — 满足条件后触发",
                "ue_tip": "先在这里理解任务流程，然后点击下一步发送到 UE5。",
                "action_type": "info",
                "action_label": "查看流程图"
            },
            {
                "title": "发送任务图到 UE5",
                "description": "将任务图发送到 UE5 仿真端。UE5 收到后会保存到临时数据中，等待你在 UE5 界面中查看和审核。",
                "ue_tip": "发送后，UE5 右下角会弹出通知。",
                "action_type": "task_graph",
                "action_key": "fire_rescue",
                "action_label": "发送消防救援任务图"
            },
            {
                "title": "在 UE5 中查看任务图",
                "description": "在 UE5 中打开任务图可视化面板，你会看到一个 DAG（有向无环图）展示任务间的依赖关系。可以拖拽节点、查看每个任务的详情。",
                "ue_tip": "按 Z 打开 Task Graph 模态窗口，查看任务节点和连线。",
                "action_type": "info",
                "action_label": "在 UE5 中按 Z 查看"
            },
            {
                "title": "审核决策",
                "description": "这就是 HITL（Human-In-The-Loop）流程：AI 生成方案，人类审核后确认或拒绝。确认后系统才会生成具体的技能分配。",
                "ue_tip": "在模态窗口中点击 Confirm（确认）或 Reject（拒绝）按钮。",
                "action_type": "info",
                "action_label": "在 UE5 中审核"
            },
            {
                "title": "📋 总结",
                "description": "你已体验了「HITL 审核」流程！在实际应用中，AI 规划器会自动生成任务图，人类操作员通过可视化界面审核后批准执行。",
                "ue_tip": "按 Z 关闭任务图。试试发送其他任务图（Warehouse Patrol, Cargo Delivery 等）。",
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
                "title": "查看协作时间线",
                "description": "下方是完整的多阶段协作方案时间线 (Mermaid 甘特图)：\n\n• 阶段0: UAV 起飞\n• 阶段1: UAV 协同搜索 RedBox\n• 阶段2: UGV/Humanoid 导航到目标，UAV 返航\n• 阶段3: Humanoid 将 RedBox 装到 UGV\n• 阶段4: 运输到目的地\n• 阶段5: Humanoid 卸载 RedBox",
                "ue_tip": "先在这里理解整体流程，然后发送到 UE5。",
                "action_type": "info",
                "action_label": "查看时间线"
            },
            {
                "title": "发送技能分配",
                "description": "将完整的多阶段协作方案发送到 UE5。UE5 收到后会显示通知，你需要在甘特图面板中审核并确认。",
                "ue_tip": "发送后，UE5 右下角会弹出通知。",
                "action_type": "skill_allocation",
                "action_key": "complete_test",
                "action_label": "发送完整协作方案"
            },
            {
                "title": "查看甘特图",
                "description": "在 UE5 中打开技能分配面板。你会看到一个甘特图，展示各机器人在每个时间步的任务分配。每个色块代表一个技能。",
                "ue_tip": "按 N 打开 Skill Allocation 模态窗口，查看甘特图。",
                "action_type": "info",
                "action_label": "在 UE5 中按 N 查看"
            },
            {
                "title": "确认执行",
                "description": "审核甘特图中的分配方案。确认后，系统会按时间步依次执行每个技能。你可以观察不同类型机器人的协同配合。",
                "ue_tip": "在模态窗口中点击 Confirm 确认。执行开始后，按 Tab 切换视角观察不同机器人。",
                "action_type": "info",
                "action_label": "在 UE5 中确认执行"
            },
            {
                "title": "观察执行",
                "description": "确认后机器人开始执行任务：UAV 起飞搜索、UGV 导航、Humanoid 搬运。每个阶段的反馈会实时显示在右侧通信日志中。",
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


# ========== Demo HTML 模板 (无 <html>/<body> 外壳，只有内容 + script) ==========
# mock_backend.py 会把这段嵌入到自己的页面框架中
DEMO_HTML_CONTENT = '''
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');

        /* ===== Landing ===== */
        .landing { height: 100%; padding: 48px 40px; overflow-y: auto; }
        .landing-header { text-align: center; margin-bottom: 40px; }
        .landing-header h1 {
            font-size: 36px; font-weight: 700; margin-bottom: 12px;
            background: linear-gradient(135deg, #ffd700 0%, #ff6b6b 50%, #00d9ff 100%);
            -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text;
            font-family: 'Inter', -apple-system, sans-serif;
        }
        .landing-header p { color: #667799; font-size: 15px; font-family: 'Inter', sans-serif; }
        .cards {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(340px, 1fr));
            gap: 28px; max-width: 1200px; margin: 0 auto;
        }
        .demo-card {
            background: linear-gradient(145deg, #131836, #171e40);
            border: 1px solid #1e2a4a; border-radius: 20px;
            padding: 32px; cursor: pointer; transition: all 0.35s;
            position: relative; overflow: hidden;
        }
        .demo-card::before {
            content: ''; position: absolute; top: 0; left: 0; right: 0; height: 3px;
            background: linear-gradient(90deg, var(--c), transparent);
        }
        .demo-card:hover {
            transform: translateY(-8px); border-color: var(--c);
            box-shadow: 0 16px 48px rgba(0,0,0,0.5);
        }
        .demo-card-icon { font-size: 48px; margin-bottom: 16px; }
        .demo-card-title { font-size: 22px; font-weight: 700; margin-bottom: 8px; }
        .demo-card-diff {
            display: inline-flex; align-items: center; gap: 4px;
            font-size: 12px; margin-bottom: 14px; padding: 4px 12px;
            border-radius: 12px; background: rgba(255,255,255,0.04);
        }
        .demo-star { color: #ffd700; } .demo-star.off { color: #222; }
        .demo-card-desc { color: #778899; font-size: 13px; line-height: 1.7; margin-bottom: 18px; }
        .demo-card-robots { display: flex; gap: 6px; flex-wrap: wrap; }
        .demo-card-robots span {
            font-size: 11px; padding: 3px 10px; border-radius: 6px;
            background: rgba(0,217,255,0.08); color: #00d9ff; border: 1px solid rgba(0,217,255,0.15);
        }
        .demo-card-enter {
            position: absolute; bottom: 24px; right: 24px;
            font-size: 14px; color: var(--c); font-weight: 600;
            opacity: 0; transition: all 0.3s;
        }
        .demo-card:hover .demo-card-enter { opacity: 1; transform: translateX(4px); }

        /* ===== Guide ===== */
        .guide { display: none; height: 100%; flex-direction: column; }
        .guide.active { display: flex; }
        .guide-header {
            padding: 16px 28px; background: #131836; border-bottom: 1px solid #1e2a4a;
            display: flex; align-items: center; gap: 14px;
        }
        .back-btn {
            padding: 7px 14px; background: #1a2248; border: 1px solid #1e2a4a;
            border-radius: 8px; color: #8899aa; cursor: pointer; font-size: 13px;
            transition: all 0.2s;
        }
        .back-btn:hover { background: #1e2a4a; color: #fff; }
        .guide-title { font-size: 17px; font-weight: 700; }
        .guide-sub { font-size: 13px; color: #556; margin-left: auto; }
        .progress { display: flex; padding: 14px 28px; background: #0d1126; gap: 4px; }
        .pbar { flex: 1; height: 4px; border-radius: 2px; background: #1e2a4a; transition: background 0.4s; }
        .pbar.done { background: #00d9ff; } .pbar.now { background: linear-gradient(90deg, #00d9ff, #1e2a4a); }
        .step-area { flex: 1; display: flex; overflow: hidden; }
        .step-main { flex: 1; padding: 36px 48px; overflow-y: auto; display: flex; flex-direction: column; gap: 22px; }
        .step-num { font-size: 11px; color: #445566; text-transform: uppercase; letter-spacing: 2px; font-weight: 600; }
        .step-title { font-size: 28px; font-weight: 700; color: #eef; }
        .step-desc { font-size: 15px; color: #99aabb; line-height: 1.85; white-space: pre-wrap; }
        .ue-tip {
            background: linear-gradient(135deg, #151d3e, #0f1a38);
            border: 1px solid #1e2a4a; border-radius: 14px; padding: 18px 22px; display: flex; gap: 14px;
        }
        .ue-tip-icon { font-size: 24px; flex-shrink: 0; }
        .ue-tip-label { font-size: 10px; color: #ffd700; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; margin-bottom: 4px; }
        .ue-tip-text { font-size: 13px; color: #bbccdd; line-height: 1.7; }
        .ue-tip-text kbd {
            display: inline-block; padding: 2px 8px; margin: 0 3px;
            background: #1a2248; border: 1px solid #2a3a5a; border-radius: 4px;
            font-family: 'JetBrains Mono', monospace; font-size: 12px; color: #ffd700;
        }
        .mermaid-wrap { background: #0d1126; border: 1px solid #1e2a4a; border-radius: 14px; padding: 24px; overflow-x: auto; }
        .mermaid-wrap .mermaid { display: flex; justify-content: center; }
        .step-actions { display: flex; gap: 12px; align-items: center; }
        .act-btn {
            padding: 14px 28px; border: none; border-radius: 12px;
            font-size: 15px; font-weight: 600; cursor: pointer;
            transition: all 0.25s; display: flex; align-items: center; gap: 8px;
        }
        .act-btn.primary {
            background: linear-gradient(135deg, #e94560, #ff6b6b);
            color: #fff; box-shadow: 0 4px 20px rgba(233,69,96,0.3);
        }
        .act-btn.primary:hover { transform: translateY(-2px); box-shadow: 0 6px 28px rgba(233,69,96,0.5); }
        .act-btn.primary:disabled { opacity: 0.5; cursor: not-allowed; transform: none; box-shadow: none; }
        .nav-row { display: flex; gap: 12px; margin-top: auto; padding-top: 24px; }
        .nav-btn {
            padding: 10px 22px; border: 1px solid #1e2a4a; border-radius: 8px;
            background: #131836; color: #8899aa; cursor: pointer; font-size: 13px; transition: all 0.2s;
        }
        .nav-btn:hover { background: #1e2a4a; color: #fff; }
        .nav-btn:disabled { opacity: 0.3; cursor: not-allowed; }
        .nav-spacer { flex: 1; }
        .demo-sidebar { width: 380px; background: #080c18; border-left: 1px solid #1e2a4a; display: flex; flex-direction: column; }
        .demo-sidebar-hdr { padding: 12px 16px; background: #131836; border-bottom: 1px solid #1e2a4a; font-size: 13px; color: #556677; font-weight: 600; }
        .demo-sidebar-msgs { flex: 1; overflow-y: auto; padding: 12px; }
        .demo-sidebar-empty { text-align: center; color: #223344; padding: 40px 16px; font-size: 13px; }
        .demo-msg {
            background: #131836; border-radius: 8px; padding: 12px;
            margin-bottom: 10px; border-left: 3px solid #e94560;
            animation: demoSlideIn 0.3s ease; font-size: 12px;
        }
        @keyframes demoSlideIn { from { opacity: 0; transform: translateX(-10px); } to { opacity: 1; } }
        .demo-msg .dir { font-size: 10px; color: #e94560; font-weight: 700; margin-bottom: 4px; }
        .demo-msg .hdr { display: flex; justify-content: space-between; margin-bottom: 6px; }
        .demo-msg .badge { background: #4a1942; color: #e94560; padding: 2px 8px; border-radius: 4px; font-weight: 700; font-size: 11px; }
        .demo-msg .time { color: #334455; font-size: 11px; }
        .demo-msg .body {
            background: #080c18; padding: 8px; border-radius: 4px;
            font-family: 'JetBrains Mono', monospace; font-size: 11px;
            white-space: pre-wrap; word-break: break-all; max-height: 140px; overflow-y: auto; color: #8899aa;
        }
    </style>

    <!-- Landing -->
    <div class="landing" id="demo-landing">
        <div class="landing-header">
            <h1>🎮 Interactive Demo</h1>
            <p>选择一个场景，跟着引导一步步体验多机器人协同仿真</p>
        </div>
        <div class="cards" id="demo-cards"></div>
    </div>

    <!-- Guide -->
    <div class="guide" id="demo-guide">
        <div class="guide-header">
            <button class="back-btn" onclick="demoBack()">← 返回</button>
            <span class="guide-title" id="dg-title"></span>
            <span class="guide-sub" id="dg-sub"></span>
        </div>
        <div class="progress" id="dg-progress"></div>
        <div class="step-area">
            <div class="step-main">
                <div class="step-num" id="ds-num"></div>
                <div class="step-title" id="ds-title"></div>
                <div class="step-desc" id="ds-desc"></div>
                <div class="mermaid-wrap" id="ds-mermaid" style="display:none">
                    <pre class="mermaid" id="ds-mermaid-code"></pre>
                </div>
                <div class="ue-tip">
                    <span class="ue-tip-icon">🎮</span>
                    <div>
                        <div class="ue-tip-label">UE5 操作提示</div>
                        <div class="ue-tip-text" id="ds-uetip"></div>
                    </div>
                </div>
                <div class="step-actions" id="ds-actions"></div>
                <div class="nav-row">
                    <button class="nav-btn" id="dn-prev" onclick="demoNav(-1)">← 上一步</button>
                    <div class="nav-spacer"></div>
                    <button class="nav-btn" id="dn-next" onclick="demoNav(1)">下一步 →</button>
                </div>
            </div>
            <div class="demo-sidebar">
                <div class="demo-sidebar-hdr">📨 实时日志</div>
                <div class="demo-sidebar-msgs" id="demo-sb">
                    <div class="demo-sidebar-empty">发送命令后，日志将显示在这里</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        const demoScenarios = {{DEMO_SCENARIOS}};
        const demoColors = ['#00d9ff', '#e94560', '#ffd700'];
        let demoCur = null, demoStep = 0, demoSent = new Set();

        (function buildDemoCards() {
            const box = document.getElementById('demo-cards'); let i = 0;
            for (const [key, sc] of Object.entries(demoScenarios)) {
                const c = demoColors[i % demoColors.length];
                const stars = Array.from({length:3}, (_,j) =>
                    '<span class="demo-star' + (j < sc.difficulty ? '' : ' off') + '">' + String.fromCharCode(9733) + '</span>'
                ).join('');
                const robots = sc.robot_types.map(r => '<span>' + r + '</span>').join('');
                const el = document.createElement('div');
                el.className = 'demo-card'; el.style.setProperty('--c', c);
                el.onclick = () => demoEnter(key);
                el.innerHTML =
                    '<div class="demo-card-icon">' + sc.icon + '</div>' +
                    '<div class="demo-card-title">' + sc.name + '</div>' +
                    '<div class="demo-card-diff">' + stars + ' <span style="margin-left:6px;color:#667">' + sc.difficulty_label + '</span></div>' +
                    '<div class="demo-card-desc">' + sc.description + '</div>' +
                    '<div class="demo-card-robots">' + robots + '</div>' +
                    '<div class="demo-card-enter">开始体验 →</div>';
                box.appendChild(el); i++;
            }
        })();

        function demoEnter(key) {
            demoCur = demoScenarios[key]; demoStep = 0; demoSent = new Set();
            document.getElementById('demo-landing').style.display = 'none';
            document.getElementById('demo-guide').classList.add('active');
            document.getElementById('dg-title').textContent = demoCur.icon + ' ' + demoCur.name;
            const pb = document.getElementById('dg-progress'); pb.innerHTML = '';
            demoCur.steps.forEach((_,i) => {
                const b = document.createElement('div'); b.className = 'pbar'; b.id = 'dpb-' + i;
                pb.appendChild(b);
            });
            demoRender();
        }

        function demoBack() {
            document.getElementById('demo-guide').classList.remove('active');
            document.getElementById('demo-landing').style.display = '';
            demoCur = null;
            document.getElementById('demo-sb').innerHTML = '<div class="demo-sidebar-empty">发送命令后，日志将显示在这里</div>';
        }

        function demoFmtTip(t) {
            return t.replace(/(Tab|Ctrl|Space|Shift|Confirm|Reject|Edit)/g, '<kbd>$1</kbd>')
                    .replace(/(按)([A-Z0-9,]{1,3})/g, '$1 <kbd>$2</kbd>');
        }

        function demoRender() {
            if (!demoCur) return;
            const s = demoCur.steps[demoStep], total = demoCur.steps.length;
            for (let i = 0; i < total; i++) {
                document.getElementById('dpb-' + i).className = 'pbar' + (i < demoStep ? ' done' : i === demoStep ? ' now' : '');
            }
            document.getElementById('ds-num').textContent = 'STEP ' + (demoStep+1) + ' / ' + total;
            document.getElementById('dg-sub').textContent = (demoStep+1) + ' / ' + total;
            document.getElementById('ds-title').textContent = s.title;
            document.getElementById('ds-desc').textContent = s.description;
            document.getElementById('ds-uetip').innerHTML = demoFmtTip(s.ue_tip);

            const mW = document.getElementById('ds-mermaid'), mC = document.getElementById('ds-mermaid-code');
            if (demoCur.mermaid && demoStep === 0) {
                mW.style.display = ''; mC.textContent = demoCur.mermaid;
                mC.removeAttribute('data-processed'); mermaid.run({nodes: [mC]});
            } else { mW.style.display = 'none'; }

            const ad = document.getElementById('ds-actions'); ad.innerHTML = '';
            if (s.action_type !== 'info') {
                const done = demoSent.has(demoStep);
                const btn = document.createElement('button'); btn.className = 'act-btn primary';
                btn.disabled = done; btn.textContent = done ? '✅ 已发送' : '🚀 ' + s.action_label;
                btn.onclick = () => demoSend(demoStep, s); ad.appendChild(btn);
            } else {
                const sp = document.createElement('span');
                sp.style.cssText = 'color:#445566; font-size:14px;'; sp.textContent = s.action_label;
                ad.appendChild(sp);
            }

            document.getElementById('dn-prev').disabled = demoStep === 0;
            const nb = document.getElementById('dn-next');
            if (demoStep >= total - 1) { nb.textContent = '完成 ✓'; nb.onclick = () => demoBack(); }
            else { nb.textContent = '下一步 →'; nb.onclick = () => demoNav(1); nb.disabled = false; }
        }

        function demoNav(d) {
            if (!demoCur) return;
            const n = demoStep + d; if (n < 0 || n >= demoCur.steps.length) return;
            demoStep = n; demoRender();
        }

        async function demoSend(idx, s) {
            try {
                const res = await fetch('/api/demo/send_step', {
                    method: 'POST', headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action_type: s.action_type, action_key: s.action_key})
                });
                const data = await res.json();
                demoSent.add(idx); demoRender(); demoAddLog(s.action_label, data);
            } catch(e) { demoAddLog('Error', {error: e.message}); }
        }

        function demoAddLog(title, content) {
            const sb = document.getElementById('demo-sb');
            const empty = sb.querySelector('.demo-sidebar-empty'); if (empty) empty.remove();
            const el = document.createElement('div'); el.className = 'demo-msg';
            el.innerHTML =
                '<div class="dir">📤 Backend → UE5</div>' +
                '<div class="hdr"><span class="badge">' + title + '</span><span class="time">' + new Date().toLocaleTimeString() + '</span></div>' +
                '<div class="body">' + JSON.stringify(content, null, 2) + '</div>';
            sb.insertBefore(el, sb.firstChild);
        }

        mermaid.initialize({
            startOnLoad: false, theme: 'dark',
            themeVariables: { darkMode: true, background: '#0d1126', primaryColor: '#1a2248',
                primaryTextColor: '#e0e6f0', primaryBorderColor: '#2a3a5a', lineColor: '#00d9ff',
                secondaryColor: '#131836', tertiaryColor: '#0d1126' }
        });
    </script>
'''


def get_demo_html(scenarios_json):
    """Return the demo HTML content with scenarios data injected."""
    return DEMO_HTML_CONTENT.replace('{{DEMO_SCENARIOS}}', scenarios_json)


def handle_demo_action(action_type, action_key, skill_lists, task_graphs, skill_allocations,
                       pending_messages, hitl_messages, message_lock, hitl_lock,
                       create_skill_list_message, create_task_graph_message,
                       create_skill_allocation_message):
    """Handle a demo action and return (status_code, response_dict).

    This function is called by mock_backend.py's handler to process demo actions
    using the backend's own data and message queues directly (no HTTP proxy).
    """
    if action_type == 'skill_list':
        if action_key in skill_lists:
            skill_data = skill_lists[action_key]['data']
            msg = create_skill_list_message(skill_data)
            with message_lock:
                pending_messages.append(msg)
            return 200, {
                "status": "queued", "action_type": action_type, "action_key": action_key,
                "message_id": msg['message_id'],
                "note": f"Demo: Skill list '{action_key}' queued for direct execution."
            }
        return 400, {"error": f"Unknown skill list: {action_key}"}

    elif action_type == 'task_graph':
        if action_key in task_graphs:
            task_data = task_graphs[action_key]['data']
            msg = create_task_graph_message(task_data, category="review")
            with hitl_lock:
                hitl_messages.append(msg)
            return 200, {
                "status": "queued", "action_type": action_type, "action_key": action_key,
                "message_id": msg['message_id'],
                "note": f"Demo: Task graph '{action_key}' queued for HITL review."
            }
        return 400, {"error": f"Unknown task graph: {action_key}"}

    elif action_type == 'skill_allocation':
        if action_key in skill_allocations:
            allocation_data = skill_allocations[action_key]
            msg = create_skill_allocation_message(allocation_data, category="review")
            with hitl_lock:
                hitl_messages.append(msg)
            return 200, {
                "status": "queued", "action_type": action_type, "action_key": action_key,
                "message_id": msg['message_id'],
                "note": f"Demo: Skill allocation '{action_key}' queued for HITL review."
            }
        return 400, {"error": f"Unknown skill allocation: {action_key}"}

    return 400, {"error": f"Unknown action type: {action_type}"}


# ========== Standalone mode (python3 demo_guided.py) ==========

if __name__ == '__main__':
    from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
    from urllib.parse import urlparse
    from urllib.request import urlopen, Request
    from urllib.error import URLError

    # Standalone HTML wraps the content in a full page
    STANDALONE_HTML = '''<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MultiAgent Interactive Demo</title>
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
           background: #0a0e1a; color: #e0e6f0; height: 100vh; overflow: hidden; }
    .sa-header { height: 50px; background: #0d1126; border-bottom: 1px solid #1e2a4a;
                 display: flex; align-items: center; padding: 0 24px; gap: 12px; }
    .sa-title { font-size: 16px; font-weight: 700; color: #ffd700; }
    .sa-status { margin-left: auto; font-size: 12px; color: #667; }
    .sa-dot { width: 8px; height: 8px; border-radius: 50%; background: #f44; display: inline-block; margin-right: 6px; }
    .sa-dot.ok { background: #0f8; }
    .sa-wrap { height: calc(100vh - 50px); overflow: hidden; }
</style></head><body>
<div class="sa-header">
    <span>🎮</span> <span class="sa-title">MultiAgent Interactive Demo (Standalone)</span>
    <div class="sa-status"><span class="sa-dot" id="sa-dot"></span><span id="sa-st">...</span></div>
</div>
<div class="sa-wrap">''' + DEMO_HTML_CONTENT.replace(
        "fetch('/api/demo/send_step'",
        "fetch('/api/proxy'"
    ) + '''</div>
<script>
async function checkBk(){try{const r=await fetch('/api/health_proxy');const d=await r.json();
const ok=d.status==='healthy';document.getElementById('sa-dot').className='sa-dot'+(ok?' ok':'');
document.getElementById('sa-st').textContent=ok?'Backend OK':'Error';}catch(e){
document.getElementById('sa-dot').className='sa-dot';document.getElementById('sa-st').textContent='Offline';}}
checkBk();setInterval(checkBk,3000);
</script></body></html>'''

    backend_url = 'http://localhost:8081'

    def proxy_to_backend(path, method='GET', body=None):
        url = backend_url.rstrip('/') + path
        req = Request(url, method=method)
        if body:
            req.data = body.encode('utf-8')
            req.add_header('Content-Type', 'application/json')
        try:
            with urlopen(req, timeout=5) as resp:
                return resp.status, resp.read().decode('utf-8')
        except URLError as e:
            return 502, json.dumps({"error": f"Backend unreachable: {e}"})
        except Exception as e:
            return 500, json.dumps({"error": str(e)})

    class StandaloneDemoHandler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args): pass
        def send_json(self, code, data):
            self.send_response(code)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))

        def do_GET(self):
            p = urlparse(self.path).path
            if p in ('/', '/index.html'):
                html = STANDALONE_HTML.replace('{{DEMO_SCENARIOS}}', json.dumps(DEMO_SCENARIOS, ensure_ascii=False))
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.end_headers()
                self.wfile.write(html.encode('utf-8'))
            elif p == '/api/health_proxy':
                code, body = proxy_to_backend('/api/health')
                self.send_response(code)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(body.encode('utf-8'))
            else:
                self.send_response(404); self.end_headers()

        def do_POST(self):
            p = urlparse(self.path).path
            length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(length).decode('utf-8') if length > 0 else ''
            if p == '/api/proxy':
                try:
                    data = json.loads(body)
                    at, ak = data.get('action_type',''), data.get('action_key','')
                    api_map = {'skill_list': '/api/send_skill', 'task_graph': '/api/send_task_graph',
                               'skill_allocation': '/api/send_skill_allocation'}
                    key_map = {'skill_list': 'skill_key', 'task_graph': 'task_key',
                               'skill_allocation': 'allocation_key'}
                    if at in api_map:
                        code, resp = proxy_to_backend(api_map[at], 'POST', json.dumps({key_map[at]: ak}))
                        try: resp_data = json.loads(resp)
                        except: resp_data = {"raw": resp}
                        self.send_json(code, resp_data)
                    else:
                        self.send_json(400, {"error": f"Unknown action type: {at}"})
                except json.JSONDecodeError as e:
                    self.send_json(400, {"error": str(e)})
            else:
                self.send_response(404); self.end_headers()

    parser = argparse.ArgumentParser(description='MultiAgent Demo Guide (Standalone)')
    parser.add_argument('--port', type=int, default=8082)
    parser.add_argument('--backend', type=str, default='http://localhost:8081')
    args = parser.parse_args()
    backend_url = args.backend.rstrip('/')

    server = ThreadingHTTPServer(('0.0.0.0', args.port), StandaloneDemoHandler)
    print(f"Demo (standalone): http://localhost:{args.port}  |  Backend: {backend_url}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped"); server.shutdown()
