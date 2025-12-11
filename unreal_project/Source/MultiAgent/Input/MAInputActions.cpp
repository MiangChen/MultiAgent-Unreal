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
    IA_Pickup = CreateInputAction(TEXT("IA_Pickup"));
    IA_Drop = CreateInputAction(TEXT("IA_Drop"));
    IA_SpawnItem = CreateInputAction(TEXT("IA_SpawnItem"));
    IA_SpawnRobotDog = CreateInputAction(TEXT("IA_SpawnRobotDog"));
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
    IA_ToggleRecording = CreateInputAction(TEXT("IA_ToggleRecording"));
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
    
    // UI 切换
    IA_ToggleMainUI = CreateInputAction(TEXT("IA_ToggleMainUI"));

    // 突发事件系统
    IA_TriggerEmergency = CreateInputAction(TEXT("IA_TriggerEmergency"));
    IA_ToggleEmergencyUI = CreateInputAction(TEXT("IA_ToggleEmergencyUI"));

    // Agent View Mode 移动和视角控制
    IA_Move = CreateInputAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
    IA_Look = CreateInputAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
    IA_LookArrow = CreateInputAction(TEXT("IA_LookArrow"), EInputActionValueType::Axis2D);
    IA_MoveUp = CreateInputAction(TEXT("IA_MoveUp"));
    IA_MoveDown = CreateInputAction(TEXT("IA_MoveDown"));

    // 创建 Input Mapping Context
    DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

    // 添加按键映射
    AddKeyMapping(DefaultMappingContext, IA_LeftClick, EKeys::LeftMouseButton);
    AddKeyMapping(DefaultMappingContext, IA_RightClick, EKeys::RightMouseButton);
    AddKeyMapping(DefaultMappingContext, IA_Pickup, EKeys::P);
    AddKeyMapping(DefaultMappingContext, IA_Drop, EKeys::O);
    AddKeyMapping(DefaultMappingContext, IA_SpawnItem, EKeys::I);
    AddKeyMapping(DefaultMappingContext, IA_SpawnRobotDog, EKeys::T);
    AddKeyMapping(DefaultMappingContext, IA_PrintInfo, EKeys::Y);
    AddKeyMapping(DefaultMappingContext, IA_DestroyLast, EKeys::U);
    AddKeyMapping(DefaultMappingContext, IA_SwitchCamera, EKeys::Tab);
    AddKeyMapping(DefaultMappingContext, IA_ReturnSpectator, EKeys::Zero);
    AddKeyMapping(DefaultMappingContext, IA_StartPatrol, EKeys::G);
    AddKeyMapping(DefaultMappingContext, IA_StartCharge, EKeys::H);
    AddKeyMapping(DefaultMappingContext, IA_StopIdle, EKeys::J);
    AddKeyMapping(DefaultMappingContext, IA_StartCoverage, EKeys::K);
    AddKeyMapping(DefaultMappingContext, IA_StartFollow, EKeys::F);
    // AddKeyMapping(DefaultMappingContext, IA_StartAvoid, EKeys::A);  // 暂时禁用
    AddKeyMapping(DefaultMappingContext, IA_StartFormation, EKeys::B);
    AddKeyMapping(DefaultMappingContext, IA_TakePhoto, EKeys::L);  // L for Lens/Light
    AddKeyMapping(DefaultMappingContext, IA_ToggleRecording, EKeys::R);  // R for Recording
    AddKeyMapping(DefaultMappingContext, IA_ToggleTCPStream, EKeys::V);  // V for Video stream

    // 编组快捷键 (1~5, Ctrl+1~5 由代码检测)
    AddKeyMapping(DefaultMappingContext, IA_ControlGroup1, EKeys::One);
    AddKeyMapping(DefaultMappingContext, IA_ControlGroup2, EKeys::Two);
    AddKeyMapping(DefaultMappingContext, IA_ControlGroup3, EKeys::Three);
    AddKeyMapping(DefaultMappingContext, IA_ControlGroup4, EKeys::Four);
    AddKeyMapping(DefaultMappingContext, IA_ControlGroup5, EKeys::Five);
    AddKeyMapping(DefaultMappingContext, IA_CreateSquad, EKeys::Q);  // Q for sQuad
    AddKeyMapping(DefaultMappingContext, IA_DisbandSquad, EKeys::Q);  // Shift+Q 解散 (Shift 在代码中检测)
    AddKeyMapping(DefaultMappingContext, IA_ToggleMouseMode, EKeys::M);  // M for Mode
    AddKeyMapping(DefaultMappingContext, IA_ToggleMainUI, EKeys::Z);  // Z for UI toggle

    // 突发事件系统
    AddKeyMapping(DefaultMappingContext, IA_TriggerEmergency, EKeys::Hyphen);  // "-" 键触发/结束事件
    AddKeyMapping(DefaultMappingContext, IA_ToggleEmergencyUI, EKeys::X);  // "X" 键切换详情界面

    // Agent View Mode 移动控制 (WASD)
    // W = 前进 (Y+), S = 后退 (Y-), A = 左移 (X-), D = 右移 (X+)
    AddWASDMapping(DefaultMappingContext, IA_Move);

    // Agent View Mode 视角控制 (鼠标)
    AddMouseLookMapping(DefaultMappingContext, IA_Look);

    // Agent View Mode 视角控制 (方向键)
    AddArrowLookMapping(DefaultMappingContext, IA_LookArrow);

    // Agent View Mode 垂直移动 (Space/Ctrl)
    AddKeyMapping(DefaultMappingContext, IA_MoveUp, EKeys::SpaceBar);
    AddKeyMapping(DefaultMappingContext, IA_MoveDown, EKeys::LeftControl);

    UE_LOG(LogTemp, Log, TEXT("[Input] MAInputActions initialized with %d actions"), 35);
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
