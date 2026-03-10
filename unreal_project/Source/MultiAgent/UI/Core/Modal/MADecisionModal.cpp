// MADecisionModal.cpp
// HITL 决策模态窗口实现

#include "MADecisionModal.h"
#include "../MAUITheme.h"
#include "../MARoundedBorderUtils.h"
#include "../../Components/MAStyledButton.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMADecisionModal, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMADecisionModal::UMADecisionModal(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , DescriptionContainer(nullptr)
    , DescriptionText(nullptr)
    , FeedbackContainer(nullptr)
    , FeedbackLabel(nullptr)
    , FeedbackInput(nullptr)
{
    // 设置模态类型
    SetModalType(EMAModalType::Decision);

    // 决策模态尺寸较小 (约 700x450)
    ModalWidth = 700.0f;
    ModalHeight = 450.0f;

    // 按钮文本: Yes / No
    ConfirmButtonText = FText::FromString(TEXT("Yes"));
    RejectButtonText = FText::FromString(TEXT("No"));

    // 不显示编辑按钮 - 决策模态直接可操作
    bShowEditButton = false;
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMADecisionModal::NativeConstruct()
{
    Super::NativeConstruct();

    // 决策模态直接进入编辑模式
    SetEditMode(true);

    // 绑定基类按钮事件
    OnConfirmClicked.AddDynamic(this, &UMADecisionModal::HandleConfirm);
    OnRejectClicked.AddDynamic(this, &UMADecisionModal::HandleReject);

    UE_LOG(LogMADecisionModal, Log, TEXT("MADecisionModal constructed"));
}

void UMADecisionModal::NativeDestruct()
{
    OnConfirmClicked.RemoveDynamic(this, &UMADecisionModal::HandleConfirm);
    OnRejectClicked.RemoveDynamic(this, &UMADecisionModal::HandleReject);

    Super::NativeDestruct();
}

//=============================================================================
// UMABaseModalWidget 重写
//=============================================================================

FText UMADecisionModal::GetModalTitleText() const
{
    return FText::FromString(TEXT("Decision Required"));
}

void UMADecisionModal::BuildContentArea(UVerticalBox* InContentContainer)
{
    if (!InContentContainer || !WidgetTree)
    {
        UE_LOG(LogMADecisionModal, Error, TEXT("BuildContentArea: Invalid container or WidgetTree"));
        return;
    }

    UE_LOG(LogMADecisionModal, Log, TEXT("Building decision modal content area..."));

    // 1. 描述区域
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

    // 2. 用户反馈输入区域
    UBorder* FeedbackArea = CreateUserFeedbackArea();
    if (FeedbackArea)
    {
        InContentContainer->AddChild(FeedbackArea);

        if (UVerticalBoxSlot* FeedbackSlot = Cast<UVerticalBoxSlot>(FeedbackArea->Slot))
        {
            FeedbackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    UE_LOG(LogMADecisionModal, Log, TEXT("Decision modal content area built successfully"));
}

void UMADecisionModal::OnThemeApplied()
{
    // 应用主题到描述容器
    if (DescriptionContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(DescriptionContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }

    // 应用主题到反馈容器
    if (FeedbackContainer && Theme)
    {
        FLinearColor BgColor = Theme->SecondaryColor;
        BgColor.A = 0.3f;
        MARoundedBorderUtils::ApplyRoundedCorners(FeedbackContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }

    // 应用主题到文本
    if (Theme)
    {
        FSlateColor TextColor = FSlateColor(Theme->TextColor);

        if (DescriptionText) DescriptionText->SetColorAndOpacity(TextColor);
        if (FeedbackLabel) FeedbackLabel->SetColorAndOpacity(TextColor);
    }
}

//=============================================================================
// 公共接口
//=============================================================================

void UMADecisionModal::LoadDecisionData(const FString& Description, const FString& ContextJson)
{
    UE_LOG(LogMADecisionModal, Log, TEXT("Loading decision data: Description='%s'"), *Description);

    CurrentDescription = Description;
    CurrentContextJson = ContextJson;

    // 更新描述文本
    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(Description));
    }

    // 清空之前的用户反馈
    if (FeedbackInput)
    {
        FeedbackInput->SetText(FText::GetEmpty());
    }
}

FString UMADecisionModal::GetUserFeedback() const
{
    if (FeedbackInput)
    {
        return FeedbackInput->GetText().ToString();
    }
    return FString();
}

//=============================================================================
// 内部方法 - UI 构建
//=============================================================================

UBorder* UMADecisionModal::CreateDescriptionArea()
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
    DescriptionContainer->SetPadding(FMargin(16.0f));

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
            DescTitle->SetText(FText::FromString(TEXT("Execution Result")));

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
            DescriptionText->SetText(FText::FromString(TEXT("Waiting for decision data...")));
            DescriptionText->SetAutoWrapText(true);

            FSlateFontInfo DescFont = DescriptionText->GetFont();
            DescFont.Size = 13;
            DescriptionText->SetFont(DescFont);

            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
            DescriptionText->SetColorAndOpacity(FSlateColor(TextColor));
        }
    }

    return DescriptionContainer;
}

UBorder* UMADecisionModal::CreateUserFeedbackArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    // 创建容器边框
    FeedbackContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FeedbackContainer"));
    if (!FeedbackContainer)
    {
        return nullptr;
    }

    // 设置背景颜色并应用圆角效果
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.3f);
    BgColor.A = 0.3f;
    MARoundedBorderUtils::ApplyRoundedCorners(FeedbackContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    FeedbackContainer->SetPadding(FMargin(16.0f));

    // 创建垂直布局
    UVerticalBox* FeedbackVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FeedbackVBox"));
    if (FeedbackVBox)
    {
        FeedbackContainer->AddChild(FeedbackVBox);

        // 创建标签
        FeedbackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackLabel"));
        if (FeedbackLabel)
        {
            FeedbackVBox->AddChild(FeedbackLabel);
            FeedbackLabel->SetText(FText::FromString(TEXT("Feedback (optional)")));

            FSlateFontInfo LabelFont = FeedbackLabel->GetFont();
            LabelFont.Size = 12;
            LabelFont.TypefaceFontName = FName("Bold");
            FeedbackLabel->SetFont(LabelFont);

            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            FeedbackLabel->SetColorAndOpacity(FSlateColor(TextColor));

            if (UVerticalBoxSlot* LabelSlot = Cast<UVerticalBoxSlot>(FeedbackLabel->Slot))
            {
                LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
            }
        }

        // 创建多行输入框
        FeedbackInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
            UMultiLineEditableTextBox::StaticClass(), TEXT("FeedbackInput"));
        if (FeedbackInput)
        {
            FeedbackVBox->AddChild(FeedbackInput);
            FeedbackInput->SetHintText(FText::FromString(TEXT("Enter feedback for replanning...")));

            // 设置输入文本颜色为黑色（白色背景上更清晰）
            FEditableTextBoxStyle InputStyle = FeedbackInput->WidgetStyle;
            FSlateColor BlackColor = FSlateColor(FLinearColor::Black);
            InputStyle.SetForegroundColor(BlackColor);
            InputStyle.SetFocusedForegroundColor(BlackColor);
            FeedbackInput->WidgetStyle = InputStyle;

            if (UVerticalBoxSlot* InputSlot = Cast<UVerticalBoxSlot>(FeedbackInput->Slot))
            {
                InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }
        }
    }

    return FeedbackContainer;
}

//=============================================================================
// 按钮回调
//=============================================================================

void UMADecisionModal::HandleConfirm()
{
    FString UserFeedback = GetUserFeedback();

    UE_LOG(LogMADecisionModal, Log, TEXT("Decision confirmed (Yes/end_task), feedback='%s'"), *UserFeedback);

    // 广播 end_task 决策
    OnDecisionConfirmed.Broadcast(TEXT("end_task"), UserFeedback);
}

void UMADecisionModal::HandleReject()
{
    FString UserFeedback = GetUserFeedback();

    UE_LOG(LogMADecisionModal, Log, TEXT("Decision rejected (No/continue_task), feedback='%s'"), *UserFeedback);

    // 广播 continue_task 决策
    OnDecisionRejected.Broadcast(TEXT("continue_task"), UserFeedback);
}
