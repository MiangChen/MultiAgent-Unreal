# 仿真-后端通信协议文档

本文档描述 MultiAgent-Unreal 仿真平台与规划器后端之间的数据流和通信协议。

## 1. 系统架构概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           通信架构                                           │
│                                                                             │
│  ┌─────────────────────┐                    ┌─────────────────────┐        │
│  │   仿真端 (UE5)       │                    │   规划器后端         │        │
│  │                     │                    │                     │        │
│  │  ┌───────────────┐  │    HTTP POST       │  ┌───────────────┐  │        │
│  │  │ MACommSubsystem│──┼──────────────────▶│  │ /api/sim/     │  │        │
│  │  │               │  │                    │  │   message     │  │        │
│  │  │               │  │    HTTP POST       │  │   scene_change│  │        │
│  │  │               │──┼──────────────────▶│  │               │  │        │
│  │  │               │  │                    │  │               │  │        │
│  │  │               │  │    HTTP GET        │  │               │  │        │
│  │  │               │◀─┼──────────────────▶│  │   poll        │  │        │
│  │  └───────────────┘  │    (轮询)          │  └───────────────┘  │        │
│  │                     │                    │                     │        │
│  └─────────────────────┘                    └─────────────────────┘        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2. API 端点

| 端点 | 方法 | 方向 | 描述 |
|------|------|------|------|
| `/api/sim/message` | POST | 仿真 → 后端 | 发送 UI 输入、按钮事件、任务反馈 |
| `/api/sim/scene_change` | POST | 仿真 → 后端 | 发送 Edit Mode 场景变化 |
| `/api/sim/poll` | GET | 仿真 → 后端 | 轮询获取任务规划 DAG |

## 3. 出站消息 (仿真端 → 后端)

### 3.1 消息信封格式

所有出站消息使用统一的信封格式包装：

```json
{
    "message_type": "ui_input | button_event | task_feedback | task_graph_submit",
    "timestamp": 1735600000000,
    "message_id": "550e8400-e29b-41d4-a716-446655440000",
    "payload": { ... }
}
```

| 字段 | 类型 | 描述 |
|------|------|------|
| `message_type` | string | 消息类型标识 |
| `timestamp` | int64 | Unix 时间戳 (毫秒) |
| `message_id` | string | UUID 格式的消息唯一标识 |
| `payload` | object | 消息负载数据 |

### 3.2 UI 输入消息 (ui_input)

用户在输入框提交文本时发送。

**Payload 格式：**
```json
{
    "instruction_id": "task_planner_input",
    "instruction_text": "让机器狗去巡逻"
}
```

| 字段 | 类型 | 描述 |
|------|------|------|
| `instruction_id` | string | 输入框 Widget 标识 |
| `instruction_text` | string | 用户输入的文本内容 |

**完整示例：**
```json
{
    "message_type": "ui_input",
    "timestamp": 1735600000000,
    "message_id": "550e8400-e29b-41d4-a716-446655440000",
    "payload": {
        "instruction_id": "task_planner_input",
        "instruction_text": "让机器狗去巡逻"
    }
}
```

### 3.3 按钮事件消息 (button_event)

用户点击按钮时发送。

**Payload 格式：**
```json
{
    "widget_name": "TaskPlannerWidget",
    "button_id": "btn_execute",
    "button_text": "执行"
}
```

| 字段 | 类型 | 描述 |
|------|------|------|
| `widget_name` | string | 所属 Widget 名称 |
| `button_id` | string | 按钮标识 |
| `button_text` | string | 按钮显示文字 |

### 3.4 任务反馈消息 (task_feedback)

任务执行完成后发送结果反馈。

**Payload 格式：**
```json
{
    "task_id": "task_001",
    "status": "success",
    "duration_seconds": 12.5,
    "energy_consumed": 8.3,
    "error_message": ""
}
```

| 字段 | 类型 | 描述 |
|------|------|------|
| `task_id` | string | 任务 ID |
| `status` | string | 状态: `success`, `failed`, `timeout`, `in_progress` |
| `duration_seconds` | float | 任务耗时 (秒) |
| `energy_consumed` | float | 能量消耗 |
| `error_message` | string | 错误信息 (失败时填写) |

### 3.5 任务图提交消息 (task_graph_submit)

用户在任务规划工作台编辑任务图后，点击"提交任务图"按钮时发送。

**Payload 格式：**

Payload 包含元数据和任务图数据：

```json
{
    "meta": {
        "source": "TaskPlannerWidget",
        "version": "1.0"
    },
    "task_graph": {
        "description": "巡逻任务",
        "nodes": [
            {
                "task_id": "node_001",
                "description": "导航到起点",
                "location": "PatrolStart"
            },
            {
                "task_id": "node_002",
                "description": "开始巡逻",
                "location": "PatrolPath_1"
            }
        ],
        "edges": [
            {
                "from": "node_001",
                "to": "node_002",
                "type": "sequential"
            }
        ]
    }
}
```

| 字段 | 类型 | 描述 |
|------|------|------|
| `meta` | object | 元数据信息 |
| `meta.source` | string | 来源 Widget 标识 |
| `meta.version` | string | 协议版本 |
| `task_graph` | object | 任务图数据 |
| `task_graph.description` | string | 任务图描述 |
| `task_graph.nodes` | array | 节点列表 |
| `task_graph.nodes[].task_id` | string | 节点 ID (兼容 `id` 字段) |
| `task_graph.nodes[].description` | string | 节点描述 |
| `task_graph.nodes[].location` | string | 位置/目标 |
| `task_graph.edges` | array | 边列表 |
| `task_graph.edges[].from` | string | 起始节点 ID |
| `task_graph.edges[].to` | string | 目标节点 ID |
| `task_graph.edges[].type` | string | 边类型 (默认: `sequential`) |

**完整示例：**
```json
{
    "message_type": "task_graph_submit",
    "timestamp": 1735600000000,
    "message_id": "550e8400-e29b-41d4-a716-446655440000",
    "payload": {
        "meta": {
            "source": "TaskPlannerWidget",
            "version": "1.0"
        },
        "task_graph": {
            "description": "巡逻任务",
            "nodes": [
                {"task_id": "node_001", "description": "导航到起点", "location": "PatrolStart"},
                {"task_id": "node_002", "description": "开始巡逻", "location": "PatrolPath_1"}
            ],
            "edges": [
                {"from": "node_001", "to": "node_002", "type": "sequential"}
            ]
        }
    }
}
```

### 3.6 场景变化消息 (scene_change)

Edit Mode 中场景发生变化时发送，使用独立端点 `/api/sim/scene_change`。

**消息格式：**
```json
{
    "change_type": "add_node | delete_node | edit_node | add_goal | delete_goal | add_zone | delete_zone | add_edge | edit_edge",
    "timestamp": 1735600000000,
    "message_id": "550e8400-e29b-41d4-a716-446655440000",
    "payload": "{ ... }"
}
```

| 变化类型 | 描述 | Payload 内容 |
|----------|------|--------------|
| `add_node` | 添加节点 | 完整的 Node JSON |
| `delete_node` | 删除节点 | Node ID |
| `edit_node` | 修改节点 | 修改后的 Node JSON |
| `add_goal` | 添加目标点 | Goal Node JSON |
| `delete_goal` | 删除目标点 | Goal ID |
| `add_zone` | 添加区域 | Zone Node JSON |
| `delete_zone` | 删除区域 | Zone ID |
| `add_edge` | 添加边 | Edge JSON |
| `edit_edge` | 修改边 | 修改后的 Edge JSON |

#### 3.6.1 场景变化消息示例

**添加节点 (add_node):**
```json
{
    "change_type": "add_node",
    "timestamp": 1735600000000,
    "message_id": "...",
    "payload": "{\"id\":\"building_01\",\"properties\":{\"type\":\"building\",\"label\":\"Building-1\"},\"shape\":{\"type\":\"point\",\"center\":[100,200,0]}}"
}
```

**添加 Goal (add_goal):**
```json
{
    "change_type": "add_goal",
    "timestamp": 1735600000000,
    "message_id": "...",
    "payload": "{\"id\":\"goal_01\",\"properties\":{\"type\":\"goal\",\"label\":\"Goal-1\",\"description\":\"搜索目标\"},\"shape\":{\"type\":\"point\",\"center\":[500,300,0]}}"
}
```

**添加 Zone (add_zone):**
```json
{
    "change_type": "add_zone",
    "timestamp": 1735600000000,
    "message_id": "...",
    "payload": "{\"id\":\"zone_01\",\"properties\":{\"type\":\"zone\",\"label\":\"Zone-1\",\"description\":\"搜索区域\"},\"shape\":{\"type\":\"polygon\",\"vertices\":[[0,0,0],[100,0,0],[100,100,0],[0,100,0]]}}"
}
```

**删除节点 (delete_node):**
```json
{
    "change_type": "delete_node",
    "timestamp": 1735600000000,
    "message_id": "...",
    "payload": "building_01"
}
```

## 4. 入站消息 (后端 → 仿真端)

通过轮询 `/api/sim/poll` 端点获取。

### 4.1 轮询响应格式

响应为消息数组或单个消息对象：

```json
[
    {
        "message_type": "task_plan_dag",
        "timestamp": 1735600000000,
        "message_id": "...",
        "payload": { ... }
    }
]
```

空响应（无新消息）：
```json
[]
```

### 4.2 任务规划 DAG (task_plan_dag)

规划器返回的任务执行计划，以有向无环图 (DAG) 形式表示。

**Payload 格式：**
```json
{
    "nodes": [
        {
            "node_id": "node_001",
            "task_type": "navigate",
            "parameters": {
                "target_x": "100.0",
                "target_y": "200.0",
                "agent_id": "dog_01"
            },
            "dependencies": []
        },
        {
            "node_id": "node_002",
            "task_type": "patrol",
            "parameters": {
                "patrol_path": "PatrolPath_1",
                "agent_id": "dog_01"
            },
            "dependencies": ["node_001"]
        }
    ],
    "edges": [
        {
            "from_node_id": "node_001",
            "to_node_id": "node_002"
        }
    ]
}
```

**节点 (Node) 字段：**

| 字段 | 类型 | 描述 |
|------|------|------|
| `node_id` | string | 节点唯一标识 |
| `task_type` | string | 任务类型 (navigate, patrol, follow, charge, coverage 等) |
| `parameters` | map<string, string> | 任务参数键值对 |
| `dependencies` | array<string> | 依赖的节点 ID 列表 |

**边 (Edge) 字段：**

| 字段 | 类型 | 描述 |
|------|------|------|
| `from_node_id` | string | 起始节点 ID |
| `to_node_id` | string | 目标节点 ID |

> **注意：** 世界模型（场景图）现在存储在仿真端本地，由 `MASceneGraphManager` 管理，不再通过轮询从后端获取。仿真端通过 `/api/sim/scene_change` 端点向后端同步场景变化。

## 5. 配置说明

配置文件路径：`config/SimConfig.json`

```json
{
    "PlannerServerURL": "http://localhost:8080",
    "bUseMockData": false,
    "bEnablePolling": true,
    "PollIntervalSeconds": 2.0
}
```

| 配置项 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `PlannerServerURL` | string | `http://localhost:8080` | 后端服务器地址 |
| `bUseMockData` | bool | `false` | 是否使用模拟数据 (开发测试用) |
| `bEnablePolling` | bool | `true` | 是否启用轮询 |
| `PollIntervalSeconds` | float | `2.0` | 轮询间隔 (秒) |

## 6. 错误处理与重试

### 6.1 HTTP 状态码处理

| 状态码 | 处理方式 |
|--------|----------|
| 2xx | 成功，重置重试计数 |
| 4xx | 客户端错误，不重试，广播失败响应 |
| 5xx | 服务器错误，触发重试 |

### 6.2 重试策略

- 最大重试次数：3 次
- 退避策略：指数退避 (1s → 2s → 4s)
- 超过最大重试次数后广播失败响应

## 7. 数据流图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              数据流                                          │
│                                                                             │
│  用户操作                                                                    │
│     │                                                                       │
│     ▼                                                                       │
│  ┌─────────────────┐                                                        │
│  │ UI Widget       │                                                        │
│  │ (输入框/按钮)    │                                                        │
│  └────────┬────────┘                                                        │
│           │                                                                 │
│           │ 调用                                                            │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      MACommSubsystem                                 │   │
│  │                                                                      │   │
│  │  SendUIInputMessage()      ──┐                                       │   │
│  │  SendButtonEventMessage()  ──┼──▶ SendMessageEnvelope()             │   │
│  │  SendTaskFeedbackMessage() ──┘           │                          │   │
│  │                                          │                          │   │
│  │  SendSceneChangeMessage() ───────────────┼──▶ HTTP POST             │   │
│  │                                          │    /api/sim/scene_change │   │
│  │                                          │                          │   │
│  │                                          ▼                          │   │
│  │                              ExecuteHttpPost()                       │   │
│  │                                          │                          │   │
│  │                                          ▼                          │   │
│  │                              HTTP POST /api/sim/message              │   │
│  │                                                                      │   │
│  │  ┌─────────────────────────────────────────────────────────────┐    │   │
│  │  │                    轮询机制                                   │    │   │
│  │  │                                                              │    │   │
│  │  │  StartPolling() ──▶ Timer ──▶ PollForMessages()             │    │   │
│  │  │                                      │                       │    │   │
│  │  │                                      ▼                       │    │   │
│  │  │                          HTTP GET /api/sim/poll              │    │   │
│  │  │                                      │                       │    │   │
│  │  │                                      ▼                       │    │   │
│  │  │                          HandlePollResponse()                │    │   │
│  │  │                                      │                       │    │   │
│  │  │                                      ▼                       │    │   │
│  │  │                          OnTaskPlanReceived (委托)            │    │   │
│  │  └─────────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 8. 委托 (Delegate) 接口

仿真端通过委托广播接收到的消息：

| 委托 | 触发时机 | 参数 |
|------|----------|------|
| `OnPlannerResponse` | 收到规划器响应 | `FMAPlannerResponse` |
| `OnTaskPlanReceived` | 收到任务规划 DAG | `FMATaskPlan` |

**订阅示例 (C++)：**
```cpp
// 获取通信子系统
UMACommSubsystem* CommSubsystem = GetGameInstance()->GetSubsystem<UMACommSubsystem>();

// 订阅任务规划
CommSubsystem->OnTaskPlanReceived.AddDynamic(this, &UMyClass::HandleTaskPlan);
```

## 9. 测试工具

### 9.1 模拟后端

使用 `scripts/mock_backend.py` 启动模拟后端进行通信测试：

```bash
# 默认端口 8080
python3 scripts/mock_backend.py

# 指定端口
python3 scripts/mock_backend.py --port 9000
```

### 9.2 快速启动脚本

使用 `bstart.sh` 同时启动模拟后端和 Unreal Editor：

```bash
./bstart.sh
```

## 10. 相关文件

| 文件 | 描述 |
|------|------|
| `unreal_project/Source/MultiAgent/Core/MACommTypes.h` | 消息类型定义 |
| `unreal_project/Source/MultiAgent/Core/MACommSubsystem.h/cpp` | 通信子系统实现 |
| `config/SimConfig.json` | 通信配置 |
| `scripts/mock_backend.py` | 模拟后端服务器 |
| `bstart.sh` | 快速启动脚本 |
