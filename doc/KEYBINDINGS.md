# MultiAgent-Unreal 按键说明

## 鼠标操作

| 按键 | Select 模式 | Navigate 模式 |
|------|-------------|---------------|
| **左键** | 点击选择 Agent / 框选多个 Agent | Human 导航到点击位置 |
| **左键 + Ctrl** | 添加/移除选择 | - |
| **右键** | 选中的 RobotDog/Drone 导航到点击位置 | 选中的 RobotDog/Drone 导航到点击位置 |

## 模式切换

| 按键 | 功能 |
|------|------|
| **M** | 切换鼠标模式 (Select ↔ Navigate) |

## 视角控制

| 按键 | 功能 |
|------|------|
| **Tab** | 切换到下一个 Agent 的 Camera 视角 |
| **0** | 返回上帝视角 (Spectator) |

## 选择与编组 (星际争霸风格)

| 按键 | 功能 |
|------|------|
| **1-5** | 选中编组 1-5 |
| **Ctrl + 1-5** | 将当前选择保存到编组 1-5 |

## Squad 编队

| 按键 | 功能 |
|------|------|
| **Q** | 创建 Squad (需选中至少 2 个 Agent) |
| **Shift + Q** | 解散选中 Agent 所属的 Squad |
| **B** | 循环切换编队类型 (None → Line → Column → Wedge → Diamond → Circle → None) |

## Agent 命令

| 按键 | 功能 |
|------|------|
| **F** | 选中的 RobotDog/Drone 跟随 Human |
| **G** | 选中的 RobotDog/Drone 开始巡逻 (需场景中有 PatrolPath) |
| **H** | 选中的 RobotDog/Drone 去充电 |
| **J** | 选中的 RobotDog/Drone 停止并进入 Idle |
| **K** | 选中的 RobotDog/Drone 开始覆盖扫描 (需场景中有 CoverageArea) |

## 物品交互

| 按键 | 功能 |
|------|------|
| **P** | Human 拾取附近物品 |
| **O** | Human 放下持有物品 |
| **I** | 在鼠标位置生成可拾取物品 |

## 生成与调试

| 按键 | 功能 |
|------|------|
| **T** | 在 Human 前方生成 RobotDog |
| **Y** | 打印所有 Agent 信息 |
| **U** | 销毁最后一个 Agent |

## 相机传感器

| 按键 | 功能 |
|------|------|
| **L** | 拍照 (保存到 Saved/Screenshots/) |
| **R** | 开始/停止录像 |
| **V** | 开始/停止 TCP 视频流 (端口 9000) |

## 编队类型说明

| 类型 | 说明 | 示意 |
|------|------|------|
| None | 无编队，自由移动 | - |
| Line | 横向一字排开 | `○ ○ L ○ ○` |
| Column | 纵队，跟随 Leader | `L → ○ → ○ → ○` |
| Wedge | V形楔形 | `○   L   ○` |
| Diamond | 菱形 | `◇` |
| Circle | 圆形环绕 Leader | `○○L○○` |

## 快捷键配置

按键映射在 `MAInputActions.cpp` 中定义，可通过修改代码自定义。
