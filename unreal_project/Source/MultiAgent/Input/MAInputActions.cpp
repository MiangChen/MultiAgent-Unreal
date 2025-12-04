// MAInputActions.cpp

#include "MAInputActions.h"
#include "InputMappingContext.h"
#include "EnhancedInputLibrary.h"

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

    UE_LOG(LogTemp, Log, TEXT("[Input] MAInputActions initialized with %d actions"), 12);
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
