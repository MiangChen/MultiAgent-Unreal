# Session 0 - 基础骨架搭建

**日期**: 2025年12月6日

## 本次完成的工作

### 1. 解决编译错误

**问题**: 链接器报错 `undefined symbol: UUserWidget::GetPrivateStaticClass()` 等

**原因**: `uidev.Build.cs` 缺少 UMG 相关模块依赖

**解决方案**: 在 `uidev.Build.cs` 中添加了以下模块：
```csharp
"UMG",           // UI系统
"Slate",         // UI底层
"SlateCore"      // UI核心
```

### 2. C++ 代码完成情况

已创建的 C++ 类（位于 `Source/uidev/`）：

| 文件 | 作用 |
|------|------|
| `SimGameMode.h/.cpp` | 游戏模式，设置 PlayerController 和 HUD |
| `SimPlayerController.h/.cpp` | 玩家控制器，处理输入和射线检测 |
| `SimHUD.h/.cpp` | HUD 管理器，负责创建和管理所有 UI Widget |

### 3. 虚幻编辑器中创建的内容

在 `Content/Sim/` 目录下创建：

**蓝图类**（`Content/Sim/Blueprints/`）：
- `BP_SimHUD` - SimHUD 的蓝图子类，用于配置 Widget 类引用
- `BP_SimGameMode` - SimGameMode 的蓝图子类，设置 HUD Class 为 BP_SimHUD

**Widget Blueprint**（`Content/Sim/UI/`）：
- `WBP_MainWidget` - 主界面（输入框+文本显示）
- `WBP_SemanticMap` - 语义地图
- `WBP_GanttChart` - 甘特图
- `WBP_PropertyPanel` - 属性面板
- `WBP_EventPopup` - 事件弹窗
- `WBP_RealMap` - 真实地图

### 4. 输入系统配置

**Enhanced Input 资产**（`Content/Sim/Input/` 或 `Content/TopDown/Input/`）：

| 资产名 | 类型 | 作用 |
|--------|------|------|
| `IMC_Default` | Input Mapping Context | 默认输入映射上下文 |
| `IA_Move` | Input Action | WASD 移动 |
| `IA_Click` | Input Action | 鼠标左键点击 |
| `IA_ToggleSemanticMap` | Input Action | M 键切换语义地图 |
| `IA_ToggleGanttChart` | Input Action | G 键切换甘特图 |
| `IA_ToggleRealMap` | Input Action | R 键切换真实地图 |

**控制器蓝图**：
- `BP_TopDownPlayerController` - 原有的俯视角控制器
- `BP_SimPlayerController` - SimPlayerController 的蓝图子类，配置了 `DefaultMappingContext` 指向 `IMC_Default`

**按键绑定**：
| 按键 | 功能 |
|------|------|
| W/A/S/D | 移动 |
| 鼠标左键 | 点击交互（射线检测） |
| M | 切换语义地图显示/隐藏 |
| G | 切换甘特图显示/隐藏 |
| R | 切换真实地图显示/隐藏 |

### 5. GameMode 配置步骤

1. 在 `BP_SimHUD` 的 Details 面板 → `UI Classes` 分类下，将 6 个 Widget 类引用指向对应的 WBP
2. 在 `BP_SimGameMode` 的 Details 面板中，将 `HUD Class` 设置为 `BP_SimHUD`
3. 在 `BP_SimGameMode` 的 Details 面板中，将 `Player Controller Class` 设置为 `BP_SimPlayerController`
4. 在关卡的 `World Settings` 中，将 `GameMode Override` 设置为 `BP_SimGameMode`

---

## 当前开发进度

**阶段一：基础骨架** ✅ 已完成

- [x] SimGameMode - 设置默认 Controller 和 HUD
- [x] SimPlayerController - 基础输入处理，鼠标点击 LineTrace
- [x] SimHUD - UI 管理器骨架
- [ ] 验收测试 - 点击屏幕能在 Log 里看到射线检测结果

---

## 下一步工作

### 阶段一验收测试

1. 启动游戏（Play 或 Alt+P）
2. 打开 Output Log（Window → Developer Tools → Output Log）
3. 点击屏幕，检查是否有日志输出：
   - `LogSimController: SimPlayerController: Input Mapping Context added`
   - `LogSimHUD: SimHUD initialized`
   - 点击时的射线检测结果

### 阶段二：常驻 UI + 通信

- SimTypes.h - 定义数据结构
- SimCommSubsystem - 通信子系统
- SimMainWidget - 输入框 + 文本显示框
- SimGameInstance - 全局访问入口

---

## 注意事项

1. **为什么要用蓝图子类？**
   - `SimHUD` 有 6 个 `UPROPERTY(EditDefaultsOnly)` 的 Widget 类引用
   - 这些需要在编辑器里配置，直接用 C++ 类无法设置
   - 所以必须：`BP_SimGameMode` → `BP_SimHUD` → 各个 Widget Blueprint

2. **后续可能需要的模块**（在 Build.cs 中添加）：
   ```csharp
   "Json",          // JSON解析
   "JsonUtilities", // JSON工具
   "HTTP",          // 网络请求
   "ProceduralMeshComponent" // 动态生成网格
   ```
