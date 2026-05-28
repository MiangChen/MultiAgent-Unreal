# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，支持 UAV、UGV、Quadruped、Humanoid 等异构机器人协同仿真，并通过 HTTP 与外部规划端交互。

详细操作手册见：[文档站点](https://miangchen.github.io/MultiAgent-Unreal/)。

## 文档站点（GitHub Pages）

当前仓库已加入 `MkDocs + GitHub Pages` 方案（同仓库维护）：

- 站点配置：`mkdocs.yml`
- 站点内容：`site_docs/`
- 自动发布：`.github/workflows/docs-pages.yml`

本地预览：

```bash
python3 -m pip install --user mkdocs mkdocs-material
~/.local/bin/mkdocs serve
```

发布方式：

1. 推送 `main` 分支中 `mkdocs.yml` / `site_docs/**` 的变更
2. GitHub Actions 自动构建并发布到 Pages
3. 仓库 `Settings -> Pages` 里将 Source 设为 `GitHub Actions`

## 1. 环境配置（基础）

- 操作系统：macOS / Linux
- Unreal Engine：建议 `5.7.x`
- Python：`3.8+`（用于 mock backend 或外部规划端）
- Git LFS：用于拉取 `Content` 资产

### 1.1 获取代码

```bash
git clone https://github.com/WindyLab/MultiAgent-Unreal.git
cd MultiAgent-Unreal
```

### 1.2 拉取 Content 资产

```bash
git lfs install
cd unreal_project
git clone https://huggingface.co/datasets/WindyLab/MultiAgent-Content Content
```

中国大陆可用镜像：

```bash
git clone https://hf-mirror.com/datasets/WindyLab/MultiAgent-Content Content
```

### 1.3 配置 UE 路径（mac）

默认脚本路径在：`mac_compile_and_start.sh`

```bash
UE5_ROOT="/Users/Shared/Epic Games/UE_5.7"
```

如果你的 UE 不是这个路径，请改成你本机实际路径（Epic Launcher 安装常见路径就是上面这个）。

### 1.4 配置后端地址

`config/simulation.json` 里默认：

```json
"server": {
  "planner_url": "http://localhost:8081"
}
```

本地联调用 `scripts/mock_backend.py` 时保持 `8081` 即可。

## 2. 启动仿真（推荐流程）

建议开两个终端。

### 2.1 终端 A：启动 mock backend

```bash
python3 scripts/mock_backend.py --port 8081
```

打开浏览器：`http://localhost:8081`

### 2.2 终端 B：编译并启动 UE

```bash
./mac_compile_and_start.sh
```

常用参数：

```bash
./mac_compile_and_start.sh -c Debug
./mac_compile_and_start.sh -r
./mac_compile_and_start.sh -- -ResX=1920 -ResY=1080
```

## 3. 启动后如何使用（键盘/鼠标）

以下按键基于当前源码（`MAInputActions.cpp` / `MAPlayerController.cpp`）。

### 3.1 鼠标

- `左键`：选择 Agent（拖拽可框选）
- `Ctrl + 左键`：增减选择
- `中键`：让当前选中 Agent 导航到鼠标点击位置
- `右键按住拖动`：旋转视角

### 3.2 常用模式与界面

- `M`：切换 Edit 模式
- `,`：切换 Modify 模式
- `Z`：打开/关闭 Task Graph 查看
- `N`：打开/关闭 Skill Allocation 查看
- `9`：打开/关闭 Decision 查看（仅有决策通知时有效）
- `6`：系统日志面板显隐
- `7`：预览面板显隐
- `8`：指令面板显隐
- `[`：Agent 高亮圈显隐

### 3.3 视角与控制

- `Tab` / `C`：切到下一个 Agent 视角
- `0` / `V`：返回 Spectator
- Agent 视角下：`W/A/S/D` 移动、`E/Q` 升降、`Space` 跳跃、鼠标移动/方向键看向

### 3.4 编组与执行

- `1..5`：选中编组
- `Ctrl + 1..5`：保存当前选择到编组
- `Q`：创建 Squad
- `Shift + Q`：解散 Squad
- `P`：暂停/恢复技能执行（Pause/Resume）

### 3.5 调试与辅助

- `Y`：打印 Agent 信息
- `U`：销毁最后一个 Agent
- `L`：当前相机拍照
- `R`：Pickup（原 `P` 已改为 Pause）
- `O`：Drop

## 4. UI 界面使用方法

### 4.1 Web 控制台（`http://localhost:8081`）

- 左侧按钮可发送三类数据：
  - `Task Graph`（HITL，UE 侧会弹确认流程）
  - `Skill Allocation`（HITL，UE 侧会弹确认流程）
  - `Skill List`（直接执行）
- 顶部状态显示：
  - 未连 UE 时：等待连接
  - 连上 UE 后：`UE5 链接成功`

### 4.2 UE 内交互建议流程

1. 在 Web 控制台发送一个 `Task Graph`。
2. 回到 UE 按 `Z` 打开任务图模态框查看。
3. 确认后继续执行，或拒绝。
4. 发送 `Skill Allocation` 后按 `N` 查看。
5. 如收到决策类通知，按 `9` 进入 Decision 模态。

### 4.3 Edit / Modify

- `Edit (M)`：用于临时场景编辑（POI、Goal/Zone 等）
- `Modify (,)`：用于场景对象标签修改
- 两种模式互斥，切换一种会退出另一种

## 5. AI 辅助配置（推荐）

可以，建议分两层用 AI：

### 5.1 层 1：AI 生成任务草案（自然语言 -> 结构化）

你可以让 LLM 先生成 `task_graph` 或 `skill_list` 草案，再人工检查后投喂给后端。

建议提示词（可直接复制）：

```text
请根据以下目标生成 MultiAgent 的 task_graph JSON。
要求：
1) 输出仅包含 JSON
2) 包含 meta 和 task_graph
3) task_graph 里包含 nodes 与 edges
4) 每个 node 包含 task_id/description/location/required_skills/produces
5) required_skills 里给出 skill_name、assigned_robot_type、assigned_robot_count
任务目标：{在这里写任务目标}
机器人资源：{在这里写可用机器人}
```

### 5.2 层 2：接入真实 AI Planner

如果你有自己的 AI 后端，把 `config/simulation.json` 的 `planner_url` 改为你的服务地址即可（协议参考 [API 参考](https://miangchen.github.io/MultiAgent-Unreal/api-reference/)）。

## 6. 常见问题

### 6.1 Web 页一直显示“等待 UE5 仿真端连接”

- 确认 UE 已运行且项目加载完成
- 确认 `config/simulation.json` 中 `planner_url` 与 mock backend 端口一致（默认 `8081`）
- 确认没有防火墙拦截本地端口

### 6.2 mac 启动后遇到 Substrate/RayTracing 相关报错

可先在 `unreal_project/Config/DefaultEngine.ini` 中关闭：

```ini
r.Substrate=False
```

### 6.3 编译脚本报 UE 路径错误

检查 `mac_compile_and_start.sh` 中 `UE5_ROOT` 是否与你本机一致。

## 7. 参考文档

- [文档站点首页](https://miangchen.github.io/MultiAgent-Unreal/)：功能介绍与接入概览
- [启动与 Example](https://miangchen.github.io/MultiAgent-Unreal/startup-and-examples/)：联调与运行流程
- [按键说明](https://miangchen.github.io/MultiAgent-Unreal/keybindings/)：键盘与鼠标交互
- [架构](https://miangchen.github.io/MultiAgent-Unreal/architecture/)：当前控制论架构基线
- [API 参考](https://miangchen.github.io/MultiAgent-Unreal/api-reference/)：后端接口与消息格式
- [config/README.md](config/README.md)：配置项详解

## License

MIT License
