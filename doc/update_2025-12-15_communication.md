# 更新日志: 2025-12-15 通信协议集成

## 概述

本次更新完成了 Task 8 (集成 UI 组件) 的实现，将 UI 组件与 `MACommSubsystem` 通信子系统集成，使所有 UI 输入和按钮事件都能正确上报到后端。

## 修改文件

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `Source/MultiAgent/UI/MASimpleMainWidget.cpp` | 修改 | 集成通信接口 |
| `Source/MultiAgent/UI/MAEmergencyWidget.cpp` | 修改 | 集成通信接口 |
| `config/SimConfig.json` | 修改 | 启用 Mock 模式用于测试 |

## 详细修改

### 1. MASimpleMainWidget 集成

**文件**: `Source/MultiAgent/UI/MASimpleMainWidget.cpp`

#### 1.1 SubmitCommand() 修改
- 调用 `SendUIInputMessage()` 发送 UI 输入消息
- `input_source_id`: `SimpleMainWidget_InputBox`

```cpp
CommSubsystem->SendUIInputMessage(
    TEXT("SimpleMainWidget_InputBox"),  // input_source_id
    Command                              // input_content
);
```

#### 1.2 OnSendButtonClicked() 修改
- 调用 `SendButtonEventMessage()` 发送按钮事件
- `widget_name`: `SimpleMainWidget`
- `button_id`: `btn_send`
- `button_text`: `发送`

```cpp
CommSubsystem->SendButtonEventMessage(
    TEXT("SimpleMainWidget"),   // widget_name
    TEXT("btn_send"),           // button_id
    TEXT("发送")                // button_text
);
```

### 2. MAEmergencyWidget 集成

**文件**: `Source/MultiAgent/UI/MAEmergencyWidget.cpp`

#### 2.1 操作按钮事件上报

| 按钮 | widget_name | button_id | button_text |
|------|-------------|-----------|-------------|
| 按钮1 | EmergencyWidget | btn_expand_search | 扩大搜索范围 |
| 按钮2 | EmergencyWidget | btn_ignore_return | 忽略并返回 |
| 按钮3 | EmergencyWidget | btn_switch_firefight | 切换灭火任务 |
| 发送 | EmergencyWidget | btn_send | 发送 |

#### 2.2 SubmitMessage() 修改
- 调用 `SendUIInputMessage()` 发送 UI 输入消息
- `input_source_id`: `EmergencyWidget_InputBox`

## 消息格式

### UI 输入消息 (ui_input)
```json
{
    "message_type": "ui_input",
    "timestamp": 1702656000000,
    "message_id": "uuid",
    "payload": {
        "input_source_id": "SimpleMainWidget_InputBox",
        "input_content": "用户输入内容"
    }
}
```

### 按钮事件消息 (button_event)
```json
{
    "message_type": "button_event",
    "timestamp": 1702656000000,
    "message_id": "uuid",
    "payload": {
        "widget_name": "EmergencyWidget",
        "button_id": "btn_expand_search",
        "button_text": "扩大搜索范围"
    }
}
```

## 验收测试

### 测试 SimpleMainWidget
1. 按 Z 键打开 UI
2. 输入文本，按回车或点击发送
3. 查看日志：
```
LogMACommSubsystem: SendUIInputMessage: SourceId=SimpleMainWidget_InputBox, Content=xxx
LogMACommSubsystem: SendButtonEventMessage: Widget=SimpleMainWidget, ButtonId=btn_send, ButtonText=发送
```

### 测试 EmergencyWidget
1. 按 X 键打开紧急事件 UI
2. 点击操作按钮或发送消息
3. 查看日志：
```
LogMACommSubsystem: SendButtonEventMessage: Widget=EmergencyWidget, ButtonId=btn_expand_search, ButtonText=扩大搜索范围
LogMACommSubsystem: SendUIInputMessage: SourceId=EmergencyWidget_InputBox, Content=xxx
```

## 新增 UI 指南

为新 Widget 添加通信支持的步骤：

### 1. 添加头文件
```cpp
#include "MACommSubsystem.h"
```

### 2. 按钮事件上报
```cpp
if (UGameInstance* GameInstance = GetGameInstance())
{
    if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
    {
        CommSubsystem->SendButtonEventMessage(
            TEXT("YourWidgetName"),     // widget_name
            TEXT("btn_xxx"),            // button_id
            TEXT("按钮文字")            // button_text
        );
    }
}
```

### 3. 输入消息上报
```cpp
if (UGameInstance* GameInstance = GetGameInstance())
{
    if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
    {
        CommSubsystem->SendUIInputMessage(
            TEXT("YourWidgetName_InputBox"),    // input_source_id
            InputContent                         // input_content
        );
    }
}
```

## 相关 Requirements

- **2.2**: UI_Input_Message 包含 widget 名称作为 input_source_id
- **2.3**: UI_Input_Message 包含实际文本内容作为 input_content
- **3.2**: Button_Event_Message 包含父 widget 名称
- **3.3**: Button_Event_Message 包含按钮程序标识
- **3.4**: Button_Event_Message 包含按钮显示文字

## 配置说明

`config/SimConfig.json` 中的相关配置：

```json
{
    "bUseMockData": true,       // Mock 模式，不发送真实 HTTP 请求
    "bEnablePolling": true,     // 启用轮询
    "PollIntervalSeconds": 1.0  // 轮询间隔
}
```

生产环境请将 `bUseMockData` 设为 `false`。
