// MAInputActions.cpp

#include "MAInputActions.h"
#include "InputMappingContext.h"
#include "EnhancedInputLibrary.h"
#include "InputModifiers.h"

UMAInputActions* UMAInputActions::Instance = nullptr;

UMAInputActions* UMAInputActions::Get()
{
    if (!Instance)
    {
        Instance = NewObject<UMAInputActions>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
        Instance->Initialize();
    }
    return Instance;
}

void UMAInputActions::Initialize()
{
    // 创建所有 Input Actions
    IA_LeftClick = CreateInputAction(TEXT("IA_LeftClick"));
    IA_RightClick = CreateInputAction(TEXT("IA_RightClick"));
    IA_MiddleClick = CreateInputAction(TEXT("IA_MiddleClick"));
    IA_Pickup = CreateInputAction(TEXT("IA_Pickup"));
    IA_Drop = CreateInputAction(TEXT("IA_Drop"));
    IA_SpawnItem = CreateInputAction(TEXT("IA_SpawnItem"));
    IA_SpawnQuadruped = CreateInputAction(TEXT("IA_SpawnQuadruped"));
    IA_PrintInfo = CreateInputAction(TEXT("IA_PrintInfo"));
    IA_DestroyLast = CreateInputAction(TEXT("IA_DestroyLast"));
    IA_SwitchCamera = CreateInputAction(TEXT("IA_SwitchCamera"));
    IA_ReturnSpectator = CreateInputAction(TEXT("IA_ReturnSpectator"));
    IA_StartPatrol = CreateInputAction(TEXT("IA_StartPatrol"));
    IA_StartCharge = CreateInputAction(TEXT("IA_StartCharge"));
    IA_StopIdle = CreateInputAction(TEXT("IA_StopIdle"));
    IA_StartCoverage = CreateInputAction(TEXT("IA_StartCoverage"));
    IA_StartFollow = CreateInputAction(TEXT("IA_StartFollow"));
    IA_StartAvoid = CreateInputAction(TEXT("IA_StartAvoid"));
    IA_StartFormation = CreateInputAction(TEXT("IA_StartFormation"));
    IA_TakePhoto = CreateInputAction(TEXT("IA_TakePhoto"));
    IA_ToggleTCPStream = CreateInputAction(TEXT("IA_ToggleTCPStream"));

    // 编组快捷键
    IA_ControlGroup1 = CreateInputAction(TEXT("IA_ControlGroup1"));
    IA_ControlGroup2 = CreateInputAction(TEXT("IA_ControlGroup2"));
    IA_ControlGroup3 = CreateInputAction(TEXT("IA_ControlGroup3"));
    IA_ControlGroup4 = CreateInputAction(TEXT("IA_ControlGroup4"));
    IA_ControlGroup5 = CreateInputAction(TEXT("IA_ControlGroup5"));
    IA_CreateSquad = CreateInputAction(TEXT("IA_CreateSquad"));
    IA_DisbandSquad = CreateInputAction(TEXT("IA_DisbandSquad"));
    IA_ToggleMouseMode = CreateInputAction(TEXT("IA_ToggleMouseMode"));
    IA_ToggleModifyMode = CreateInputAction(TEXT("IA_ToggleModifyMode"));

    // UI 切换
    IA_ToggleMainUI = CreateInputAction(TEXT("IA_ToggleMainUI"));
    IA_ToggleSkillAllocationViewer = CreateInputAction(TEXT("IA_ToggleSkillAllocationViewer"));

    // 突发事件系统
    IA_TriggerEmergency = CreateInputAction(TEXT("IA_TriggerEmergency"));
    IA_ToggleEmergencyUI = CreateInputAction(TEXT("IA_ToggleEmergencyUI"));

    // HUD 状态管理输入 (UI Visual Redesign)
    // 注意: 这些输入与 IA_ToggleMainUI/IA_ToggleSkillAllocationViewer/IA_ToggleEmergencyUI 共享按键
    // 但用于新的 HUD 状态管理器系统
    IA_CheckTask = CreateInputAction(TEXT("IA_CheckTask"));
    IA_CheckSkill = CreateInputAction(TEXT("IA_CheckSkill"));
    IA_CheckUnexpected = CreateInputAction(TEXT("IA_CheckUnexpected"));

    // Agent View Mode 移动和视角控制
    IA_Move = CreateInputAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
    IA_Look = CreateInputAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
    IA_LookArrow = CreateInputAction(TEXT("IA_LookArrow"), EInputActionValueType::Axis2D);
    IA_MoveUp = CreateInputAction(TEXT("IA_MoveUp"));
    IA_MoveDown = CreateInputAction(TEXT("IA_MoveDown"));
    IA_Jump = CreateInputAction(TEXT("IA_Jump"));

    // 创建 Input Mapping Contexts
    IMC_RTS = NewObject<UInputMappingContext>(this, TEXT("IMC_RTS"));
    IMC_AgentControl = NewObject<UInputMappingContext>(this, TEXT("IMC_AgentControl"));
    
    // 兼容旧代码
    DefaultMappingContext = IMC_RTS;

    // ========== IMC_RTS 按键映射 (框选、编组、生成等) ==========
    AddKeyMapping(IMC_RTS, IA_LeftClick, EKeys::LeftMouseButton);
    AddKeyMapping(IMC_RTS, IA_RightClick, EKeys::RightMouseButton);
    AddKeyMapping(IMC_RTS, IA_MiddleClick, EKeys::MiddleMouseButton);
    AddKeyMapping(IMC_RTS, IA_Pickup, EKeys::P);
    AddKeyMapping(IMC_RTS, IA_Drop, EKeys::O);
    AddKeyMapping(IMC_RTS, IA_SpawnItem, EKeys::I);
    AddKeyMapping(IMC_RTS, IA_SpawnQuadruped, EKeys::T);
    AddKeyMapping(IMC_RTS, IA_PrintInfo, EKeys::Y);
    AddKeyMapping(IMC_RTS, IA_DestroyLast, EKeys::U);
    AddKeyMapping(IMC_RTS, IA_SwitchCamera, EKeys::Tab);
    AddKeyMapping(IMC_RTS, IA_ReturnSpectator, EKeys::Zero);
    AddKeyMapping(IMC_RTS, IA_StartPatrol, EKeys::G);
    AddKeyMapping(IMC_RTS, IA_StartCharge, EKeys::H);
    AddKeyMapping(IMC_RTS, IA_StopIdle, EKeys::J);
    AddKeyMapping(IMC_RTS, IA_StartCoverage, EKeys::K);
    AddKeyMapping(IMC_RTS, IA_StartFollow, EKeys::F);
    // AddKeyMapping(IMC_RTS, IA_StartAvoid, EKeys::A);  // 暂时禁用
    AddKeyMapping(IMC_RTS, IA_StartFormation, EKeys::B);
    AddKeyMapping(IMC_RTS, IA_TakePhoto, EKeys::L);  // L for Lens/Light
    // TCP 视频流暂时禁用
    // AddKeyMapping(IMC_RTS, IA_ToggleTCPStream, EKeys::V);  // V for Video stream
    
    // Direct Control 视角切换
    AddKeyMapping(IMC_RTS, IA_SwitchCamera, EKeys::C);  // C for Camera switch (额外绑定)
    AddKeyMapping(IMC_RTS, IA_ReturnSpectator, EKeys::V);  // V for View (返回观察者)

    // 编组快捷键 (1~5, Ctrl+1~5 由代码检测)
    AddKeyMapping(IMC_RTS, IA_ControlGroup1, EKeys::One);
    AddKeyMapping(IMC_RTS, IA_ControlGroup2, EKeys::Two);
    AddKeyMapping(IMC_RTS, IA_ControlGroup3, EKeys::Three);
    AddKeyMapping(IMC_RTS, IA_ControlGroup4, EKeys::Four);
    AddKeyMapping(IMC_RTS, IA_ControlGroup5, EKeys::Five);
    AddKeyMapping(IMC_RTS, IA_CreateSquad, EKeys::Q);  // Q for sQuad
    AddKeyMapping(IMC_RTS, IA_DisbandSquad, EKeys::Q);  // Shift+Q 解散 (Shift 在代码中检测)
    AddKeyMapping(IMC_RTS, IA_ToggleMouseMode, EKeys::M);  // M for Edit Mode
    AddKeyMapping(IMC_RTS, IA_ToggleModifyMode, EKeys::Comma);  // , for Modify Mode
    // 注意: Z 和 N 键现在由 HUD 状态管理器处理
    // IA_ToggleMainUI 和 IA_ToggleSkillAllocationViewer 不再直接绑定到 Z/N 键
    // 它们通过 IA_CheckTask 和 IA_CheckSkill 间接触发（点击 Edit 按钮后）
    // AddKeyMapping(IMC_RTS, IA_ToggleMainUI, EKeys::Z);  // 已移除 - 由 IA_CheckTask 处理
    // AddKeyMapping(IMC_RTS, IA_ToggleSkillAllocationViewer, EKeys::N);  // 已移除 - 由 IA_CheckSkill 处理

    // 突发事件系统
    AddKeyMapping(IMC_RTS, IA_TriggerEmergency, EKeys::Hyphen);  // "-" 键触发/结束事件
    // 注意: IA_ToggleEmergencyUI 不再绑定到 X 键，由 IA_CheckUnexpected 处理
    // AddKeyMapping(IMC_RTS, IA_ToggleEmergencyUI, EKeys::X);  // 已移除 - 由 IA_CheckUnexpected 处理

    // HUD 状态管理输入 (UI Visual Redesign)
    // Requirements: 10.1, 10.2, 10.3
    AddKeyMapping(IMC_RTS, IA_CheckTask, EKeys::Z);       // Z 键检查任务图
    AddKeyMapping(IMC_RTS, IA_CheckSkill, EKeys::N);      // N 键检查技能列表
    AddKeyMapping(IMC_RTS, IA_CheckUnexpected, EKeys::X); // X 键检查突发事件

    // 跳跃 (空格键)
    AddKeyMapping(IMC_RTS, IA_Jump, EKeys::SpaceBar);

    // ========== IMC_AgentControl 按键映射 (WASD、视角) ==========
    // 仅在 Agent View Mode 时由 MAAgentInputComponent 添加
    
    // WASD 移动
    AddWASDMapping(IMC_AgentControl, IA_Move);

    // 鼠标视角
    AddMouseLookMapping(IMC_AgentControl, IA_Look);

    // 方向键视角
    AddArrowLookMapping(IMC_AgentControl, IA_LookArrow);

    // 垂直移动 (E上升/Q下降)
    AddKeyMapping(IMC_AgentControl, IA_MoveUp, EKeys::E);
    AddKeyMapping(IMC_AgentControl, IA_MoveDown, EKeys::Q);

    // 跳跃 (Space) - Agent View Mode 下也需要
    AddKeyMapping(IMC_AgentControl, IA_Jump, EKeys::SpaceBar);

    UE_LOG(LogTemp, Log, TEXT("[Input] MAInputActions initialized - IMC_RTS + IMC_AgentControl"));
}

UInputAction* UMAInputActions::CreateInputAction(const FName& Name, EInputActionValueType ValueType)
{
    UInputAction* Action = NewObject<UInputAction>(this, Name);
    Action->ValueType = ValueType;
    return Action;
}

void UMAInputActions::AddKeyMapping(UInputMappingContext* Context, UInputAction* Action, FKey Key)
{
    if (!Context || !Action) return;

    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
    // 可以在这里添加 Modifiers 或 Triggers
}

void UMAInputActions::AddWASDMapping(UInputMappingContext* Context, UInputAction* Action)
{
    if (!Context || !Action) return;

    // W = 前进 (Y+): 需要 Swizzle YXZ 将 1D 值放到 Y 轴
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::W);
        UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
        Swizzle->Order = EInputAxisSwizzle::YXZ;
        Mapping.Modifiers.Add(Swizzle);
    }

    // S = 后退 (Y-): Swizzle YXZ + Negate
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::S);
        UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
        Swizzle->Order = EInputAxisSwizzle::YXZ;
        Mapping.Modifiers.Add(Swizzle);
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        Mapping.Modifiers.Add(Negate);
    }

    // A = 左移 (X-): Negate (默认 1D 值在 X 轴)
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::A);
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        Mapping.Modifiers.Add(Negate);
    }

    // D = 右移 (X+): 无需修改，默认 1D 值在 X 轴
    {
        Context->MapKey(Action, EKeys::D);
    }
}

void UMAInputActions::AddMouseLookMapping(UInputMappingContext* Context, UInputAction* Action)
{
    if (!Context || !Action) return;

    // 鼠标 2D 轴输入
    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::Mouse2D);
    
    // 添加灵敏度修改器 (可选，根据需要调整)
    UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(this);
    Scalar->Scalar = FVector(0.1f, 0.1f, 0.1f);  // 降低灵敏度
    Mapping.Modifiers.Add(Scalar);
}

void UMAInputActions::AddArrowLookMapping(UInputMappingContext* Context, UInputAction* Action)
{
    if (!Context || !Action) return;

    // 左箭头 = 左转 (X-): Negate
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::Left);
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        Mapping.Modifiers.Add(Negate);
    }

    // 右箭头 = 右转 (X+): 无需修改
    {
        Context->MapKey(Action, EKeys::Right);
    }

    // 上箭头 = 抬头 (Y+): Swizzle YXZ
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::Up);
        UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
        Swizzle->Order = EInputAxisSwizzle::YXZ;
        Mapping.Modifiers.Add(Swizzle);
    }

    // 下箭头 = 低头 (Y-): Swizzle YXZ + Negate
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, EKeys::Down);
        UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
        Swizzle->Order = EInputAxisSwizzle::YXZ;
        Mapping.Modifiers.Add(Swizzle);
        UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
        Mapping.Modifiers.Add(Negate);
    }
}
