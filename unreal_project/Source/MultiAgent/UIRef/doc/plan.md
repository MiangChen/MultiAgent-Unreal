开发路线总览
你的项目本质上是一个多机器人仿真平台的可视化与交互层，核心是：

接收外部规划器的 JSON 数据
渲染语义地图和机器人状态
提供用户交互界面
推荐开发顺序
阶段一：基础骨架（1-2天）
目标：让游戏能跑起来，有基本的控制框架

SimGameMode - 设置默认 Controller 和 HUD
SimPlayerController - 基础输入处理，鼠标点击 LineTrace
SimHUD - UI 管理器骨架
验收标准：点击屏幕能在 Log 里看到射线检测结果

阶段二：常驻 UI + 通信（2-3天）
目标：打通数据流

SimTypes.h - 先定义好所有数据结构（这个很重要，后面都依赖它）
SimCommSubsystem - 通信子系统，先用假数据测试
SimMainWidget - 输入框 + 文本显示框
SimGameInstance - 全局访问入口
验收标准：输入框输入文字 → Subsystem 处理 → 显示框显示结果

阶段三：机器人与视角（2-3天）
目标：能看到机器人，能切换视角

SimRobotCharacter - 机器人实体，带摄像机
视角切换逻辑 - 在 Controller 里实现 SetViewTargetWithBlend
键盘控制转发 - 接管机器人时的输入处理
验收标准：按键切换到机器人第一人称，能用键盘控制移动

阶段四：语义地图系统（3-5天，最复杂）
目标：根据 JSON 渲染语义地图

SimSemanticMapManager - 先生成简单几何体
SimCaptureManager - 俯视摄像机
SimSemanticMapWidget - 地图 UI 容器
SimPropertyPanelWidget - 点击元素显示属性
验收标准：JSON → 生成几何体 → 显示在 UI 地图上 → 点击显示属性

阶段五：进阶功能（3-4天）
SimGanttChartWidget - 甘特图
SimEventPopupWidget - 事件弹窗
真实地图小地图 - SceneCapture 实时画面
属性编辑功能 - 修改/删除/添加对象
关键建议
先改 Build.cs - 把所有依赖模块加上，避免后面编译报错
SimTypes.h 要先设计好 - 数据结构是整个系统的基础
每个阶段都要能编译运行 - 不要堆太多代码再测试
C++ 做逻辑，Blueprint 做布局 - Widget 的视觉部分在编辑器里拖