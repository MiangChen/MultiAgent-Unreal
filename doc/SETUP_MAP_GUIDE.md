# Setup 地图创建指南

## 概述

Setup 地图是游戏启动后的第一个界面，用于配置小队成员（选择机器人类型和数量），然后进入仿真场景。

## 创建步骤

### 1. 创建空地图

1. 打开 UE 编辑器
2. File → New Level → Empty Level
3. 保存到 `Content/Maps/Setup`

### 2. 创建 GameMode 蓝图（推荐）

1. Content Browser → 右键 → Blueprint Class
2. 搜索并选择 `MASetupGameMode` 作为父类
3. 命名为 `BP_SetupGameMode`
4. 保存到 `Content/Blueprints/Core/`

### 3. 配置 Setup 地图

1. 打开 Setup 地图
2. World Settings (窗口右侧)
3. GameMode Override → 选择 `BP_SetupGameMode`

### 4. 设为默认启动地图

1. Edit → Project Settings
2. Maps & Modes 分类
3. Default Maps:
   - Editor Startup Map: `/Game/Maps/Setup`
   - Game Default Map: `/Game/Maps/Setup`

## 流程说明

```
游戏启动
    ↓
加载 Setup 地图 (MASetupGameMode)
    ↓
显示配置界面 (MASetupWidget)
    - 选择场景
    - 添加机器人类型和数量
    ↓
点击"开始仿真"
    ↓
配置保存到 GameInstance
    ↓
加载目标场景地图 (MAGameMode)
    ↓
根据配置生成机器人
```

## 文件结构

```
Content/
├── Maps/
│   ├── Setup.umap              # Setup 地图（空地图）
│   ├── CyberCity.umap          # 仿真场景
│   └── ...
└── Blueprints/
    └── Core/
        └── BP_SetupGameMode.uasset
```

## 注意事项

1. Setup 地图不需要任何场景内容，只显示 UI
2. MASetupHUD 会自动创建 MASetupWidget
3. 场景选择下拉框中的选项需要与实际地图名称对应
4. 如果地图路径不匹配，需要修改 `MASetupHUD.cpp` 中的 `MapPath` 格式

## 调试

如果 Setup 界面不显示：
1. 检查 GameMode 是否正确设置
2. 查看 Output Log 中的 `[MASetupHUD]` 和 `[MASetupWidget]` 日志
3. 确认 HUDClass 是否为 `AMASetupHUD`
