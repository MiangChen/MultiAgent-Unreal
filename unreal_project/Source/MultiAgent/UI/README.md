# UI Architecture

`UI/` 现在按和 `Core/` 一致的方式组织：顶层是 context，context 内部再按 layer 拆分。

## 顶层 context

```text
UI/
├── Core/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Feedback/
│   ├── Infrastructure/
│   └── Modal/
├── HUD/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Domain/
│   ├── Feedback/
│   └── Infrastructure/
├── SceneEditing/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Domain/
│   ├── Feedback/
│   └── Infrastructure/
├── TaskGraph/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Domain/
│   ├── Feedback/
│   └── Infrastructure/
├── SkillAllocation/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Domain/
│   ├── Feedback/
│   └── Infrastructure/
├── Components/
│   ├── Application/
│   ├── Bootstrap/
│   ├── Domain/
│   └── Infrastructure/
└── Setup/
    ├── Application/
    ├── Bootstrap/
    ├── Domain/
    └── Infrastructure/
```

## 规则

- `L1`：widget / HUD shell / UI entry
- `L2`：`Application/`
- `L3`：`Domain/` + `Feedback/`
- `L4`：`Infrastructure/`
- `CR`：`Bootstrap/`

约束：
- widget 壳不直接做 runtime/subsystem 查询
- `Application` 不直接 include `Infrastructure`
- `Domain` 不依赖 runtime 类型
- `Bootstrap` 只做装配

## context 角色

- `UI/Core`
  - UI 根级机制：widget 生命周期、runtime bridge、全局 modal 机制
- `UI/HUD`
  - 常驻 HUD 与 HUD drawing/apply flow
- `UI/SceneEditing`
  - edit / modify / scene list 工作流
- `UI/TaskGraph`
  - 任务图编辑工作流
- `UI/SkillAllocation`
  - 技能分配工作流
- `UI/Components`
  - 可复用 UI 部件集合
- `UI/Setup`
  - 启动与配置流程

## 特例

- `UI/Core/Modal/`
  - 只保留共享 modal 机制和全局决策 modal
  - 业务专属 modal 已归还给各自 owner context
- `UI/TaskGraph`、`UI/SkillAllocation`
  - 是轻量 UI context
  - 它们没有独立 `Runtime/`，运行时数据分别经 `Core/TempData`、`Core/Comm` 与 `UI/Core` 接入

## 当前状态

- `UI/Mode` 已退休，现为 `UI/SceneEditing`
- `UI/Modal` 已退休，共享 modal 机制已并入 `UI/Core/Modal`
- `UI/Legacy` 已退休
