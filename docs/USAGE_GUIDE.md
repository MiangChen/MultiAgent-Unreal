# MultiAgent-Unreal 使用手册

本手册聚焦三件事：
- 如何启动并联调 UE + backend
- 仿真运行后的键鼠操作
- 任务图 / 技能分配 / 决策类 UI 的使用流程

## 1. 联调启动流程

### 1.1 启动 backend（终端 A）

```bash
cd PathTo/MultiAgent-Unreal
python3 scripts/mock_backend.py --port 8081
```

浏览器访问：`http://localhost:8081`

### 1.2 编译并启动 UE（终端 B）

```bash
cd PathTo/MultiAgent-Unreal
./mac_compile_and_start.sh
```

`mac_compile_and_start.sh` 默认使用：

- `UE5_ROOT="/Users/Shared/Epic Games/UE_5.7"`
- 项目：`unreal_project/MultiAgent.uproject`

如果你 UE 安装路径不同，先改脚本中的 `UE5_ROOT`。

### 1.3 联调成功判断

- Web 页状态从“等待 UE5 连接”切到 `UE5 链接成功`
- 控制台有 `UE5 polling` 日志
- 在 Web 发送消息后，右侧日志能看到 `Backend -> UE5`

## 2. 键鼠速查（当前代码）

### 2.1 鼠标

| 输入 | 功能 |
|---|---|
| 左键 | 选择 / 框选 Agent |
| Ctrl + 左键 | 增减当前选择 |
| 中键 | 命令选中 Agent 导航到点击点 |
| 右键拖动 | 旋转视角 |

### 2.2 模式与面板

| 按键 | 功能 |
|---|---|
| `M` | Edit 模式开关 |
| `,` | Modify 模式开关 |
| `Z` | Task Graph 模态开关 |
| `N` | Skill Allocation 模态开关 |
| `9` | Decision 模态开关（有决策通知时） |
| `6` | System Log 面板 |
| `7` | Preview 面板 |
| `8` | Instruction 面板 |
| `[` | Agent 高亮圈开关 |

### 2.3 视角与直接控制

| 按键 | 功能 |
|---|---|
| `Tab` / `C` | 切到下一个 Agent 视角 |
| `0` / `V` | 返回 Spectator |
| `W/A/S/D` | Agent 视角移动 |
| `E/Q` | 升/降 |
| `Space` | 跳跃 |
| 鼠标移动 / 方向键 | 调整视角 |

### 2.4 编组与执行

| 按键 | 功能 |
|---|---|
| `1..5` | 选择编组 |
| `Ctrl + 1..5` | 保存编组 |
| `Q` | 创建 Squad |
| `Shift + Q` | 解散 Squad |
| `P` | 暂停/恢复执行 |

### 2.5 调试

| 按键 | 功能 |
|---|---|
| `Y` | 打印 Agent 信息 |
| `U` | 销毁最后一个 Agent |
| `L` | 当前相机拍照 |
| `R` | Pickup |
| `O` | Drop |

## 3. 任务 UI 使用流程

### 3.1 Task Graph（推荐冒烟）

1. 在 Web UI 左侧点击一个 Task Graph 按钮。
2. 回到 UE 按 `Z` 打开任务图模态。
3. 查看后确认或拒绝。

### 3.2 Skill Allocation

1. 在 Web UI 点击一个 Skill Allocation 按钮。
2. 回到 UE 按 `N` 打开技能分配模态。
3. 确认后执行（或拒绝）。

### 3.3 Decision

1. 当系统产生决策类通知后，按 `9`。
2. 在 Decision 模态中进行确认/拒绝。

## 4. AI 辅助配置建议

### 4.1 用 AI 生成任务图草案

给 AI 下面模板，让它先产出结构化 JSON（你再审核）：

```text
请输出 MultiAgent 的 task_graph JSON（仅 JSON，不要解释）。
字段要求：meta, task_graph.nodes, task_graph.edges。
node 必须包含：task_id, description, location, required_skills, produces。
required_skills 必须包含：skill_name, assigned_robot_type, assigned_robot_count。
任务目标：{这里填写}
可用机器人：{这里填写}
```

### 4.2 接真实 AI 后端

把 `config/simulation.json` 的 `server.planner_url` 改成你的 AI Planner 服务地址，保持协议字段与 `doc/COMMUNICATION_PROTOCOL.md` 对齐。

### 4.3 Mock Backend 现状

当前 `scripts/mock_backend.py` 主要通过预设 `task_key` / `allocation_key` / `skill_key` 发送内容。若你要快速验证新方案，可直接在脚本内新增预设条目。

## 5. 故障排查

### 5.1 UE 未连接

- 确认 backend 端口与 `planner_url` 一致（默认 `8081`）
- 确认 UE 已完成加载且未卡在报错弹窗

### 5.2 Substrate 报错（mac）

可先临时关闭：

```ini
r.Substrate=False
```

位置：`unreal_project/Config/DefaultEngine.ini`
