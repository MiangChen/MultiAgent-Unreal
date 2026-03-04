# 启动与 Example

本页给出最短联调路径：启动后端、启动 UE、发送示例任务并在 UE 内查看执行。

## 1. 前置准备

- 已拉取仓库和 Content 资源
- 本机安装 UE5（mac 常见路径：`/Users/Shared/Epic Games/UE_5.7`）
- `config/simulation.json` 中 `server.planner_url` 指向后端地址（默认 `http://localhost:8081`）

## 2. 启动步骤（推荐两终端）

## 2.1 终端 A：启动后端

```bash
cd PathTo/MultiAgent-Unreal
python3 scripts/mock_backend.py --port 8081
```

打开浏览器：`http://localhost:8081`

## 2.2 终端 B：编译并启动 UE（按系统）

| 系统 | 编译与运行指令 |
|---|---|
| Linux | `cd PathTo/MultiAgent-Unreal`<br>`cp example_compile.sh compile.sh && cp example_start_linux.sh start.sh && chmod +x compile.sh start.sh`<br>`./compile.sh`<br>`./start.sh` |
| mac | `cd PathTo/MultiAgent-Unreal`<br>`./mac_compile_and_start.sh`<br>`./mac_compile_and_start.sh -c Debug`<br>`./mac_compile_and_start.sh -r`<br>`./mac_compile_and_start.sh -- -ResX=1920 -ResY=1080` |
| Windows | `cd PathTo/MultiAgent-Unreal`<br>`\"PathToUE/Engine/Build/BatchFiles/Build.bat\" MultiAgentEditor Win64 Development -Project=\"%cd%/unreal_project/MultiAgent.uproject\" -WaitMutex`<br>`\"PathToUE/Engine/Binaries/Win64/UnrealEditor.exe\" \"%cd%/unreal_project/MultiAgent.uproject\"` |

## 3. 连接成功判定

满足任一即可进一步联调：

- Web 状态文本显示 `UE5 连接成功`
- 后端控制台出现 UE 轮询日志（`/api/sim/poll` 或 `/api/hitl/poll`）

## 4. 运行 Example（Web 按钮流程）

## 4.1 Task Graph 示例

1. 在 Web 左侧点击一个 `Task Graph` 按钮
2. 切回 UE，按 `Z` 打开 Task Graph 模态
3. 审核后确认/拒绝

## 4.2 Skill Allocation 示例

1. 在 Web 左侧点击一个 `Skill Allocation` 按钮
2. 切回 UE，按 `N` 打开技能分配模态
3. 审核后确认/拒绝

## 4.3 Skill List 示例（直接执行）

1. 在 Web 左侧点击一个 `Skill List` 按钮
2. 消息直接进入执行链路（无需模态确认）

## 5. 运行中的常用键

- `Z`：Task Graph 模态
- `N`：Skill Allocation 模态
- `9`：Decision 模态（有决策通知时）
- `M`：Edit 模式
- `,`：Modify 模式
- `6 / 7 / 8`：右侧三个面板显隐
- `Tab` / `C`：切换 Agent 视角
- `0` / `V`：返回 Spectator

## 6. API 调用入口（最小说明）

当前 mock backend 提供以下发送接口：

- `POST /api/send_task_graph`
- `POST /api/send_skill_allocation`
- `POST /api/send_skill`

示例（占位 key）：

```bash
curl -X POST http://localhost:8081/api/send_task_graph \
  -H "Content-Type: application/json" \
  -d '{"task_key":"<task_key>"}'
```

```bash
curl -X POST http://localhost:8081/api/send_skill_allocation \
  -H "Content-Type: application/json" \
  -d '{"allocation_key":"<allocation_key>"}'
```

```bash
curl -X POST http://localhost:8081/api/send_skill \
  -H "Content-Type: application/json" \
  -d '{"skill_key":"<skill_key>"}'
```

完整字段和状态码说明见左侧 `API 参考` 页面。

## 7. 常见启动问题

## 7.1 Web 一直显示“等待 UE5 连接”

- 检查 `planner_url` 与后端端口是否一致
- 检查 UE 是否已完全进入场景
- 检查本机防火墙是否拦截本地端口

## 7.2 mac 渲染相关报错（Substrate）

可先在 `unreal_project/Config/DefaultEngine.ini` 临时关闭：

```ini
r.Substrate=False
```
