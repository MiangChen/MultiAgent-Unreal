// MAEmergencyModal.cpp
// 紧急事件模态窗口实现
// Requirements: 6.2, 6.3, 12.3, 12.4, 12.5

#include "MAEmergencyModal.h"
#include "../Core/MAUITheme.h"
#include "../../Agent/Character/MACharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Image.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogMAEmergencyModal);

//=============================================================================
// 构造函数
//=============================================================================

UMAEmergencyModal::UMAEmergencyModal(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , EventDetailsContainer(nullptr)
    , SourceAgentText(nullptr)
    , EventTypeText(nullptr)
    , TimestampText(nullptr)
    , LocationText(nullptr)
    , DescriptionContainer(nullptr)
    , DescriptionText(nullptr)
    , UserResponseContainer(nullptr)
    , UserResponseLabel(nullptr)
    , UserResponseInput(nullptr)
{
    // 设置模态类型
    SetModalType(EMAModalType::Emergency);
    
    // 设置模态大小
    ModalWidth = 700.0f;
    ModalHeight = 550.0f;
    
    // 设置按钮文本 - 紧急事件直接进入编辑模式
    ConfirmButtonText = FText::FromString(TEXT("Submit"));
    RejectButtonText = FText::FromString(TEXT("Reject"));
    
    // 不显示编辑按钮 - 紧急事件直接可编辑
    bShowEditButton = false;
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAEmergencyModal::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 紧急事件模态直接进入编辑模式
    SetEditMode(true);
    
    // 绑定基类按钮事件
    OnConfirmClicked.AddDynamic(this, &UMAEmergencyModal::HandleConfirm);
    OnRejectClicked.AddDynamic(this, &UMAEmergencyModal::HandleReject);
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("MAEmergencyModal constructed"));
}

void UMAEmergencyModal::NativeDestruct()
{
    OnConfirmClicked.RemoveDynamic(this, &UMAEmergencyModal::HandleConfirm);
    OnRejectClicked.RemoveDynamic(this, &UMAEmergencyModal::HandleReject);
    
    Super::NativeDestruct();
}

//=============================================================================
// UMABaseModalWidget 重写
//=============================================================================

void UMAEmergencyModal::OnEditModeChanged(bool bEditable)
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Edit mode changed to: %s"), bEditable ? TEXT("true") : TEXT("false"));
    
    // 更新用户响应输入框的可编辑状态
    if (UserResponseInput)
    {
        UserResponseInput->SetIsReadOnly(!bEditable);
    }
}

FText UMAEmergencyModal::GetModalTitleText() const
{
    return FText::FromString(TEXT("Emergency Event"));
}

void UMAEmergencyModal::BuildContentArea(UVerticalBox* InContentContainer)
{
    if (!InContentContainer || !WidgetTree)
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("BuildContentArea: Invalid container or WidgetTree"));
        return;
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Building content area..."));
    
    // 创建事件详情区域
    UBorder* DetailsArea = CreateEventDetailsArea();
    if (DetailsArea)
    {
        InContentContainer->AddChild(DetailsArea);
        
        if (UVerticalBoxSlot* DetailsSlot = Cast<UVerticalBoxSlot>(DetailsArea->Slot))
        {
            DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            DetailsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
        }
    }
    
    // 创建描述区域
    UBorder* DescArea = CreateDescriptionArea();
    if (DescArea)
    {
        InContentContainer->AddChild(DescArea);
        
        if (UVerticalBoxSlot* DescSlot = Cast<UVerticalBoxSlot>(DescArea->Slot))
        {
            DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            DescSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
        }
    }
    
    // 创建用户响应区域
    UBorder* ResponseArea = CreateUserResponseArea();
    if (ResponseArea)
    {
        InContentContainer->AddChild(ResponseArea);
        
        if (UVerticalBoxSlot* ResponseSlot = Cast<UVerticalBoxSlot>(ResponseArea->Slot))
        {
            ResponseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Content area built successfully"));
}

void UMAEmergencyModal::OnThemeApplied()
{
    // 应用主题到事件详情容器
    if (EventDetailsContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        EventDetailsContainer->SetBrushColor(BgColor);
    }
    
    // 应用主题到描述容器
    if (DescriptionContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        DescriptionContainer->SetBrushColor(BgColor);
    }
    
    // 应用主题到用户响应容器
    if (UserResponseContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        UserResponseContainer->SetBrushColor(BgColor);
    }
    
    // 应用主题到文本
    if (Theme)
    {
        FSlateColor TextColor = FSlateColor(Theme->TextColor);
        
        if (SourceAgentText) SourceAgentText->SetColorAndOpacity(TextColor);
        if (EventTypeText) EventTypeText->SetColorAndOpacity(TextColor);
        if (TimestampText) TimestampText->SetColorAndOpacity(TextColor);
        if (LocationText) LocationText->SetColorAndOpacity(TextColor);
        if (DescriptionText) DescriptionText->SetColorAndOpacity(TextColor);
        if (UserResponseLabel) UserResponseLabel->SetColorAndOpacity(TextColor);
    }
}

//=============================================================================
// 公共接口
//=============================================================================

void UMAEmergencyModal::LoadEmergencyEvent(const FMAEmergencyEventData& Data)
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Loading emergency event: Agent=%s, Type=%s"),
        *Data.SourceAgentId, *Data.EventType);
    
    EventData = Data;
    UpdateEventDetailsDisplay();
}

void UMAEmergencyModal::LoadFromAgent(AMACharacter* SourceAgent, const FString& InEventType, const FString& InDescription)
{
    FMAEmergencyEventData Data;
    
    if (SourceAgent)
    {
        Data.SourceAgentId = SourceAgent->AgentID;
        Data.SourceAgentName = SourceAgent->GetName();
        Data.Location = SourceAgent->GetActorLocation();
        SourceAgentRef = SourceAgent;
    }
    else
    {
        Data.SourceAgentId = TEXT("Unknown");
        Data.SourceAgentName = TEXT("Unknown Agent");
        Data.Location = FVector::ZeroVector;
    }
    
    Data.EventType = InEventType;
    Data.Description = InDescription;
    Data.Timestamp = FDateTime::Now();
    
    LoadEmergencyEvent(Data);
}

FMAEmergencyEventData UMAEmergencyModal::GetEmergencyEventData() const
{
    FMAEmergencyEventData Data = EventData;
    
    // 获取用户响应
    if (UserResponseInput)
    {
        Data.UserResponse = UserResponseInput->GetText().ToString();
    }
    
    return Data;
}

FString UMAEmergencyModal::GetUserResponse() const
{
    if (UserResponseInput)
    {
        return UserResponseInput->GetText().ToString();
    }
    return FString();
}

void UMAEmergencyModal::SetUserResponse(const FString& Response)
{
    if (UserResponseInput)
    {
        UserResponseInput->SetText(FText::FromString(Response));
    }
    EventData.UserResponse = Response;
}

//=============================================================================
// 内部方法
//=============================================================================

UBorder* UMAEmergencyModal::CreateEventDetailsArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    EventDetailsContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EventDetailsContainer"));
    if (!EventDetailsContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    EventDetailsContainer->SetBrushColor(BgColor);
    EventDetailsContainer->SetPadding(FMargin(12.0f));
    
    // 创建垂直布局
    UVerticalBox* DetailsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailsVBox"));
    if (DetailsVBox)
    {
        EventDetailsContainer->AddChild(DetailsVBox);
        
        // 创建标题
        UTextBlock* DetailsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailsTitle"));
        if (DetailsTitle)
        {
            DetailsVBox->AddChild(DetailsTitle);
            DetailsTitle->SetText(FText::FromString(TEXT("Event Details")));
            
            FSlateFontInfo TitleFont = DetailsTitle->GetFont();
            TitleFont.Size = 16;
            DetailsTitle->SetFont(TitleFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            DetailsTitle->SetColorAndOpacity(FSlateColor(TextColor));
            
            if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(DetailsTitle->Slot))
            {
                TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
        }
        
        // 创建详情行
        UHorizontalBox* AgentRow = CreateDetailRow(TEXT("Source Agent:"), SourceAgentText);
        if (AgentRow) DetailsVBox->AddChild(AgentRow);
        
        UHorizontalBox* TypeRow = CreateDetailRow(TEXT("Event Type:"), EventTypeText);
        if (TypeRow) DetailsVBox->AddChild(TypeRow);
        
        UHorizontalBox* TimeRow = CreateDetailRow(TEXT("Timestamp:"), TimestampText);
        if (TimeRow) DetailsVBox->AddChild(TimeRow);
        
        UHorizontalBox* LocationRow = CreateDetailRow(TEXT("Location:"), LocationText);
        if (LocationRow) DetailsVBox->AddChild(LocationRow);
    }
    
    return EventDetailsContainer;
}

UBorder* UMAEmergencyModal::CreateDescriptionArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    DescriptionContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DescriptionContainer"));
    if (!DescriptionContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    DescriptionContainer->SetBrushColor(BgColor);
    DescriptionContainer->SetPadding(FMargin(12.0f));
    
    // 创建垂直布局
    UVerticalBox* DescVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DescVBox"));
    if (DescVBox)
    {
        DescriptionContainer->AddChild(DescVBox);
        
        // 创建标题
        UTextBlock* DescTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescTitle"));
        if (DescTitle)
        {
            DescVBox->AddChild(DescTitle);
            DescTitle->SetText(FText::FromString(TEXT("Description")));
            
            FSlateFontInfo TitleFont = DescTitle->GetFont();
            TitleFont.Size = 16;
            DescTitle->SetFont(TitleFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            DescTitle->SetColorAndOpacity(FSlateColor(TextColor));
            
            if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(DescTitle->Slot))
            {
                TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
        }
        
        // 创建描述文本
        DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
        if (DescriptionText)
        {
            DescVBox->AddChild(DescriptionText);
            DescriptionText->SetText(FText::FromString(TEXT("No description available.")));
            DescriptionText->SetAutoWrapText(true);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
            DescriptionText->SetColorAndOpacity(FSlateColor(TextColor));
        }
    }
    
    return DescriptionContainer;
}

UBorder* UMAEmergencyModal::CreateUserResponseArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    UserResponseContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UserResponseContainer"));
    if (!UserResponseContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    UserResponseContainer->SetBrushColor(BgColor);
    UserResponseContainer->SetPadding(FMargin(12.0f));
    
    // 创建垂直布局
    UVerticalBox* ResponseVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResponseVBox"));
    if (ResponseVBox)
    {
        UserResponseContainer->AddChild(ResponseVBox);
        
        // 创建标题
        UserResponseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UserResponseLabel"));
        if (UserResponseLabel)
        {
            ResponseVBox->AddChild(UserResponseLabel);
            UserResponseLabel->SetText(FText::FromString(TEXT("User Response (Enter your intervention)")));
            
            FSlateFontInfo TitleFont = UserResponseLabel->GetFont();
            TitleFont.Size = 16;
            UserResponseLabel->SetFont(TitleFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            UserResponseLabel->SetColorAndOpacity(FSlateColor(TextColor));
            
            if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(UserResponseLabel->Slot))
            {
                TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
        }
        
        // 创建用户响应输入框
        UserResponseInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
            UMultiLineEditableTextBox::StaticClass(), TEXT("UserResponseInput"));
        if (UserResponseInput)
        {
            ResponseVBox->AddChild(UserResponseInput);
            UserResponseInput->SetHintText(FText::FromString(TEXT("Enter your intervention response here...")));
            UserResponseInput->SetIsReadOnly(false); // 默认可编辑
            
            // 设置样式：纯黑色，字号 12
            FEditableTextBoxStyle ResponseStyle;
            FSlateColor BlackColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
            ResponseStyle.SetForegroundColor(BlackColor);
            ResponseStyle.SetFocusedForegroundColor(BlackColor);
            FSlateFontInfo ResponseFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
            ResponseStyle.SetFont(ResponseFont);
            UserResponseInput->WidgetStyle = ResponseStyle;
            
            if (UVerticalBoxSlot* InputSlot = Cast<UVerticalBoxSlot>(UserResponseInput->Slot))
            {
                InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }
        }
    }
    
    return UserResponseContainer;
}

void UMAEmergencyModal::UpdateEventDetailsDisplay()
{
    // 更新来源 Agent
    if (SourceAgentText)
    {
        FString AgentDisplay = EventData.SourceAgentName.IsEmpty() 
            ? EventData.SourceAgentId 
            : FString::Printf(TEXT("%s (%s)"), *EventData.SourceAgentName, *EventData.SourceAgentId);
        SourceAgentText->SetText(FText::FromString(AgentDisplay));
    }
    
    // 更新事件类型
    if (EventTypeText)
    {
        EventTypeText->SetText(FText::FromString(EventData.EventType.IsEmpty() ? TEXT("Unknown") : EventData.EventType));
    }
    
    // 更新时间戳
    if (TimestampText)
    {
        FString TimeString = EventData.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M:%S"));
        TimestampText->SetText(FText::FromString(TimeString));
    }
    
    // 更新位置
    if (LocationText)
    {
        FString LocationString = FString::Printf(TEXT("X: %.1f, Y: %.1f, Z: %.1f"),
            EventData.Location.X, EventData.Location.Y, EventData.Location.Z);
        LocationText->SetText(FText::FromString(LocationString));
    }
    
    // 更新描述
    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(
            EventData.Description.IsEmpty() ? TEXT("No description available.") : EventData.Description));
    }
    
    // 更新用户响应
    if (UserResponseInput && !EventData.UserResponse.IsEmpty())
    {
        UserResponseInput->SetText(FText::FromString(EventData.UserResponse));
    }
}

void UMAEmergencyModal::HandleConfirm()
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Emergency event confirmed"));
    
    // 获取当前数据（包含用户响应）
    FMAEmergencyEventData Data = GetEmergencyEventData();
    
    // 广播确认事件
    OnEmergencyConfirmed.Broadcast(Data);
}

void UMAEmergencyModal::HandleReject()
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Emergency event rejected"));
    
    // 广播拒绝事件
    OnEmergencyRejected.Broadcast();
}

UHorizontalBox* UMAEmergencyModal::CreateDetailRow(const FString& Label, UTextBlock*& OutValueText)
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!Row)
    {
        return nullptr;
    }
    
    // 创建标签
    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (LabelText)
    {
        Row->AddChild(LabelText);
        LabelText->SetText(FText::FromString(Label));
        
        FSlateFontInfo Font = LabelText->GetFont();
        Font.Size = 14;
        LabelText->SetFont(Font);
        
        FLinearColor LabelColor = Theme ? Theme->TextColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
        LabelColor.A = 0.8f;
        LabelText->SetColorAndOpacity(FSlateColor(LabelColor));
        
        if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(LabelText->Slot))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        }
    }
    
    // 创建值文本
    OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (OutValueText)
    {
        Row->AddChild(OutValueText);
        OutValueText->SetText(FText::FromString(TEXT("-")));
        
        FSlateFontInfo Font = OutValueText->GetFont();
        Font.Size = 14;
        OutValueText->SetFont(Font);
        
        FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
        OutValueText->SetColorAndOpacity(FSlateColor(TextColor));
        
        if (UHorizontalBoxSlot* ValueSlot = Cast<UHorizontalBoxSlot>(OutValueText->Slot))
        {
            ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }
    
    // 设置行间距
    if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Row->Slot))
    {
        RowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 2.0f));
    }
    
    return Row;
}

FString UMAEmergencyModal::GetResponseJson() const
{
    // 构建紧急事件响应 JSON
    TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    
    // 添加事件数据
    JsonObject->SetStringField(TEXT("source_agent_id"), EventData.SourceAgentId);
    JsonObject->SetStringField(TEXT("source_agent_name"), EventData.SourceAgentName);
    JsonObject->SetStringField(TEXT("event_type"), EventData.EventType);
    JsonObject->SetStringField(TEXT("timestamp"), EventData.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
    JsonObject->SetStringField(TEXT("description"), EventData.Description);
    
    // 添加位置
    TSharedRef<FJsonObject> LocationObject = MakeShared<FJsonObject>();
    LocationObject->SetNumberField(TEXT("x"), EventData.Location.X);
    LocationObject->SetNumberField(TEXT("y"), EventData.Location.Y);
    LocationObject->SetNumberField(TEXT("z"), EventData.Location.Z);
    JsonObject->SetObjectField(TEXT("location"), LocationObject);
    
    // 添加用户响应
    FString UserResponse = GetUserResponse();
    JsonObject->SetStringField(TEXT("user_response"), UserResponse);
    
    // 序列化为 JSON 字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject, Writer);
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("GetResponseJson: Generated response JSON"));
    
    return OutputString;
}
