// MAEmergencyModal.cpp
// 紧急事件模态窗口实现
// Requirements: 6.2, 6.3, 12.3, 12.4, 12.5

#include "MAEmergencyModal.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Components/MAStyledButton.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Blueprint/WidgetTree.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY(LogMAEmergencyModal);

//=============================================================================
// FMAEmergencyEventData 方法实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 2.2, 2.4
//=============================================================================

bool FMAEmergencyEventData::IsValid() const
{
    // EventType must not be empty
    if (EventType.IsEmpty())
    {
        return false;
    }
    
    // Timestamp must be valid (non-zero ticks)
    if (Timestamp.GetTicks() == 0)
    {
        return false;
    }
    
    return true;
}

void FMAEmergencyEventData::ApplyDefaults()
{
    // Apply default for SourceAgentId
    if (SourceAgentId.IsEmpty())
    {
        SourceAgentId = TEXT("System");
    }
    
    // Apply default for SourceAgentName
    if (SourceAgentName.IsEmpty())
    {
        SourceAgentName = TEXT("Unknown Agent");
    }
    
    // Apply default for EventType
    if (EventType.IsEmpty())
    {
        EventType = TEXT("unknown");
    }
    
    // Apply default for Description
    if (Description.IsEmpty())
    {
        Description = TEXT("No description provided");
    }
    
    // Apply default for Timestamp
    if (Timestamp.GetTicks() == 0)
    {
        Timestamp = FDateTime::Now();
    }
    
    // Apply default for AvailableOptions - Requirements: 1.2
    if (AvailableOptions.Num() == 0)
    {
        AvailableOptions.Add(TEXT("Acknowledge"));
    }
}

FString FMAEmergencyEventData::ToJson() const
{
    TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    
    // Basic fields
    JsonObject->SetStringField(TEXT("agent_id"), SourceAgentId);
    JsonObject->SetStringField(TEXT("agent_name"), SourceAgentName);
    JsonObject->SetStringField(TEXT("event_type"), EventType);
    JsonObject->SetStringField(TEXT("description"), Description);
    JsonObject->SetStringField(TEXT("timestamp"), FString::Printf(TEXT("%lld"), Timestamp.ToUnixTimestamp()));
    JsonObject->SetStringField(TEXT("user_response"), UserResponse);
    JsonObject->SetStringField(TEXT("selected_option"), SelectedOption);
    JsonObject->SetStringField(TEXT("user_input_text"), UserInputText);
    
    // Location as nested object
    TSharedRef<FJsonObject> LocationObject = MakeShared<FJsonObject>();
    LocationObject->SetNumberField(TEXT("x"), Location.X);
    LocationObject->SetNumberField(TEXT("y"), Location.Y);
    LocationObject->SetNumberField(TEXT("z"), Location.Z);
    JsonObject->SetObjectField(TEXT("location"), LocationObject);
    
    // AvailableOptions as array
    TArray<TSharedPtr<FJsonValue>> OptionsArray;
    for (const FString& Option : AvailableOptions)
    {
        OptionsArray.Add(MakeShared<FJsonValueString>(Option));
    }
    JsonObject->SetArrayField(TEXT("available_options"), OptionsArray);
    
    // Serialize to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject, Writer);
    
    return OutputString;
}

bool FMAEmergencyEventData::FromJson(const FString& JsonString, FMAEmergencyEventData& OutData)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("FromJson: Failed to parse JSON string"));
        return false;
    }
    
    return FromJsonObject(JsonObject, OutData);
}

bool FMAEmergencyEventData::FromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FMAEmergencyEventData& OutData)
{
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("FromJsonObject: Invalid JSON object"));
        return false;
    }
    
    // Parse basic string fields
    OutData.SourceAgentId = JsonObject->GetStringField(TEXT("agent_id"));
    OutData.SourceAgentName = JsonObject->GetStringField(TEXT("agent_name"));
    OutData.EventType = JsonObject->GetStringField(TEXT("event_type"));
    OutData.Description = JsonObject->GetStringField(TEXT("description"));
    OutData.UserResponse = JsonObject->GetStringField(TEXT("user_response"));
    OutData.SelectedOption = JsonObject->GetStringField(TEXT("selected_option"));
    OutData.UserInputText = JsonObject->GetStringField(TEXT("user_input_text"));
    
    // Parse timestamp
    FString TimestampStr;
    if (JsonObject->TryGetStringField(TEXT("timestamp"), TimestampStr))
    {
        int64 UnixTimestamp = FCString::Atoi64(*TimestampStr);
        if (UnixTimestamp > 0)
        {
            OutData.Timestamp = FDateTime::FromUnixTimestamp(UnixTimestamp);
        }
        else
        {
            OutData.Timestamp = FDateTime::Now();
        }
    }
    else
    {
        OutData.Timestamp = FDateTime::Now();
    }
    
    // Parse location
    const TSharedPtr<FJsonObject>* LocationObject;
    if (JsonObject->TryGetObjectField(TEXT("location"), LocationObject))
    {
        OutData.Location.X = (*LocationObject)->GetNumberField(TEXT("x"));
        OutData.Location.Y = (*LocationObject)->GetNumberField(TEXT("y"));
        OutData.Location.Z = (*LocationObject)->GetNumberField(TEXT("z"));
    }
    else
    {
        OutData.Location = FVector::ZeroVector;
    }
    
    // Parse available_options array - Requirements: 1.2
    const TArray<TSharedPtr<FJsonValue>>* OptionsArray;
    if (JsonObject->TryGetArrayField(TEXT("available_options"), OptionsArray))
    {
        OutData.AvailableOptions.Empty();
        for (const TSharedPtr<FJsonValue>& OptionValue : *OptionsArray)
        {
            FString OptionStr;
            if (OptionValue->TryGetString(OptionStr))
            {
                OutData.AvailableOptions.Add(OptionStr);
            }
        }
    }
    
    return true;
}

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
    , OptionsContainer(nullptr)
    , OptionsVBox(nullptr)
    , SelectedOptionIndex(-1)
    , CameraViewContainer(nullptr)
    , CameraFeedImage(nullptr)
{
    // 设置模态类型
    SetModalType(EMAModalType::Emergency);
    
    // 设置模态大小 - 大幅增大以匹配参考图的红色框
    ModalWidth = 1400.0f;
    ModalHeight = 750.0f;
    
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
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Building content area with new layout (reference image style)..."));
    
    // 创建主水平布局：左侧相机视图 + 右侧内容
    UHorizontalBox* MainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHBox"));
    if (!MainHBox)
    {
        return;
    }
    InContentContainer->AddChild(MainHBox);
    
    if (UVerticalBoxSlot* MainSlot = Cast<UVerticalBoxSlot>(MainHBox->Slot))
    {
        MainSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    
    //=========================================================================
    // 左侧：相机视图区域 (黄色区域 - 占据左侧大部分空间)
    //=========================================================================
    UBorder* CameraArea = CreateCameraViewArea();
    if (CameraArea)
    {
        MainHBox->AddChild(CameraArea);
        
        if (UHorizontalBoxSlot* CameraSlot = Cast<UHorizontalBoxSlot>(CameraArea->Slot))
        {
            // 相机视图占据约 55% 的宽度
            CameraSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CameraSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
            CameraSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }
    
    //=========================================================================
    // 右侧：内容区域（垂直布局）
    //=========================================================================
    UVerticalBox* RightContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightContentVBox"));
    if (RightContentVBox)
    {
        MainHBox->AddChild(RightContentVBox);
        
        if (UHorizontalBoxSlot* RightSlot = Cast<UHorizontalBoxSlot>(RightContentVBox->Slot))
        {
            // 右侧内容占据约 45% 的宽度
            RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        
        //=====================================================================
        // 右上区域：Event Details 和 Description 水平并排 (红色 + 橙色)
        //=====================================================================
        UHorizontalBox* TopRowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRowHBox"));
        if (TopRowHBox)
        {
            RightContentVBox->AddChild(TopRowHBox);
            
            if (UVerticalBoxSlot* TopRowSlot = Cast<UVerticalBoxSlot>(TopRowHBox->Slot))
            {
                TopRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                TopRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
            
            // Event Details (红色区域)
            UBorder* DetailsArea = CreateEventDetailsArea();
            if (DetailsArea)
            {
                TopRowHBox->AddChild(DetailsArea);
                
                if (UHorizontalBoxSlot* DetailsSlot = Cast<UHorizontalBoxSlot>(DetailsArea->Slot))
                {
                    DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    DetailsSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
                }
            }
            
            // Description (橙色区域)
            UBorder* DescArea = CreateDescriptionArea();
            if (DescArea)
            {
                TopRowHBox->AddChild(DescArea);
                
                if (UHorizontalBoxSlot* DescSlot = Cast<UHorizontalBoxSlot>(DescArea->Slot))
                {
                    DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    DescSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
                }
            }
        }
        
        //=====================================================================
        // 右下区域：User Response 和 Options 水平并排 (绿色 + 紫色)
        //=====================================================================
        UHorizontalBox* BottomRowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRowHBox"));
        if (BottomRowHBox)
        {
            RightContentVBox->AddChild(BottomRowHBox);
            
            if (UVerticalBoxSlot* BottomRowSlot = Cast<UVerticalBoxSlot>(BottomRowHBox->Slot))
            {
                BottomRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }
            
            // User Response (绿色区域) - 占据更多空间
            UBorder* ResponseArea = CreateUserResponseArea();
            if (ResponseArea)
            {
                BottomRowHBox->AddChild(ResponseArea);
                
                if (UHorizontalBoxSlot* ResponseSlot = Cast<UHorizontalBoxSlot>(ResponseArea->Slot))
                {
                    ResponseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    ResponseSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
                    ResponseSlot->SetVerticalAlignment(VAlign_Fill);
                }
            }
            
            // Options (紫色区域) - 垂直排列的按钮
            UBorder* OptionsArea = CreateOptionsArea();
            if (OptionsArea)
            {
                BottomRowHBox->AddChild(OptionsArea);
                
                if (UHorizontalBoxSlot* OptionsSlot = Cast<UHorizontalBoxSlot>(OptionsArea->Slot))
                {
                    OptionsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                    OptionsSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
                    OptionsSlot->SetVerticalAlignment(VAlign_Fill);
                }
            }
        }
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Content area built successfully with new layout"));
}

void UMAEmergencyModal::OnThemeApplied()
{
    // 应用主题到事件详情容器 - 使用圆角效果
    if (EventDetailsContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(EventDetailsContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 应用主题到描述容器 - 使用圆角效果
    if (DescriptionContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(DescriptionContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 应用主题到选项容器 - Requirements: 5.4 - 使用圆角效果
    if (OptionsContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(OptionsContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 应用主题到用户响应容器 - 使用圆角效果
    if (UserResponseContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(UserResponseContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 应用主题到相机视图容器 - 使用圆角效果
    if (CameraViewContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(CameraViewContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
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
    
    // 更新选项按钮样式
    UpdateOptionButtonStyles();
}

//=============================================================================
// 公共接口
//=============================================================================

void UMAEmergencyModal::LoadEmergencyEvent(const FMAEmergencyEventData& Data)
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Loading emergency event: Agent=%s, Type=%s, Options=%d"),
        *Data.SourceAgentId, *Data.EventType, Data.AvailableOptions.Num());
    
    EventData = Data;
    
    // 重置选中状态
    SelectedOptionIndex = -1;
    
    // 更新显示
    UpdateEventDetailsDisplay();
    UpdateOptionsDisplay();
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
        FText InputTextValue = UserResponseInput->GetText();
        FString InputString = InputTextValue.ToString();
        Data.UserResponse = InputString;
        Data.UserInputText = InputString;
        UE_LOG(LogMAEmergencyModal, Log, TEXT("GetEmergencyEventData: UserResponseInput valid, text='%s', IsEmpty=%d"), 
            *InputString, InputString.IsEmpty());
    }
    else
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("GetEmergencyEventData: UserResponseInput is NULL!"));
    }
    
    // 包含选中的选项 - Requirements: 6.5
    if (SelectedOptionIndex >= 0 && SelectedOptionIndex < EventData.AvailableOptions.Num())
    {
        Data.SelectedOption = EventData.AvailableOptions[SelectedOptionIndex];
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("GetEmergencyEventData: Returning Data with SelectedOption='%s', UserInputText='%s'"),
        *Data.SelectedOption, *Data.UserInputText);
    
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
    
    // 设置背景颜色并应用圆角效果
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(EventDetailsContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
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
            TitleFont.Size = 14;
            TitleFont.TypefaceFontName = FName("Bold");
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
    
    // 设置背景颜色并应用圆角效果
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(DescriptionContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
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
            TitleFont.Size = 14;
            TitleFont.TypefaceFontName = FName("Bold");
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
            
            FSlateFontInfo DescFont = DescriptionText->GetFont();
            DescFont.Size = 12;
            DescFont.TypefaceFontName = FName("Bold");
            DescriptionText->SetFont(DescFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
            DescriptionText->SetColorAndOpacity(FSlateColor(TextColor));
        }
    }
    
    return DescriptionContainer;
}

UBorder* UMAEmergencyModal::CreateOptionsArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框 - Requirements: 5.3
    OptionsContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OptionsContainer"));
    if (!OptionsContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色并应用圆角效果 - 与其他区域保持一致
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(OptionsContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    OptionsContainer->SetPadding(FMargin(10.0f));
    
    // 创建垂直布局
    OptionsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsVBox"));
    if (OptionsVBox)
    {
        OptionsContainer->AddChild(OptionsVBox);
        
        // 选项按钮将在 UpdateOptionsDisplay 中动态创建
        // 不再显示标题，直接显示按钮以节省空间
    }
    
    return OptionsContainer;
}

void UMAEmergencyModal::UpdateOptionsDisplay()
{
    if (!OptionsVBox || !WidgetTree)
    {
        return;
    }
    
    // 清除所有现有的选项按钮
    OptionsVBox->ClearChildren();
    
    // 解绑旧按钮的事件
    for (UMAStyledButton* OldButton : OptionButtons)
    {
        if (OldButton)
        {
            OldButton->OnClicked.RemoveAll(this);
        }
    }
    
    OptionButtons.Empty();
    OptionButtonBorders.Empty();
    OptionButtonTexts.Empty();
    
    // 为每个可用选项创建按钮 - Requirements: 5.3, 6.1
    // 使用 UMAStyledButton 以统一样式与 Reject/Confirm 按钮
    for (int32 i = 0; i < EventData.AvailableOptions.Num(); ++i)
    {
        const FString& OptionText = EventData.AvailableOptions[i];
        
        // 创建样式化按钮 - 与 Reject/Confirm 按钮样式一致
        UMAStyledButton* OptionButton = CreateWidget<UMAStyledButton>(this, UMAStyledButton::StaticClass());
        if (OptionButton)
        {
            OptionsVBox->AddChild(OptionButton);
            
            // 设置按钮文本
            OptionButton->SetButtonText(FText::FromString(OptionText));
            
            // 默认使用 Secondary 样式（灰色），选中时会更新为 Primary
            OptionButton->SetButtonStyle(EMAButtonStyle::Secondary);
            
            // 应用主题
            if (Theme)
            {
                OptionButton->ApplyTheme(Theme);
            }
            
            // 绑定点击事件
            OptionButton->OnClicked.AddDynamic(this, &UMAEmergencyModal::HandleOptionButtonClicked);
            
            // 存储按钮引用
            OptionButtons.Add(OptionButton);
            
            // 设置按钮间距
            if (UVerticalBoxSlot* ButtonSlot = Cast<UVerticalBoxSlot>(OptionButton->Slot))
            {
                ButtonSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 3.0f));
                ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
            }
        }
    }
    
    // 如果没有选项，显示默认提示
    if (EventData.AvailableOptions.Num() == 0)
    {
        UTextBlock* NoOptionsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoOptionsText"));
        if (NoOptionsText)
        {
            OptionsVBox->AddChild(NoOptionsText);
            NoOptionsText->SetText(FText::FromString(TEXT("No options")));
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
            TextColor.A = 0.7f;
            NoOptionsText->SetColorAndOpacity(FSlateColor(TextColor));
        }
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Updated options display with %d styled buttons"), EventData.AvailableOptions.Num());
}

void UMAEmergencyModal::OnOptionButtonClicked(int32 OptionIndex)
{
    if (OptionIndex < 0 || OptionIndex >= EventData.AvailableOptions.Num())
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("Invalid option index: %d"), OptionIndex);
        return;
    }
    
    // 更新选中索引 - Requirements: 6.1
    SelectedOptionIndex = OptionIndex;
    EventData.SelectedOption = EventData.AvailableOptions[OptionIndex];
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Option selected and confirmed: %d - %s"), OptionIndex, *EventData.SelectedOption);
    
    // 更新按钮样式以显示选中状态
    UpdateOptionButtonStyles();
    
    // 首先直接从输入框获取用户输入文本
    FString UserInputText;
    if (UserResponseInput)
    {
        UserInputText = UserResponseInput->GetText().ToString();
        UE_LOG(LogMAEmergencyModal, Log, TEXT("OnOptionButtonClicked: Direct read from UserResponseInput: '%s'"), *UserInputText);
    }
    else
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("OnOptionButtonClicked: UserResponseInput is NULL!"));
    }
    
    // 获取当前数据（包含用户响应）
    FMAEmergencyEventData Data = GetEmergencyEventData();
    
    // 确保选中的选项被包含
    Data.SelectedOption = EventData.AvailableOptions[OptionIndex];
    
    // 确保用户输入文本被包含（双重保险）
    if (!UserInputText.IsEmpty())
    {
        Data.UserInputText = UserInputText;
        Data.UserResponse = UserInputText;
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("OnOptionButtonClicked: Final Data - SelectedOption='%s', UserInputText='%s'"),
        *Data.SelectedOption, *Data.UserInputText);
    
    // 广播确认事件 - 与 Confirm 按钮相同的行为
    OnEmergencyConfirmed.Broadcast(Data);
    
    // 触发基类的确认回调，这会关闭窗口并清除紧急状态
    OnConfirmClicked.Broadcast();
}

void UMAEmergencyModal::UpdateOptionButtonStyles()
{
    // 更新所有选项按钮的样式 - 高亮选中的按钮
    for (int32 i = 0; i < OptionButtons.Num(); ++i)
    {
        UMAStyledButton* Button = OptionButtons[i];
        
        if (!Button)
        {
            continue;
        }
        
        if (i == SelectedOptionIndex)
        {
            // 选中状态 - 使用 Primary 样式（蓝色高亮）
            Button->SetButtonStyle(EMAButtonStyle::Primary);
        }
        else
        {
            // 未选中状态 - 使用 Secondary 样式（灰色）
            Button->SetButtonStyle(EMAButtonStyle::Secondary);
        }
        
        // 重新应用主题以更新颜色
        if (Theme)
        {
            Button->ApplyTheme(Theme);
        }
    }
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
    
    // 设置背景颜色并应用圆角效果
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(UserResponseContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
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
            UserResponseLabel->SetText(FText::FromString(TEXT("User Response")));
            
            FSlateFontInfo TitleFont = UserResponseLabel->GetFont();
            TitleFont.Size = 14;
            TitleFont.TypefaceFontName = FName("Bold");
            UserResponseLabel->SetFont(TitleFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            UserResponseLabel->SetColorAndOpacity(FSlateColor(TextColor));
            
            if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(UserResponseLabel->Slot))
            {
                TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
        }
        
        // 创建用户响应输入框 - 使用 SizeBox 确保最小高度
        USizeBox* InputSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InputSizeBox"));
        if (InputSizeBox)
        {
            InputSizeBox->SetMinDesiredHeight(120.0f);  // 设置最小高度为 120 像素
            
            // 创建圆角 Border 包装文本框
            UBorder* UserResponseBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UserResponseBorder"));
            if (UserResponseBorder)
            {
                UserResponseBorder->SetPadding(FMargin(10.0f, 8.0f));
                
                // 应用圆角效果 - 可编辑文本框使用主题输入框背景颜色
                FLinearColor ResponseBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
                MARoundedBorderUtils::ApplyRoundedCorners(UserResponseBorder, ResponseBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
                
                // 创建用户响应输入框
                UserResponseInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
                    UMultiLineEditableTextBox::StaticClass(), TEXT("UserResponseInput"));
                if (UserResponseInput)
                {
                    UserResponseInput->SetHintText(FText::FromString(TEXT("Enter your intervention response here...")));
                    UserResponseInput->SetIsReadOnly(false);
                    
                    // 设置样式：使用主题输入框文字颜色，字号 14，透明背景
                    FEditableTextBoxStyle ResponseStyle;
                    FLinearColor InputTextCol = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    FSlateColor InputTextColor = FSlateColor(InputTextCol);
                    ResponseStyle.SetForegroundColor(InputTextColor);
                    ResponseStyle.SetFocusedForegroundColor(InputTextColor);
                    FSlateFontInfo ResponseFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
                    ResponseStyle.SetFont(ResponseFont);
                    
                    // 设置透明背景
                    FSlateBrush TransparentBrush;
                    TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
                    ResponseStyle.SetBackgroundImageNormal(TransparentBrush);
                    ResponseStyle.SetBackgroundImageHovered(TransparentBrush);
                    ResponseStyle.SetBackgroundImageFocused(TransparentBrush);
                    ResponseStyle.SetBackgroundImageReadOnly(TransparentBrush);
                    
                    UserResponseInput->WidgetStyle = ResponseStyle;
                    
                    UserResponseBorder->AddChild(UserResponseInput);
                }
                
                InputSizeBox->AddChild(UserResponseBorder);
            }
            
            ResponseVBox->AddChild(InputSizeBox);
            
            if (UVerticalBoxSlot* InputSlot = Cast<UVerticalBoxSlot>(InputSizeBox->Slot))
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
    UE_LOG(LogMAEmergencyModal, Log, TEXT("HandleConfirm: Emergency event confirmed"));
    
    // 首先直接从输入框获取用户输入文本
    FString UserInputText;
    if (UserResponseInput)
    {
        UserInputText = UserResponseInput->GetText().ToString();
        UE_LOG(LogMAEmergencyModal, Log, TEXT("HandleConfirm: Direct read from UserResponseInput: '%s'"), *UserInputText);
    }
    else
    {
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("HandleConfirm: UserResponseInput is NULL!"));
    }
    
    // 获取当前数据（包含用户响应）
    FMAEmergencyEventData Data = GetEmergencyEventData();
    
    // 确保选中的选项被包含 - Requirements: 6.2, 6.3
    if (SelectedOptionIndex >= 0 && SelectedOptionIndex < EventData.AvailableOptions.Num())
    {
        Data.SelectedOption = EventData.AvailableOptions[SelectedOptionIndex];
    }
    else if (EventData.AvailableOptions.Num() > 0)
    {
        // 如果没有选择选项，使用第一个作为默认 - Requirements: 6.2
        Data.SelectedOption = EventData.AvailableOptions[0];
        UE_LOG(LogMAEmergencyModal, Log, TEXT("HandleConfirm: No option selected, using default: %s"), *Data.SelectedOption);
    }
    
    // 确保用户输入文本被包含（双重保险）- Requirements: 6.5, 7.5
    if (!UserInputText.IsEmpty())
    {
        Data.UserInputText = UserInputText;
        Data.UserResponse = UserInputText;
    }
    
    UE_LOG(LogMAEmergencyModal, Log, TEXT("HandleConfirm: Final Data - SelectedOption='%s', UserInputText='%s'"),
        *Data.SelectedOption, *Data.UserInputText);
    
    // 广播确认事件 - Requirements: 6.3
    OnEmergencyConfirmed.Broadcast(Data);
}

void UMAEmergencyModal::HandleReject()
{
    UE_LOG(LogMAEmergencyModal, Log, TEXT("Emergency event rejected"));
    
    // 广播拒绝事件 - Requirements: 6.4
    OnEmergencyRejected.Broadcast();
    
    // 重置选中状态
    SelectedOptionIndex = -1;
    EventData.SelectedOption.Empty();
    EventData.UserInputText.Empty();
}

void UMAEmergencyModal::HandleOptionButtonClicked()
{
    // 遍历所有按钮，找到被点击的那个
    // UMAStyledButton 使用 OnClicked 委托，我们需要检查哪个按钮触发了事件
    for (int32 i = 0; i < OptionButtons.Num(); ++i)
    {
        UMAStyledButton* Button = OptionButtons[i];
        if (Button && Button->IsHovered())
        {
            OnOptionButtonClicked(i);
            return;
        }
    }
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("HandleOptionButtonClicked: Could not determine which button was clicked"));
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
        Font.Size = 12;
        Font.TypefaceFontName = FName("Bold");
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
        Font.Size = 12;
        Font.TypefaceFontName = FName("Bold");
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

//=============================================================================
// 相机视图区域
//=============================================================================

UBorder* UMAEmergencyModal::CreateCameraViewArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    CameraViewContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CameraViewContainer"));
    if (!CameraViewContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色并应用圆角效果
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(CameraViewContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    CameraViewContainer->SetPadding(FMargin(8.0f));
    
    // 创建垂直布局
    UVerticalBox* CameraVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CameraVBox"));
    if (CameraVBox)
    {
        CameraViewContainer->AddChild(CameraVBox);
        
        // 创建标题
        UTextBlock* CameraTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CameraTitle"));
        if (CameraTitle)
        {
            CameraVBox->AddChild(CameraTitle);
            CameraTitle->SetText(FText::FromString(TEXT("Agent Camera View")));
            
            FSlateFontInfo TitleFont = CameraTitle->GetFont();
            TitleFont.Size = 14;
            TitleFont.TypefaceFontName = FName("Bold");
            CameraTitle->SetFont(TitleFont);
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            CameraTitle->SetColorAndOpacity(FSlateColor(TextColor));
            
            if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(CameraTitle->Slot))
            {
                TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            }
        }
        
        // 创建相机画面显示 - 使用 SizeBox 包装以确保尺寸一致
        USizeBox* CameraSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CameraSizeBox"));
        if (CameraSizeBox)
        {
            // 设置相机画面的固定尺寸，与容器匹配
            CameraSizeBox->SetWidthOverride(560.0f);
            CameraSizeBox->SetHeightOverride(420.0f);
            
            // 创建内部圆角边框包装相机画面
            UBorder* CameraImageBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CameraImageBorder"));
            if (CameraImageBorder)
            {
                // 应用圆角效果到相机画面边框 - 使用主题画布背景颜色
                FLinearColor CameraBorderColor = Theme ? Theme->CanvasBackgroundColor : FLinearColor::Black;
                MARoundedBorderUtils::ApplyRoundedCorners(CameraImageBorder, CameraBorderColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
                CameraImageBorder->SetPadding(FMargin(0.0f));
                
                // 创建相机画面 Image
                CameraFeedImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CameraFeedImage"));
                if (CameraFeedImage)
                {
                    CameraImageBorder->AddChild(CameraFeedImage);
                    
                    // 初始设置为黑色
                    CameraFeedImage->SetColorAndOpacity(FLinearColor::Black);
                    
                    UE_LOG(LogMAEmergencyModal, Log, TEXT("Created CameraFeedImage widget with SizeBox (560x420)"));
                }
                
                CameraSizeBox->AddChild(CameraImageBorder);
            }
            
            CameraVBox->AddChild(CameraSizeBox);
            
            if (UVerticalBoxSlot* SizeBoxSlot = Cast<UVerticalBoxSlot>(CameraSizeBox->Slot))
            {
                SizeBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                SizeBoxSlot->SetHorizontalAlignment(HAlign_Fill);
                SizeBoxSlot->SetVerticalAlignment(VAlign_Fill);
            }
        }
    }
    
    return CameraViewContainer;
}

void UMAEmergencyModal::SetCameraSource(UMACameraSensorComponent* Camera)
{
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("========== SetCameraSource START =========="));
    
    if (!Camera)
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("SetCameraSource: Camera is null!"));
        ClearCameraSource();
        return;
    }
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: Camera component valid, SensorName=%s"), *Camera->SensorName);
    
    // 清除之前的相机源（如果有，恢复其设置）
    if (CurrentCameraSource.IsValid() && CurrentCameraSource.Get() != Camera)
    {
        USceneCaptureComponent2D* OldCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (OldCapture)
        {
            OldCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource = Camera;
    
    // 获取相机的 SceneCaptureComponent
    USceneCaptureComponent2D* SceneCapture = Camera->GetSceneCaptureComponent();
    if (!SceneCapture)
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("SetCameraSource: SceneCaptureComponent is NULL!"));
        ClearCameraSource();
        return;
    }
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: SceneCaptureComponent valid"));
    
    // 检查 SceneCapture 是否已注册
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: SceneCapture IsRegistered=%d, IsActive=%d"),
        SceneCapture->IsRegistered(), SceneCapture->IsActive());
    
    // 启用每帧捕获以显示实时画面
    SceneCapture->bCaptureEveryFrame = true;
    SceneCapture->bAlwaysPersistRenderingState = true;
    
    // 获取相机的渲染目标
    UTextureRenderTarget2D* CameraRenderTarget = SceneCapture->TextureTarget;
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: SceneCapture=%p, TextureTarget=%p, CameraFeedImage=%p"),
        SceneCapture, CameraRenderTarget, CameraFeedImage);
    
    if (!CameraRenderTarget)
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("SetCameraSource: TextureTarget (RenderTarget) is NULL! Camera may not have called BeginPlay yet."));
        
        // 尝试获取 Camera 的 RenderTarget 成员
        UTextureRenderTarget2D* DirectRT = Camera->GetRenderTarget();
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: Direct RenderTarget from Camera=%p"), DirectRT);
        
        if (DirectRT)
        {
            CameraRenderTarget = DirectRT;
            // 手动设置到 SceneCapture
            SceneCapture->TextureTarget = DirectRT;
            UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: Manually set TextureTarget from Camera->RenderTarget"));
        }
    }
    
    if (!CameraFeedImage)
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("SetCameraSource: CameraFeedImage widget is NULL! Widget may not have been created."));
        return;
    }
    
    // 调试：检查 CameraFeedImage 的状态
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: CameraFeedImage IsVisible=%d, GetVisibility=%d"),
        CameraFeedImage->IsVisible(), (int32)CameraFeedImage->GetVisibility());
    
    if (CameraRenderTarget && CameraFeedImage)
    {
        // 检查渲染目标的状态
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: RenderTarget Size=%dx%d, Format=%d, IsValidLowLevel=%d"),
            CameraRenderTarget->SizeX, CameraRenderTarget->SizeY,
            (int32)CameraRenderTarget->GetFormat(),
            CameraRenderTarget->IsValidLowLevel());
        
        // 强制捕获一帧以确保渲染目标有内容
        SceneCapture->CaptureScene();
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: CaptureScene() called"));
        
        // 使用 Brush 设置渲染目标
        FSlateBrush Brush;
        Brush.SetResourceObject(CameraRenderTarget);
        Brush.ImageSize = FVector2D(CameraRenderTarget->SizeX, CameraRenderTarget->SizeY);
        
        CameraFeedImage->SetBrush(Brush);
        CameraFeedImage->SetColorAndOpacity(FLinearColor::White);
        
        // 验证 brush 设置
        FSlateBrush VerifyBrush = CameraFeedImage->GetBrush();
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: Brush ResourceObject=%p, ImageSize=(%f,%f)"),
            VerifyBrush.GetResourceObject(), VerifyBrush.ImageSize.X, VerifyBrush.ImageSize.Y);
        
        // 检查 RenderTarget 的资源状态
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: RenderTarget Resource=%p, RenderTargetFormat=%d"),
            CameraRenderTarget->GetResource(), (int32)CameraRenderTarget->RenderTargetFormat);
        
        UE_LOG(LogMAEmergencyModal, Warning, TEXT("SetCameraSource: SUCCESS - Camera connected (size: %dx%d)"), 
            CameraRenderTarget->SizeX, CameraRenderTarget->SizeY);
    }
    else
    {
        UE_LOG(LogMAEmergencyModal, Error, TEXT("SetCameraSource: FAILED - RT=%p, Image=%p"), 
            CameraRenderTarget, CameraFeedImage);
        ClearCameraSource();
    }
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("========== SetCameraSource END =========="));
}

void UMAEmergencyModal::ClearCameraSource()
{
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("========== ClearCameraSource called =========="));
    
    // 恢复之前相机的设置
    if (CurrentCameraSource.IsValid())
    {
        USceneCaptureComponent2D* SceneCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (SceneCapture)
        {
            // 恢复为不每帧捕获（节省性能）
            SceneCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource.Reset();
    
    // 设置为黑屏
    if (CameraFeedImage)
    {
        FSlateBrush BlackBrush;
        BlackBrush.DrawAs = ESlateBrushDrawType::Image;
        BlackBrush.TintColor = FSlateColor(FLinearColor::Black);
        CameraFeedImage->SetBrush(BlackBrush);
        CameraFeedImage->SetColorAndOpacity(FLinearColor::White);
    }
    
    UE_LOG(LogMAEmergencyModal, Warning, TEXT("========== ClearCameraSource END =========="));
}
