# 任务描述：开发 Edit Mode (编辑模式)

我需要你为当前的虚幻仿真平台开发一个新的 **Edit Mode**。

## 1. 背景与核心概念
我们现有的系统已经包含一个 **Modify Mode**，用于对场景图（Scene Graph）进行持久化的语义标注（如给 Actor 打标签）。现在需要新增 **Edit Mode**，用于模拟任务执行过程中发生的“新情况”（动态变化）。

### 场景图文件处理机制
*   **源文件**：`unreal_project/datasets/scene_graph_cyberworld.json`。
*   **Modify Mode**：直接修改上述源文件（持久化修改）。
*   **Edit Mode**：
    1.  每次启动游戏时，必须基于源文件创建一个**临时场景图文件**。
    2.  Edit Mode 的所有增删改操作仅针对这个**临时文件**进行，不影响源文件。
    3.  用户需要能浏览并修改这个临时文件的内容。

### 平台目标
该平台用于验证任务规划算法。当场景（世界）中发生新情况（由 Edit Mode 触发）并影响任务规划或执行时，系统需触发重规划。

---

## 2. Edit Mode 功能定义：新情况与操作

所有的“新情况”都对应场景图的修改，主要分为以下几类：

### A. 基础 Actor 操作 (增/删/改)
*   **增**：在虚幻场景内新增一个 Actor -> 场景图中新增对应的 Node 和 Edge。
*   **删**：在虚幻场景内删除一个 Actor -> 场景图中删除对应的 Node 和 Edge。
*   **改坐标**：在虚幻场景内平移 Actor -> 修改场景图中 `shape` 里的 `center` 字段。
    *   *约束*：上述三种操作（增、删、改坐标）仅允许针对 `shape` 类型为 `point` 的节点进行。如果节点是 `polygon` 类型，则禁止这些操作。
*   **改属性**：修改场景图中 Node 的 `property` 字段。

### B. 特殊节点操作 (Goal / Zone)
这两种操作涉及 POI (Point of Interest) 的交互（详见下文交互部分）：

1.  **新增 Goal (目标点)**
    *   在虚幻场景中选择一个 POI 点，将其设为 Goal。
    *   对应 JSON 结构示例：
        ```json
        {
            "id": "27",
            "properties": {
                "type": "goal",
                "label": "goal-5",
                "category": "goal",
                "status": "uncompleted",
                "description": "none"
            },
            "shape": {
                "type": "point",
                "center": [-6400, 4400.000156, 0]
            }
        }
        ```

2.  **新增 Zone (区域)**
    *   由多个 POI 点围成的多边形区域。
    *   对应 JSON 结构示例：
        ```json
        {
            "id": "14",
            "properties": {
                "type": "zone",
                "label": "Zone-14",
                "category": "zone",
                "status": "undiscovered",
                "description": "none"
            },
            "shape": {
                "type": "polygon",
                "vertices": [
                    [-5174.82, -10332.27, 0],
                    [-5546.33, -8692.97, 0],
                    [-6505.74, -7235.16, 0]
                ]
            }
        }
        ```

---

## 3. 后端通信接口

任何修改发生后，都必须通过通信模块向后端规划器报告。请使用以下函数式声明规则设计报告逻辑（`params` 均为对应的 JSON 片段）：

*   `add_node(params)`
*   `add_edge(params)`
*   `delete_node(params)`：后端需自动删除与该点相连的所有边。
*   `edit_node(params)`：用于属性修改等。
*   `edit_edge(params)`
*   `add_goal(params)`
*   `delete_goal(params)`
*   `add_zone(params)`
*   `delete_zone(params)`

---

## 4. 交互与 UI 设计细节

### 模式切换
*   按 `M` 键在不同模式间切换（含 Edit Mode 和 Modify Mode）。

### 鼠标交互与 POI 创建
1.  **点击生成 POI**：
    *   单独鼠标左键点击场景 -> 进行射线检测。
    *   **视觉反馈**：
        *   在点击位置生成一个常驻的 **Niagara 组件**（标记点击点）。
        *   屏幕下方用绿色小字显示点击点坐标。
    *   **逻辑定义**：此操作创建了一个 **POI**（Point of Interest）。POI 仅作为中间临时对象存在，**此时不写入 JSON**。

2.  **对象选择 (Shift + 左键)**：
    *   支持选中 Actor 或 POI，选中后使用高亮材质显示（逻辑同 Modify Mode）。
    *   **选择规则**：
        *   普通 Actor（如房屋、花坛）：仅支持**单选**。
        *   POI：支持**多选**。
        *   **互斥**：不能同时选中 Actor 和 POI。

### UI 组件与操作逻辑

#### 情况一：选中 Actor 时
右侧显示 UI 面板，包含：
1.  **JSON 编辑区**：
    *   显示当前 Actor 对应的 JSON 片段。
    *   **多 Node 切换**：如果该 Actor 属于多个 Node，需提供按钮进行切换。
    *   **编辑限制**：根据 Node 类型（point/polygon/linestring）筛选可编辑范围，防止用户修改非法字段（越界保护）。
2.  **操作按钮**：
    *   **[确认]**：提交修改。程序需检查 JSON 格式合法性。若修改了坐标，**暂时忽略**场景图中边的断裂与重连问题。
    *   **[删除]**：删除该 Actor，并级联删除其涉及的所有点和边。
3.  **通信**：操作完成后，自动组合上述函数式声明报告给后端。

#### 情况二：选中 POI 时
右侧显示 UI 面板，包含：
1.  **描述输入框**：
    *   多行可编辑文本框。
    *   输入格式示例：`des:xxxxxx`。
2.  **功能按钮**：
    *   **[创建 Goal]**（单选 POI 时可用）：创建 Goal Node 写入临时场景图，文本框内容自动填入 `description` 字段。
    *   **[创建区域]**（多选 POI 时可用）：创建 Zone Node 写入临时场景图，文本框内容自动填入 `description` 字段。
3.  **预设 Actor 添加（UI 预留）**：
    *   提供一个选择框，列出预备好的 Actor。
    *   功能：在 POI 位置添加选定的 Actor，并自动在场景图中创建对应的预设 Node。
    *   *注：具体 Actor 资源暂未准备好，但需预先实现此 UI 逻辑及对应的通信接口。*
4.  **通信**：操作完成后，自动组合上述函数式声明报告给后端。