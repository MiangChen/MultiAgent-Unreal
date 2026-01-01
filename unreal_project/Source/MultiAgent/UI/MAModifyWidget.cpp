// MAModifyWidget.cpp
// Modify 模式修改面板 Widget - 纯 C++ 实现
// 支持单选和多选模式
// Requirements: 2.4, 3.1, 3.2, 3.3, 3.4, 3.5, 4.1, 4.2, 4.3, 5.1, 5.2, 5.3, 5.4, 8.1, 8.2, 8.3, 9.3, 9.4

#include "MAModifyWidget.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "../Core/Manager/MASceneGraphManager.h"
#include "../Utils/MAGeometryUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAModifyWidget, Log, All);

// 提示文字常量
// Requirements: 10.3 - 更新为新格式提示
static const FString DefaultHintText = TEXT("输入格式: cate:building/trans_facility/prop,type:xxx");
static const FString MultiSelectHintText = TEXT("输入格式: cate:xxx,type:xxx,shape:polygon/linestring");

UMAModifyWidget::UMAModifyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAModifyWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAModifyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定确认按钮事件
    if (ConfirmButton && !ConfirmButton->OnClicked.IsAlreadyBound(this, &UMAModifyWidget::OnConfirmButtonClicked))
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UMAModifyWidget::OnConfirmButtonClicked);
        UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton event bound"));
    }
    
    // 初始化为未选中状态
    ClearSelection();
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("MAModifyWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMAModifyWidget::RebuildWidget()
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    
    // 构建 UI
    if (!WidgetTree->RootWidget)
    {
        BuildUI();
    }
    
    return Super::RebuildWidget();
}

void UMAModifyWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border - 位于屏幕右侧
    // Requirements: 3.5 - 修改面板宽度约为屏幕宽度的 20%
    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.95f));
    BackgroundBorder->SetPadding(FMargin(15.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    // 设置为右侧锚点，宽度约 20% 屏幕宽度 (约 380 像素)
    BorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
    BorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(-20, 80));  // 右边距 20，顶部距离 80 (避开模式指示器)
    BorderSlot->SetSize(FVector2D(350, 420));  // 增加高度以容纳 JSON 预览框
    BorderSlot->SetAutoSize(false);

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // 标题 - Requirements: 3.1
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("修改面板")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f)));  // 橙色，与 Modify 模式颜色一致
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 提示文本 - Requirements: 4.3
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(DefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // JSON 预览文本框 - 只读，显示 Actor 对应的 JSON 片段
    UBorder* JsonPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonPreviewBorder"));
    JsonPreviewBorder->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));  // 深灰色背景
    JsonPreviewBorder->SetPadding(FMargin(8.0f));
    
    UScrollBox* JsonScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JsonScrollBox"));
    JsonPreviewBorder->AddChild(JsonScrollBox);
    
    JsonPreviewText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonPreviewText"));
    JsonPreviewText->SetText(FText::FromString(TEXT("选中 Actor 后显示 JSON 预览")));
    FSlateFontInfo JsonFont = JsonPreviewText->GetFont();
    JsonFont.Size = 10;
    JsonPreviewText->SetFont(JsonFont);
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 0.7f)));  // 浅绿色
    JsonPreviewText->SetAutoWrapText(true);
    JsonScrollBox->AddChild(JsonPreviewText);
    
    USizeBox* JsonPreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonPreviewSizeBox"));
    JsonPreviewSizeBox->SetMinDesiredHeight(80.0f);
    JsonPreviewSizeBox->SetMaxDesiredHeight(100.0f);
    JsonPreviewSizeBox->AddChild(JsonPreviewBorder);
    
    UVerticalBoxSlot* JsonPreviewSlot = MainVBox->AddChildToVerticalBox(JsonPreviewSizeBox);
    JsonPreviewSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 多行文本框 - Requirements: 3.2, 4.4
    LabelTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("LabelTextBox"));
    // Requirements: 2.5, 10.3 - 显示新格式输入提示
    LabelTextBox->SetHintText(FText::FromString(DefaultHintText));
    
    // 设置文字颜色为严格的黑色 - 通过 WidgetStyle 属性
    FEditableTextBoxStyle TextBoxStyle;
    TextBoxStyle.SetForegroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));  // 严格黑色
    LabelTextBox->WidgetStyle = TextBoxStyle;
    
    // 使用 SizeBox 设置最小高度
    USizeBox* TextBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TextBoxSizeBox"));
    TextBoxSizeBox->SetMinDesiredHeight(150.0f);
    TextBoxSizeBox->AddChild(LabelTextBox);
    
    UVerticalBoxSlot* TextBoxSlot = MainVBox->AddChildToVerticalBox(TextBoxSizeBox);
    TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TextBoxSlot->SetPadding(FMargin(0, 0, 0, 15));

    // 确认按钮 - Requirements: 3.3
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("  确认  ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ButtonFont = ButtonText->GetFont();
    ButtonFont.Size = 14;
    ButtonText->SetFont(ButtonFont);
    ConfirmButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = MainVBox->AddChildToVerticalBox(ConfirmButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Center);

    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}


//=========================================================================
// GenerateActorLabel - 占位符实现
// Requirements: 4.2
//=========================================================================

FString UMAModifyWidget::GenerateActorLabel(AActor* Actor) const
{
    if (!Actor)
    {
        return FString();
    }
    
    // 获取 Actor 名称
    FString ActorName = Actor->GetName();
    
    // 占位符标签 - 未来从 IMALabelProvider 获取
    FString Label = TEXT("[placeholder]");
    
    // 格式: "Actor: [ActorName]\nLabel: [placeholder]"
    // Requirements: 4.2
    return FString::Printf(TEXT("Actor: %s\nLabel: %s"), *ActorName, *Label);
}


//=========================================================================
// SetSelectedActor - 设置选中的 Actor (单选模式)
// Requirements: 4.1, 9.1, 9.2
//=========================================================================

void UMAModifyWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        // 如果传入 nullptr，清除选择
        ClearSelection();
        return;
    }
    
    // 单选模式：清空数组并添加单个 Actor
    SelectedActors.Empty();
    SelectedActors.Add(Actor);
    
    // 清空文本框，让用户输入新的标签
    // Requirements: 2.5, 10.3 - 显示新格式输入提示
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
        LabelTextBox->SetHintText(FText::FromString(DefaultHintText));
    }
    
    // 更新提示文本
    if (HintText)
    {
        HintText->SetText(FText::FromString(FString::Printf(TEXT("已选中: %s"), *Actor->GetName())));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));  // 绿色表示已选中
    }
    
    // 更新 JSON 预览
    UpdateJsonPreview(Actor);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetSelectedActor: %s"), *Actor->GetName());
}

//=========================================================================
// SetSelectedActors - 设置多个选中的 Actor (多选模式)
// Requirements: 2.4
//=========================================================================

void UMAModifyWidget::SetSelectedActors(const TArray<AActor*>& Actors)
{
    // 过滤掉 nullptr
    SelectedActors.Empty();
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            SelectedActors.Add(Actor);
        }
    }
    
    if (SelectedActors.Num() == 0)
    {
        ClearSelection();
        return;
    }
    
    // 更新提示文本显示选择数量
    // Requirements: 2.4 - 更新 HintText 显示选择数量
    if (HintText)
    {
        if (SelectedActors.Num() == 1)
        {
            HintText->SetText(FText::FromString(FString::Printf(TEXT("已选中: %s"), *SelectedActors[0]->GetName())));
        }
        else
        {
            HintText->SetText(FText::FromString(FString::Printf(TEXT("已选中: %d 个 Actor"), SelectedActors.Num())));
        }
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));  // 绿色表示已选中
    }
    
    // 清空文本框，让用户输入新的标签
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
        // Requirements: 10.3 - 多选模式提示不同的输入格式
        if (SelectedActors.Num() > 1)
        {
            LabelTextBox->SetHintText(FText::FromString(MultiSelectHintText));
        }
        else
        {
            LabelTextBox->SetHintText(FText::FromString(DefaultHintText));
        }
    }
    
    // 更新 JSON 预览
    if (SelectedActors.Num() > 1)
    {
        UpdateJsonPreviewMultiSelect(SelectedActors);
    }
    else
    {
        UpdateJsonPreview(SelectedActors[0]);
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetSelectedActors: %d actors selected"), SelectedActors.Num());
}

//=========================================================================
// ClearSelection - 清除选中状态
// Requirements: 4.3
//=========================================================================

void UMAModifyWidget::ClearSelection()
{
    SelectedActors.Empty();
    
    // 清空文本框
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
    }
    
    // 显示提示文字
    if (HintText)
    {
        HintText->SetText(FText::FromString(DefaultHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));  // 灰色
    }
    
    // 清空 JSON 预览
    if (JsonPreviewText)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("选中 Actor 后显示 JSON 预览")));
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

//=========================================================================
// GetLabelText / SetLabelText - 文本框内容访问
//=========================================================================

FString UMAModifyWidget::GetLabelText() const
{
    if (LabelTextBox)
    {
        return LabelTextBox->GetText().ToString();
    }
    return FString();
}

void UMAModifyWidget::SetLabelText(const FString& Text)
{
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::FromString(Text));
    }
}


//=========================================================================
// OnConfirmButtonClicked - 确认按钮点击处理
// Requirements: 5.1, 5.2, 5.3, 5.4, 8.1, 8.2, 8.3
//=========================================================================

void UMAModifyWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton clicked"));
    
    // 检查是否有选中的 Actor
    if (SelectedActors.Num() == 0)
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: No Actor selected, ignoring"));
        return;
    }
    
    // 获取文本框内容
    FString LabelContent = GetLabelText();
    
    // 判断是单选还是多选模式
    if (IsMultiSelectMode())
    {
        // 多选模式
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Multi-select mode, %d actors, LabelText=%s"), 
            SelectedActors.Num(), *LabelContent);
        
        // 解析输入
        FMAAnnotationInput ParsedInput;
        FString ParseError;
        if (!ParseAnnotationInput(LabelContent, ParsedInput, ParseError))
        {
            UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: Parse failed - %s"), *ParseError);
            // 解析失败，保留输入，由 MAHUD 处理错误显示
            OnModifyConfirmed.Broadcast(SelectedActors[0], LabelContent);
            return;
        }
        
        // 检查多选模式是否指定了 shape
        if (!ParsedInput.IsMultiSelect())
        {
            UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: Multi-select requires shape:polygon or shape:linestring"));
            // 广播单选委托，让 MAHUD 处理错误
            OnModifyConfirmed.Broadcast(SelectedActors[0], LabelContent);
            return;
        }
        
        // 生成 JSON
        FString GeneratedJson = GenerateSceneGraphNode(ParsedInput, SelectedActors);
        
        // 广播多选确认委托
        OnMultiSelectModifyConfirmed.Broadcast(SelectedActors, LabelContent, GeneratedJson);
    }
    else
    {
        // 单选模式 - 保持原有行为
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Single-select mode, Actor=%s, LabelText=%s"), 
            *SelectedActors[0]->GetName(), *LabelContent);
        
        // 广播单选确认委托
        OnModifyConfirmed.Broadcast(SelectedActors[0], LabelContent);
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Modification confirmed, waiting for MAHUD to handle state"));
}

//=========================================================================
// UpdateJsonPreview - 更新 JSON 预览文本
// 显示选中 Actor 对应的 JSON 片段
// Requirements: 2.3, 2.5, 2.6
//
// 改进逻辑:
// 1. 获取选中 Actor 的 GUID
// 2. 同时查找 GuidArray 匹配 (polygon/linestring) 和 guid 字段匹配 (point)
// 3. 如果找到多个匹配节点，显示所有节点的 JSON
// 4. 如果未找到，保持现有的单 Actor 预览行为
//=========================================================================

void UMAModifyWidget::UpdateJsonPreview(AActor* Actor)
{
    if (!JsonPreviewText)
    {
        return;
    }
    
    if (!Actor)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("选中 Actor 后显示 JSON 预览")));
        return;
    }
    
    // 获取 SceneGraphManager
    UWorld* World = GetWorld();
    if (!World)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("无法获取 World")));
        return;
    }
    
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("无法获取 GameInstance")));
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("SceneGraphManager 未找到")));
        return;
    }
    
    // 获取 Actor 的 GUID
    FString ActorGuid = Actor->GetActorGuid().ToString();
    
    // 收集所有匹配的节点 (包括 GuidArray 匹配和 guid 字段匹配)
    TArray<FMASceneGraphNode> AllMatchingNodes;
    
    // 1. 通过 GuidArray 查找 (polygon/linestring 类型)
    TArray<FMASceneGraphNode> GuidArrayMatches = SceneGraphManager->FindNodesByGuid(ActorGuid);
    AllMatchingNodes.Append(GuidArrayMatches);
    
    // 2. 通过单个 guid 字段查找 (point 类型)
    TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Guid == ActorGuid)
        {
            // 检查是否已经在 AllMatchingNodes 中 (避免重复)
            bool bAlreadyAdded = false;
            for (const FMASceneGraphNode& ExistingNode : AllMatchingNodes)
            {
                if (ExistingNode.Id == Node.Id)
                {
                    bAlreadyAdded = true;
                    break;
                }
            }
            if (!bAlreadyAdded)
            {
                AllMatchingNodes.Add(Node);
            }
        }
    }
    
    if (AllMatchingNodes.Num() > 0)
    {
        // 找到匹配节点，显示所有节点的 JSON
        FString CombinedPreview;
        
        // 如果有多个节点，添加总数提示
        if (AllMatchingNodes.Num() > 1)
        {
            CombinedPreview = FString::Printf(TEXT("此 Actor 属于 %d 个节点:\n"), AllMatchingNodes.Num());
            CombinedPreview += TEXT("━━━━━━━━━━━━━━━━━━━━\n\n");
        }
        
        // 显示每个节点的信息
        for (int32 i = 0; i < AllMatchingNodes.Num(); ++i)
        {
            const FMASceneGraphNode& Node = AllMatchingNodes[i];
            
            // 添加节点分隔符 (如果不是第一个)
            if (i > 0)
            {
                CombinedPreview += TEXT("\n────────────────────\n\n");
            }
            
            // 格式化并添加节点信息
            CombinedPreview += FormatJsonPreviewWithHighlight(Node, ActorGuid);
        }
        
        JsonPreviewText->SetText(FText::FromString(CombinedPreview));
        JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 0.7f)));  // 浅绿色
        
        UE_LOG(LogMAModifyWidget, Log, TEXT("UpdateJsonPreview: Found %d nodes for Actor GUID %s"), 
            AllMatchingNodes.Num(), *ActorGuid);
        return;
    }
    
    // 未找到任何匹配的节点，显示默认的单 Actor 预览
    FVector Location = Actor->GetActorLocation();
    FString PreviewText = FString::Printf(
        TEXT("Actor: %s\nGUID: %s\nLocation: [%.0f, %.0f, %.0f]\n\n(尚未标注 - 不属于任何节点)"),
        *Actor->GetName(),
        *ActorGuid,
        Location.X,
        Location.Y,
        Location.Z
    );
    JsonPreviewText->SetText(FText::FromString(PreviewText));
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.7f, 0.5f)));  // 橙色表示未标注
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("UpdateJsonPreview: No node found for Actor GUID %s"), *ActorGuid);
}

//=========================================================================
// UpdateJsonPreviewMultiSelect - 更新 JSON 预览文本 (多选模式)
// Requirements: 8.3
//=========================================================================

void UMAModifyWidget::UpdateJsonPreviewMultiSelect(const TArray<AActor*>& Actors)
{
    if (!JsonPreviewText)
    {
        return;
    }
    
    if (Actors.Num() == 0)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("选中 Actor 后显示 JSON 预览")));
        return;
    }
    
    // 构建多选预览信息
    FString PreviewText = FString::Printf(TEXT("多选模式: %d 个 Actor\n\n"), Actors.Num());
    
    // 列出所有选中的 Actor
    PreviewText += TEXT("选中的 Actor:\n");
    for (int32 i = 0; i < FMath::Min(Actors.Num(), 5); ++i)
    {
        if (Actors[i])
        {
            FString Guid = Actors[i]->GetActorGuid().ToString();
            PreviewText += FString::Printf(TEXT("  %d. %s\n     GUID: %s\n"), 
                i + 1, *Actors[i]->GetName(), *Guid.Left(20));
        }
    }
    
    if (Actors.Num() > 5)
    {
        PreviewText += FString::Printf(TEXT("  ... 还有 %d 个\n"), Actors.Num() - 5);
    }
    
    PreviewText += TEXT("\n输入 shape:polygon 或 shape:linestring");
    
    JsonPreviewText->SetText(FText::FromString(PreviewText));
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.7f, 0.9f)));  // 浅蓝色表示多选
}

//=========================================================================
// FormatJsonPreviewWithHighlight - 格式化 JSON 预览并高亮显示选中 Actor 的 GUID
// Requirements: 3.1, 3.2, 3.3, 3.4
//
// 功能:
// - 添加节点类型标题 (Polygon/LineString/Point)
// - 高亮显示选中 Actor 的 GUID (使用 >>> <<< 标记)
// - 使用缩进格式化 JSON
//=========================================================================

FString UMAModifyWidget::FormatJsonPreviewWithHighlight(const FMASceneGraphNode& Node, const FString& ActorGuid) const
{
    FString Result;
    
    // Requirements: 3.4 - 添加节点类型标题
    FString TypeTitle;
    if (Node.ShapeType == TEXT("polygon"))
    {
        TypeTitle = TEXT("=== Polygon 节点 ===");
    }
    else if (Node.ShapeType == TEXT("linestring"))
    {
        TypeTitle = TEXT("=== LineString 节点 ===");
    }
    else
    {
        TypeTitle = TEXT("=== Point 节点 ===");
    }
    Result += TypeTitle + TEXT("\n\n");
    
    // Requirements: 3.1, 3.2 - 格式化 JSON 显示
    // 使用 RawJson 字段，但进行简化显示以适应预览框
    
    // 基本信息
    Result += FString::Printf(TEXT("id: %s\n"), *Node.Id);
    Result += FString::Printf(TEXT("type: %s\n"), *Node.Type);
    Result += FString::Printf(TEXT("label: %s\n"), *Node.Label);
    
    // 显示中心点
    Result += FString::Printf(TEXT("center: [%.0f, %.0f, %.0f]\n"), 
        Node.Center.X, Node.Center.Y, Node.Center.Z);
    
    // Requirements: 3.3 - 高亮显示选中 Actor 的 GUID
    if (Node.GuidArray.Num() > 0)
    {
        Result += TEXT("\nGuid 数组:\n");
        for (int32 i = 0; i < Node.GuidArray.Num(); ++i)
        {
            const FString& Guid = Node.GuidArray[i];
            if (Guid == ActorGuid)
            {
                // 高亮显示选中的 GUID
                Result += FString::Printf(TEXT("  [%d] >>> %s <<< (选中)\n"), i, *Guid);
            }
            else
            {
                Result += FString::Printf(TEXT("  [%d] %s\n"), i, *Guid);
            }
        }
    }
    else if (!Node.Guid.IsEmpty())
    {
        // Point 类型节点，显示单个 guid
        if (Node.Guid == ActorGuid)
        {
            Result += FString::Printf(TEXT("\nguid: >>> %s <<< (选中)\n"), *Node.Guid);
        }
        else
        {
            Result += FString::Printf(TEXT("\nguid: %s\n"), *Node.Guid);
        }
    }
    
    return Result;
}

//=========================================================================
// ParseAnnotationInput - 解析标注输入字符串
// 支持向后兼容：检测输入格式，新格式调用 ParseAnnotationInputV2
// Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 10.1, 10.2
//=========================================================================

bool UMAModifyWidget::ParseAnnotationInput(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError)
{
    OutResult.Reset();
    OutError.Empty();
    
    // 检查空输入
    if (Input.IsEmpty())
    {
        OutError = TEXT("输入不能为空");
        return false;
    }
    
    // 检测输入格式：新格式包含 "cate:" 或 "category:"
    // Requirements: 10.1, 10.2 - 向后兼容性
    FString InputLower = Input.ToLower();
    if (InputLower.Contains(TEXT("cate:")) || InputLower.Contains(TEXT("category:")))
    {
        // 新格式：调用 ParseAnnotationInputV2
        UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: Detected new format (cate:xxx), delegating to ParseAnnotationInputV2"));
        return ParseAnnotationInputV2(Input, OutResult, OutError);
    }
    
    // 旧格式：保持原有逻辑
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: Using legacy format (id:xxx,type:xxx)"));
    
    // 解析 "key:value, key:value" 格式
    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);
    
    for (const FString& Pair : Pairs)
    {
        FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }
        
        // 分割 key:value
        FString Key, Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("无效的键值对格式: %s (应为 key:value)"), *TrimmedPair);
            return false;
        }
        
        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();
        
        if (Key.IsEmpty())
        {
            OutError = TEXT("键名不能为空");
            return false;
        }
        
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("键 '%s' 的值不能为空"), *Key);
            return false;
        }
        
        // 解析已知字段
        if (Key == TEXT("id"))
        {
            OutResult.Id = Value;
        }
        else if (Key == TEXT("type"))
        {
            OutResult.Type = Value;
        }
        else if (Key == TEXT("shape"))
        {
            // 验证 shape 值
            FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") && 
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle"))
            {
                OutError = FString::Printf(TEXT("无效的 shape 值: %s (应为 polygon, linestring, point 或 rectangle)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
        }
        else
        {
            // 存储为额外属性
            OutResult.Properties.Add(Key, Value);
        }
    }
    
    // 验证必需字段 (旧格式需要 id)
    if (OutResult.Id.IsEmpty())
    {
        OutError = TEXT("缺少必需字段: id");
        return false;
    }
    
    if (OutResult.Type.IsEmpty())
    {
        OutError = TEXT("缺少必需字段: type");
        return false;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: %s"), *OutResult.ToString());
    return true;
}

//=========================================================================
// GenerateSceneGraphNode - 生成场景图节点 JSON
// Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 5.1, 5.2, 5.3, 5.4, 5.5, 6.1, 6.2, 7.1, 7.2, 8.1, 8.2
//=========================================================================

FString UMAModifyWidget::GenerateSceneGraphNode(const FMAAnnotationInput& Input, const TArray<AActor*>& Actors)
{
    if (Actors.Num() == 0)
    {
        return TEXT("{}");
    }
    
    // 创建 JSON 对象
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    
    // 基本字段
    RootObject->SetStringField(TEXT("id"), Input.Id);
    RootObject->SetNumberField(TEXT("count"), Actors.Num());
    
    // GUID 数组
    TArray<TSharedPtr<FJsonValue>> GuidArray;
    TArray<FString> Guids = CollectActorGuids(Actors);
    for (const FString& Guid : Guids)
    {
        GuidArray.Add(MakeShareable(new FJsonValueString(Guid)));
    }
    RootObject->SetArrayField(TEXT("Guid"), GuidArray);
    
    // properties 对象
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("type"), Input.Type);
    
    // 生成标签
    FString Label = FString::Printf(TEXT("%s-%s"), *Input.Type, *Input.Id);
    // 首字母大写
    if (Label.Len() > 0)
    {
        Label[0] = FChar::ToUpper(Label[0]);
    }
    PropertiesObject->SetStringField(TEXT("label"), Label);
    
    // Requirements: 6.1, 6.2, 7.1, 7.2, 8.1, 8.2 - 集成默认字段
    // 如果有 Category，先添加默认字段
    if (Input.HasCategory())
    {
        TMap<FString, FString> DefaultProps = GetDefaultPropertiesForCategory(Input.Category);
        for (const auto& DefaultProp : DefaultProps)
        {
            // 检查用户是否已经在 Input.Properties 中指定了该字段
            // 如果没有，则使用默认值
            if (!Input.Properties.Contains(DefaultProp.Key))
            {
                // 处理布尔值字段
                if (DefaultProp.Value == TEXT("true") || DefaultProp.Value == TEXT("false"))
                {
                    PropertiesObject->SetBoolField(DefaultProp.Key, DefaultProp.Value == TEXT("true"));
                }
                else
                {
                    PropertiesObject->SetStringField(DefaultProp.Key, DefaultProp.Value);
                }
            }
        }
        UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNode: Added %d default properties for category %s"), 
            DefaultProps.Num(), *Input.GetCategoryString());
    }
    
    // 添加用户指定的额外属性（用户输入优先，会覆盖默认值）
    for (const auto& Prop : Input.Properties)
    {
        // 处理布尔值字段
        if (Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || 
            Prop.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
        {
            PropertiesObject->SetBoolField(Prop.Key, Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
        }
        else
        {
            PropertiesObject->SetStringField(Prop.Key, Prop.Value);
        }
    }
    RootObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    // shape 对象
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    
    if (Input.IsPolygon())
    {
        // Polygon 类型 - 计算凸包
        ShapeObject->SetStringField(TEXT("type"), TEXT("polygon"));
        
        TArray<FVector2D> Vertices = ComputeConvexHull(Actors);
        TArray<TSharedPtr<FJsonValue>> VerticesArray;
        for (const FVector2D& Vertex : Vertices)
        {
            // 三维坐标 [x, y, z]，z 用 0 填充
            TArray<TSharedPtr<FJsonValue>> PointArray;
            PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.X)));
            PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Y)));
            PointArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
            VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
        }
        ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
    }
    else if (Input.IsLineString())
    {
        // LineString 类型 - 计算点序列
        ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
        
        TArray<FVector2D> Points = ComputeLineString(Actors);
        TArray<TSharedPtr<FJsonValue>> PointsArray;
        for (const FVector2D& Point : Points)
        {
            // 三维坐标 [x, y, z]，z 用 0 填充
            TArray<TSharedPtr<FJsonValue>> PointArray;
            PointArray.Add(MakeShareable(new FJsonValueNumber(Point.X)));
            PointArray.Add(MakeShareable(new FJsonValueNumber(Point.Y)));
            PointArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
            PointsArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
        }
        ShapeObject->SetArrayField(TEXT("points"), PointsArray);
    }
    else
    {
        // 默认为 Point 类型 - 使用第一个 Actor 的中心
        ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
        
        FVector Center = Actors[0]->GetActorLocation();
        // 三维坐标 [x, y, z]
        TArray<TSharedPtr<FJsonValue>> CenterArray;
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
        ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    }
    
    RootObject->SetObjectField(TEXT("shape"), ShapeObject);
    
    // 序列化为 JSON 字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNode: Generated JSON for %d actors"), Actors.Num());
    return OutputString;
}

//=========================================================================
// ComputeConvexHull - 计算凸包顶点
// Requirements: 6.1, 6.2, 6.3
//=========================================================================

TArray<FVector2D> UMAModifyWidget::ComputeConvexHull(const TArray<AActor*>& Actors)
{
    // 收集所有边界框角点
    TArray<FVector2D> AllCorners = FMAGeometryUtils::CollectBoundingBoxCorners(Actors);
    
    // 计算凸包
    return FMAGeometryUtils::ComputeConvexHull2D(AllCorners);
}

//=========================================================================
// ComputeLineString - 计算线串端点
// Requirements: 7.1, 7.2, 7.3
// 
// 算法：找到点集的主方向（线段走向），返回首尾两个端点
// 1. 收集所有 Actor 的中心点
// 2. 计算点集的主方向（使用 PCA 或最远点对）
// 3. 将所有点投影到主方向上
// 4. 返回投影值最小和最大的两个点
//=========================================================================

TArray<FVector2D> UMAModifyWidget::ComputeLineString(const TArray<AActor*>& Actors)
{
    TArray<FVector2D> Result;
    
    // 收集所有 Actor 的中心点
    TArray<FVector2D> CenterPoints;
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            FVector Location = Actor->GetActorLocation();
            CenterPoints.Add(FVector2D(Location.X, Location.Y));
        }
    }
    
    // 处理退化情况
    if (CenterPoints.Num() == 0)
    {
        return Result;
    }
    
    if (CenterPoints.Num() == 1)
    {
        // 只有一个点，返回两个相同的点
        Result.Add(CenterPoints[0]);
        Result.Add(CenterPoints[0]);
        return Result;
    }
    
    if (CenterPoints.Num() == 2)
    {
        // 两个点，直接返回
        Result.Add(CenterPoints[0]);
        Result.Add(CenterPoints[1]);
        return Result;
    }
    
    // 计算点集的质心
    FVector2D Centroid(0.0f, 0.0f);
    for (const FVector2D& Point : CenterPoints)
    {
        Centroid += Point;
    }
    Centroid /= CenterPoints.Num();
    
    // 使用简化的 PCA：找到主方向
    // 计算协方差矩阵的元素
    float Cxx = 0.0f, Cyy = 0.0f, Cxy = 0.0f;
    for (const FVector2D& Point : CenterPoints)
    {
        float Dx = Point.X - Centroid.X;
        float Dy = Point.Y - Centroid.Y;
        Cxx += Dx * Dx;
        Cyy += Dy * Dy;
        Cxy += Dx * Dy;
    }
    
    // 计算主方向（协方差矩阵的主特征向量）
    // 对于 2x2 矩阵 [[Cxx, Cxy], [Cxy, Cyy]]，主特征向量方向为：
    // theta = 0.5 * atan2(2 * Cxy, Cxx - Cyy)
    float Theta = 0.5f * FMath::Atan2(2.0f * Cxy, Cxx - Cyy);
    FVector2D PrincipalDirection(FMath::Cos(Theta), FMath::Sin(Theta));
    
    // 将所有点投影到主方向上，找到最小和最大投影值的点
    float MinProjection = TNumericLimits<float>::Max();
    float MaxProjection = TNumericLimits<float>::Lowest();
    int32 MinIndex = 0;
    int32 MaxIndex = 0;
    
    for (int32 i = 0; i < CenterPoints.Num(); ++i)
    {
        // 计算点到质心的向量在主方向上的投影
        FVector2D ToPoint = CenterPoints[i] - Centroid;
        float Projection = FVector2D::DotProduct(ToPoint, PrincipalDirection);
        
        if (Projection < MinProjection)
        {
            MinProjection = Projection;
            MinIndex = i;
        }
        if (Projection > MaxProjection)
        {
            MaxProjection = Projection;
            MaxIndex = i;
        }
    }
    
    // 返回首尾两个端点
    Result.Add(CenterPoints[MinIndex]);
    Result.Add(CenterPoints[MaxIndex]);
    
    return Result;
}

//=========================================================================
// CollectActorGuids - 收集所有选中 Actor 的 GUID
// Requirements: 4.3, 5.4
//=========================================================================

TArray<FString> UMAModifyWidget::CollectActorGuids(const TArray<AActor*>& Actors)
{
    TArray<FString> Guids;
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Guids.Add(Actor->GetActorGuid().ToString());
        }
    }
    return Guids;
}

//=========================================================================
// GetNextAvailableId - 获取下一个可用的节点 ID
// 读取 scene_graph_cyberworld.json，找到最大 ID 并返回 +1
// Requirements: 9.1, 9.2, 9.3, 9.4
//=========================================================================

FString UMAModifyWidget::GetNextAvailableId()
{
    // 获取项目路径
    FString ProjectDir = FPaths::ProjectDir();
    FString JsonFilePath = FPaths::Combine(ProjectDir, TEXT("datasets/scene_graph_cyberworld.json"));
    
    // 检查文件是否存在
    if (!FPaths::FileExists(JsonFilePath))
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GetNextAvailableId: JSON file not found at %s, returning '1'"), *JsonFilePath);
        return TEXT("1");
    }
    
    // 读取文件内容
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GetNextAvailableId: Failed to read JSON file, returning '1'"));
        return TEXT("1");
    }
    
    // 解析 JSON
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GetNextAvailableId: Failed to parse JSON, returning '1'"));
        return TEXT("1");
    }
    
    // 获取 nodes 数组
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!RootObject->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GetNextAvailableId: No 'nodes' array found, returning '1'"));
        return TEXT("1");
    }
    
    // 遍历节点找到最大数字 ID
    int32 MaxId = 0;
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        const TSharedPtr<FJsonObject>* NodeObject;
        if (NodeValue->TryGetObject(NodeObject))
        {
            FString IdStr;
            if ((*NodeObject)->TryGetStringField(TEXT("id"), IdStr))
            {
                // 尝试将 ID 转换为数字
                if (IdStr.IsNumeric())
                {
                    int32 IdNum = FCString::Atoi(*IdStr);
                    if (IdNum > MaxId)
                    {
                        MaxId = IdNum;
                    }
                }
            }
        }
    }
    
    // 返回 (最大 ID + 1) 的字符串
    int32 NextId = MaxId + 1;
    UE_LOG(LogMAModifyWidget, Log, TEXT("GetNextAvailableId: Max ID = %d, Next ID = %d"), MaxId, NextId);
    return FString::FromInt(NextId);
}

//=========================================================================
// GetDefaultPropertiesForCategory - 根据分类获取默认属性字段
// Requirements: 6.1, 7.1, 8.1
//=========================================================================

TMap<FString, FString> UMAModifyWidget::GetDefaultPropertiesForCategory(EMANodeCategory Category)
{
    TMap<FString, FString> DefaultProperties;
    
    switch (Category)
    {
    case EMANodeCategory::Building:
        // Building 类型默认字段
        DefaultProperties.Add(TEXT("category"), TEXT("building"));
        DefaultProperties.Add(TEXT("status"), TEXT("undiscovered"));
        DefaultProperties.Add(TEXT("visibility"), TEXT("high"));
        DefaultProperties.Add(TEXT("wind_condition"), TEXT("weak"));
        DefaultProperties.Add(TEXT("congestion"), TEXT("none"));
        DefaultProperties.Add(TEXT("is_fire"), TEXT("false"));
        DefaultProperties.Add(TEXT("is_spill"), TEXT("false"));
        break;
        
    case EMANodeCategory::TransFacility:
        // TransFacility 类型默认字段 (同 Building)
        DefaultProperties.Add(TEXT("category"), TEXT("trans_facility"));
        DefaultProperties.Add(TEXT("status"), TEXT("undiscovered"));
        DefaultProperties.Add(TEXT("visibility"), TEXT("high"));
        DefaultProperties.Add(TEXT("wind_condition"), TEXT("weak"));
        DefaultProperties.Add(TEXT("congestion"), TEXT("none"));
        DefaultProperties.Add(TEXT("is_fire"), TEXT("false"));
        DefaultProperties.Add(TEXT("is_spill"), TEXT("false"));
        break;
        
    case EMANodeCategory::Prop:
        // Prop 类型默认字段
        DefaultProperties.Add(TEXT("category"), TEXT("prop"));
        DefaultProperties.Add(TEXT("is_abnormal"), TEXT("false"));
        break;
        
    case EMANodeCategory::None:
    default:
        // 无分类时不添加默认字段
        break;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("GetDefaultPropertiesForCategory: Category=%d, Properties count=%d"), 
        static_cast<int32>(Category), DefaultProperties.Num());
    
    return DefaultProperties;
}

//=========================================================================
// ParseAnnotationInputV2 - 解析新格式的标注输入 (cate:xxx,type:xxx)
// Requirements: 5.1, 5.2, 5.3, 5.4, 5.5
//=========================================================================

bool UMAModifyWidget::ParseAnnotationInputV2(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError)
{
    OutResult.Reset();
    OutError.Empty();
    
    // 检查空输入
    if (Input.IsEmpty())
    {
        OutError = TEXT("输入不能为空");
        return false;
    }
    
    // 解析 "key:value, key:value" 格式
    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);
    
    bool bHasCate = false;
    bool bHasType = false;
    
    for (const FString& Pair : Pairs)
    {
        FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }
        
        // 分割 key:value
        FString Key, Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("无效的键值对格式: %s (应为 key:value)"), *TrimmedPair);
            return false;
        }
        
        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();
        
        if (Key.IsEmpty())
        {
            OutError = TEXT("键名不能为空");
            return false;
        }
        
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("键 '%s' 的值不能为空"), *Key);
            return false;
        }
        
        // 解析已知字段
        if (Key == TEXT("cate") || Key == TEXT("category"))
        {
            // 解析 category 字段
            EMANodeCategory ParsedCategory = FMAAnnotationInput::ParseCategoryFromString(Value);
            if (ParsedCategory == EMANodeCategory::None)
            {
                OutError = FString::Printf(TEXT("无效的 cate 值: %s (应为 building, trans_facility 或 prop)"), *Value);
                return false;
            }
            OutResult.Category = ParsedCategory;
            bHasCate = true;
        }
        else if (Key == TEXT("id"))
        {
            OutResult.Id = Value;
        }
        else if (Key == TEXT("type"))
        {
            OutResult.Type = Value;
            bHasType = true;
        }
        else if (Key == TEXT("shape"))
        {
            // 验证 shape 值
            FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") && 
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle"))
            {
                OutError = FString::Printf(TEXT("无效的 shape 值: %s (应为 polygon, linestring, point 或 rectangle)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
        }
        else
        {
            // 存储为额外属性
            OutResult.Properties.Add(Key, Value);
        }
    }
    
    // 验证必需字段
    if (!bHasCate)
    {
        OutError = TEXT("缺少必需字段: cate (应为 building, trans_facility 或 prop)");
        return false;
    }
    
    if (!bHasType)
    {
        OutError = TEXT("缺少必需字段: type");
        return false;
    }
    
    // 如果没有指定 ID，自动分配
    if (OutResult.Id.IsEmpty())
    {
        OutResult.Id = GetNextAvailableId();
        UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: Auto-assigned ID = %s"), *OutResult.Id);
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: %s"), *OutResult.ToString());
    return true;
}
