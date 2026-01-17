// MAModifyWidget.cpp
// Modify Mode Panel Widget - Pure C++ Implementation
// Supports single and multi-select modes

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

// Hint text constants (in anonymous namespace to avoid linker conflicts)
namespace
{
    const FString ModifyDefaultHintText = TEXT("Input format: cate:building/trans_facility/prop,type:xxx\n• building: Buildings (prism modeling, single-select only)\n• trans_facility: Transport facilities (OBB rectangle)\n• prop: Props (point modeling)");
    const FString ModifyMultiSelectHintText = TEXT("Multi-select mode - Input format: cate:trans_facility/prop,type:xxx\nNote: building type only supports single-select");
    const FString ModifyBuildingHintText = TEXT("Building type (prism modeling)\nInput format: cate:building,type:xxx\n• Single-select only\n• Auto-calculates base polygon and height");
    const FString ModifyTransFacilityHintText = TEXT("TransFacility type (OBB rectangle modeling)\nInput format: cate:trans_facility,type:xxx\n• Supports single or multi-select\n• Auto-calculates minimum bounding rectangle");
    const FString ModifyPropHintText = TEXT("Prop type (point modeling)\nInput format: cate:prop,type:xxx\n• Supports single or multi-select\n• Auto-calculates geometric center");
}

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

    // Title
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Modify Panel")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f)));  // Orange, matches Modify mode color
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Hint text
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(ModifyDefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // JSON preview text box - read-only, displays JSON fragment for selected Actor
    UBorder* JsonPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonPreviewBorder"));
    JsonPreviewBorder->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));  // Dark gray background
    JsonPreviewBorder->SetPadding(FMargin(8.0f));
    
    UScrollBox* JsonScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JsonScrollBox"));
    JsonPreviewBorder->AddChild(JsonScrollBox);
    
    JsonPreviewText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonPreviewText"));
    JsonPreviewText->SetText(FText::FromString(TEXT("Select an Actor to display JSON preview")));
    FSlateFontInfo JsonFont = JsonPreviewText->GetFont();
    JsonFont.Size = 10;
    JsonPreviewText->SetFont(JsonFont);
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 0.7f)));  // Light green
    JsonPreviewText->SetAutoWrapText(true);
    JsonScrollBox->AddChild(JsonPreviewText);
    
    USizeBox* JsonPreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonPreviewSizeBox"));
    JsonPreviewSizeBox->SetMinDesiredHeight(80.0f);
    JsonPreviewSizeBox->SetMaxDesiredHeight(100.0f);
    JsonPreviewSizeBox->AddChild(JsonPreviewBorder);
    
    UVerticalBoxSlot* JsonPreviewSlot = MainVBox->AddChildToVerticalBox(JsonPreviewSizeBox);
    JsonPreviewSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Multi-line text box
    LabelTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("LabelTextBox"));
    LabelTextBox->SetHintText(FText::FromString(ModifyDefaultHintText));
    
    // Set text color to strict black - via WidgetStyle property
    FEditableTextBoxStyle TextBoxStyle;
    TextBoxStyle.SetForegroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));  // Strict black
    LabelTextBox->WidgetStyle = TextBoxStyle;
    
    // Use SizeBox to set minimum height
    USizeBox* TextBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TextBoxSizeBox"));
    TextBoxSizeBox->SetMinDesiredHeight(150.0f);
    TextBoxSizeBox->AddChild(LabelTextBox);
    
    UVerticalBoxSlot* TextBoxSlot = MainVBox->AddChildToVerticalBox(TextBoxSizeBox);
    TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TextBoxSlot->SetPadding(FMargin(0, 0, 0, 15));

    // Confirm button
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("  Confirm  ")));
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
    return FString::Printf(TEXT("Actor: %s\nLabel: %s"), *ActorName, *Label);
}


//=========================================================================
// SetSelectedActor - 设置选中的 Actor (单选模式)
//=========================================================================

void UMAModifyWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        // 如果传入 nullptr，清除选择
        ClearSelection();
        return;
    }
    
    // Single-select mode: clear array and add single Actor
    SelectedActors.Empty();
    SelectedActors.Add(Actor);
    
    // Clear text box, let user input new label
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
        LabelTextBox->SetHintText(FText::FromString(ModifyDefaultHintText));
    }
    
    // Update hint text
    if (HintText)
    {
        HintText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %s"), *Actor->GetName())));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));  // Green indicates selected
    }
    
    // Update JSON preview
    UpdateJsonPreview(Actor);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetSelectedActor: %s"), *Actor->GetName());
}

//=========================================================================
// SetSelectedActors - Set multiple selected Actors (multi-select mode)
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
    
    // Update hint text to display selection count
    if (HintText)
    {
        if (SelectedActors.Num() == 1)
        {
            HintText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %s"), *SelectedActors[0]->GetName())));
        }
        else
        {
            HintText->SetText(FText::FromString(FString::Printf(TEXT("Selected: %d Actors"), SelectedActors.Num())));
        }
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));  // Green indicates selected
    }
    
    // Clear text box, let user input new label
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
        if (SelectedActors.Num() > 1)
        {
            LabelTextBox->SetHintText(FText::FromString(ModifyMultiSelectHintText));
        }
        else
        {
            LabelTextBox->SetHintText(FText::FromString(ModifyDefaultHintText));
        }
    }
    
    // Update JSON preview
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
// ClearSelection - Clear selection state
//=========================================================================

void UMAModifyWidget::ClearSelection()
{
    SelectedActors.Empty();
    
    // Clear text box
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
    }
    
    // Display hint text
    if (HintText)
    {
        HintText->SetText(FText::FromString(ModifyDefaultHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));  // Gray
    }
    
    // Clear JSON preview
    if (JsonPreviewText)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("Select an Actor to display JSON preview")));
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

//=========================================================================
// GetLabelText / SetLabelText - Text box content access
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
    
    // 解析输入 (支持新旧格式)
    FMAAnnotationInput ParsedInput;
    FString ParseError;
    if (!ParseAnnotationInput(LabelContent, ParsedInput, ParseError))
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: Parse failed - %s"), *ParseError);
        // 解析失败，广播单选委托让 MAHUD 处理错误显示
        OnModifyConfirmed.Broadcast(SelectedActors[0], LabelContent);
        return;
    }
    if (ParsedInput.HasCategory())
    {
        FString ValidationError;
        if (!ValidateSelectionForCategory(ParsedInput.Category, SelectedActors.Num(), ValidationError))
        {
            UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: Validation failed - %s"), *ValidationError);
            // 验证失败，广播单选委托让 MAHUD 处理错误显示
            // 将错误信息附加到 LabelContent 以便 MAHUD 显示
            OnModifyConfirmed.Broadcast(SelectedActors[0], FString::Printf(TEXT("ERROR: %s"), *ValidationError));
            return;
        }
    }
    
    // 根据是否有 Category 决定使用哪个 JSON 生成方法
    FString GeneratedJson;
    if (ParsedInput.HasCategory())
    {
        // 新格式 (cate:xxx,type:xxx) - 使用 GenerateSceneGraphNodeV2
        GeneratedJson = GenerateSceneGraphNodeV2(ParsedInput, SelectedActors);
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Using GenerateSceneGraphNodeV2 for Category=%s"), 
            *ParsedInput.GetCategoryString());
    }
    else if (ParsedInput.IsMultiSelect())
    {
        // 旧格式多选 (id:xxx,type:xxx,shape:polygon) - 使用 GenerateSceneGraphNode
        GeneratedJson = GenerateSceneGraphNode(ParsedInput, SelectedActors);
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Using GenerateSceneGraphNode for legacy multi-select"));
    }
    else
    {
        // 旧格式单选 - 不生成 JSON，由 MAHUD 处理
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Legacy single-select mode, Actor=%s, LabelText=%s"), 
            *SelectedActors[0]->GetName(), *LabelContent);
        OnModifyConfirmed.Broadcast(SelectedActors[0], LabelContent);
        UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Modification confirmed, waiting for MAHUD to handle state"));
        return;
    }
    
    // 广播多选确认委托 (包含生成的 JSON)
    OnMultiSelectModifyConfirmed.Broadcast(SelectedActors, LabelContent, GeneratedJson);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Modification confirmed with JSON, waiting for MAHUD to handle state"));
}

//=========================================================================
// UpdateJsonPreview - 更新 JSON 预览文本
// 显示选中 Actor 对应的 JSON 片段
//
// 改进逻辑:
// 1. 获取选中 Actor 的 GUID
// 2. 同时查找 GuidArray 匹配 (polygon/linestring) 和 guid 字段匹配 (point)
// 3. 如果找到多个匹配节点，显示所有节点的 JSON
// 4. If not found, maintain existing single Actor preview behavior
//=========================================================================

void UMAModifyWidget::UpdateJsonPreview(AActor* Actor)
{
    if (!JsonPreviewText)
    {
        return;
    }
    
    if (!Actor)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("Select an Actor to display JSON preview")));
        return;
    }
    
    // Get SceneGraphManager
    UWorld* World = GetWorld();
    if (!World)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("Unable to get World")));
        return;
    }
    
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("Unable to get GameInstance")));
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("SceneGraphManager not found")));
        return;
    }
    
    // Get Actor's GUID
    FString ActorGuid = Actor->GetActorGuid().ToString();
    
    // Collect all matching nodes (including GuidArray matches and guid field matches)
    TArray<FMASceneGraphNode> AllMatchingNodes;
    
    // 1. Find via GuidArray (polygon/linestring types)
    TArray<FMASceneGraphNode> GuidArrayMatches = SceneGraphManager->FindNodesByGuid(ActorGuid);
    AllMatchingNodes.Append(GuidArrayMatches);
    
    // 2. Find via single guid field (point type)
    TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Guid == ActorGuid)
        {
            // Check if already in AllMatchingNodes (avoid duplicates)
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
        // Found matching nodes, display JSON for all nodes
        FString CombinedPreview;
        
        // If multiple nodes, add count hint
        if (AllMatchingNodes.Num() > 1)
        {
            CombinedPreview = FString::Printf(TEXT("This Actor belongs to %d nodes:\n"), AllMatchingNodes.Num());
            CombinedPreview += TEXT("━━━━━━━━━━━━━━━━━━━━\n\n");
        }
        
        // Display info for each node
        for (int32 i = 0; i < AllMatchingNodes.Num(); ++i)
        {
            const FMASceneGraphNode& Node = AllMatchingNodes[i];
            
            // Add node separator (if not first)
            if (i > 0)
            {
                CombinedPreview += TEXT("\n────────────────────\n\n");
            }
            
            // Format and add node info
            CombinedPreview += FormatJsonPreviewWithHighlight(Node, ActorGuid);
        }
        
        JsonPreviewText->SetText(FText::FromString(CombinedPreview));
        JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 0.7f)));  // Light green
        
        UE_LOG(LogMAModifyWidget, Log, TEXT("UpdateJsonPreview: Found %d nodes for Actor GUID %s"), 
            AllMatchingNodes.Num(), *ActorGuid);
        return;
    }
    
    // No matching nodes found, display default single Actor preview
    FVector Location = Actor->GetActorLocation();
    FString PreviewText = FString::Printf(
        TEXT("Actor: %s\nGUID: %s\nLocation: [%.0f, %.0f, %.0f]\n\n(Not annotated - does not belong to any node)"),
        *Actor->GetName(),
        *ActorGuid,
        Location.X,
        Location.Y,
        Location.Z
    );
    JsonPreviewText->SetText(FText::FromString(PreviewText));
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.7f, 0.5f)));  // Orange indicates not annotated
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("UpdateJsonPreview: No node found for Actor GUID %s"), *ActorGuid);
}

//=========================================================================
// UpdateJsonPreviewMultiSelect - Update JSON preview text (multi-select mode)
//=========================================================================

void UMAModifyWidget::UpdateJsonPreviewMultiSelect(const TArray<AActor*>& Actors)
{
    if (!JsonPreviewText)
    {
        return;
    }
    
    if (Actors.Num() == 0)
    {
        JsonPreviewText->SetText(FText::FromString(TEXT("Select an Actor to display JSON preview")));
        return;
    }
    
    // Build multi-select preview info
    FString PreviewText = FString::Printf(TEXT("Multi-select mode: %d Actors\n\n"), Actors.Num());
    
    // List all selected Actors
    PreviewText += TEXT("Selected Actors:\n");
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
        PreviewText += FString::Printf(TEXT("  ... and %d more\n"), Actors.Num() - 5);
    }
    
    PreviewText += TEXT("\nEnter shape:polygon or shape:linestring");
    
    JsonPreviewText->SetText(FText::FromString(PreviewText));
    JsonPreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.7f, 0.9f)));  // Light blue indicates multi-select
}

//=========================================================================
// FormatJsonPreviewWithHighlight - Format JSON preview and highlight selected Actor's GUID
//
// Features:
// - Add node type title (Polygon/LineString/Point)
// - Highlight selected Actor's GUID (using >>> <<< markers)
// - Use indentation to format JSON
//=========================================================================

FString UMAModifyWidget::FormatJsonPreviewWithHighlight(const FMASceneGraphNode& Node, const FString& ActorGuid) const
{
    FString Result;
    FString TypeTitle;
    if (Node.ShapeType == TEXT("polygon"))
    {
        TypeTitle = TEXT("=== Polygon Node ===");
    }
    else if (Node.ShapeType == TEXT("linestring"))
    {
        TypeTitle = TEXT("=== LineString Node ===");
    }
    else
    {
        TypeTitle = TEXT("=== Point Node ===");
    }
    Result += TypeTitle + TEXT("\n\n");
    // Use RawJson field, but simplify display to fit preview box
    
    // Basic info
    Result += FString::Printf(TEXT("id: %s\n"), *Node.Id);
    Result += FString::Printf(TEXT("type: %s\n"), *Node.Type);
    Result += FString::Printf(TEXT("label: %s\n"), *Node.Label);
    
    // Display center point
    Result += FString::Printf(TEXT("center: [%.0f, %.0f, %.0f]\n"), 
        Node.Center.X, Node.Center.Y, Node.Center.Z);
    if (Node.GuidArray.Num() > 0)
    {
        Result += TEXT("\nGuid Array:\n");
        for (int32 i = 0; i < Node.GuidArray.Num(); ++i)
        {
            const FString& Guid = Node.GuidArray[i];
            if (Guid == ActorGuid)
            {
                // Highlight selected GUID
                Result += FString::Printf(TEXT("  [%d] >>> %s <<< (selected)\n"), i, *Guid);
            }
            else
            {
                Result += FString::Printf(TEXT("  [%d] %s\n"), i, *Guid);
            }
        }
    }
    else if (!Node.Guid.IsEmpty())
    {
        // Point type node, display single guid
        if (Node.Guid == ActorGuid)
        {
            Result += FString::Printf(TEXT("\nguid: >>> %s <<< (selected)\n"), *Node.Guid);
        }
        else
        {
            Result += FString::Printf(TEXT("\nguid: %s\n"), *Node.Guid);
        }
    }
    
    return Result;
}

//=========================================================================
// ParseAnnotationInput - Parse annotation input string
// Supports backward compatibility: detect input format, new format calls ParseAnnotationInputV2
//=========================================================================

bool UMAModifyWidget::ParseAnnotationInput(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError)
{
    OutResult.Reset();
    OutError.Empty();
    
    // Check for empty input
    if (Input.IsEmpty())
    {
        OutError = TEXT("Input cannot be empty");
        return false;
    }
    
    // Detect input format: new format contains "cate:" or "category:"
    FString InputLower = Input.ToLower();
    if (InputLower.Contains(TEXT("cate:")) || InputLower.Contains(TEXT("category:")))
    {
        // New format: call ParseAnnotationInputV2
        UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: Detected new format (cate:xxx), delegating to ParseAnnotationInputV2"));
        return ParseAnnotationInputV2(Input, OutResult, OutError);
    }
    
    // Legacy format: maintain original logic
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: Using legacy format (id:xxx,type:xxx)"));
    
    // Parse "key:value, key:value" format
    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);
    
    for (const FString& Pair : Pairs)
    {
        FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }
        
        // Split key:value
        FString Key, Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("Invalid key-value format: %s (expected key:value)"), *TrimmedPair);
            return false;
        }
        
        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();
        
        if (Key.IsEmpty())
        {
            OutError = TEXT("Key name cannot be empty");
            return false;
        }
        
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Value for key '%s' cannot be empty"), *Key);
            return false;
        }
        
        // Parse known fields
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
            // Validate shape value
            FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") && 
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle"))
            {
                OutError = FString::Printf(TEXT("Invalid shape value: %s (expected polygon, linestring, point or rectangle)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
        }
        else
        {
            // Store as extra property
            OutResult.Properties.Add(Key, Value);
        }
    }
    
    // Validate required fields (legacy format requires id)
    if (OutResult.Id.IsEmpty())
    {
        OutError = TEXT("Missing required field: id");
        return false;
    }
    
    if (OutResult.Type.IsEmpty())
    {
        OutError = TEXT("Missing required field: type");
        return false;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInput: %s"), *OutResult.ToString());
    return true;
}

//=========================================================================
// GenerateSceneGraphNode - Generate scene graph node JSON
//=========================================================================

FString UMAModifyWidget::GenerateSceneGraphNode(const FMAAnnotationInput& Input, const TArray<AActor*>& Actors)
{
    if (Actors.Num() == 0)
    {
        return TEXT("{}");
    }
    
    // Create JSON object
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    
    // Basic fields
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
    
    // 生成标签 - 使用 MASceneGraphManager::GenerateLabel() 生成 Type-N 格式
    FString Label;
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    if (SceneGraphManager)
    {
        Label = SceneGraphManager->GenerateLabel(Input.Type);
    }
    else
    {
        // 回退：MASceneGraphManager 不可用时使用 Type-1 格式
        FString CapitalizedType = Input.Type;
        if (CapitalizedType.Len() > 0)
        {
            CapitalizedType[0] = FChar::ToUpper(CapitalizedType[0]);
        }
        Label = FString::Printf(TEXT("%s-1"), *CapitalizedType);
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNode: MASceneGraphManager not available, using fallback label: %s"), *Label);
    }
    PropertiesObject->SetStringField(TEXT("label"), Label);
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
// GenerateSceneGraphNodeV2 - 生成场景图节点 JSON V2 (支持新的形状类型)
// 根据 Category 调用不同的几何计算方法:
// - Building: ComputePrismFromActors → Prism JSON
// - TransFacility: ComputeOBBFromActors → LineString + Vertices JSON
// - Prop: ComputeCenterFromActors → Point JSON
//=========================================================================

FString UMAModifyWidget::GenerateSceneGraphNodeV2(const FMAAnnotationInput& Input, const TArray<AActor*>& Actors)
{
    if (Actors.Num() == 0)
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: No actors provided"));
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
    
    // 生成标签 - 使用 MASceneGraphManager::GenerateLabel() 生成 Type-N 格式
    FString Label;
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    if (SceneGraphManager)
    {
        Label = SceneGraphManager->GenerateLabel(Input.Type);
    }
    else
    {
        // 回退：MASceneGraphManager 不可用时使用 Type-1 格式
        FString CapitalizedType = Input.Type;
        if (CapitalizedType.Len() > 0)
        {
            CapitalizedType[0] = FChar::ToUpper(CapitalizedType[0]);
        }
        Label = FString::Printf(TEXT("%s-1"), *CapitalizedType);
        UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: MASceneGraphManager not available, using fallback label: %s"), *Label);
    }
    PropertiesObject->SetStringField(TEXT("label"), Label);
    
    // 添加 category 字段
    if (Input.HasCategory())
    {
        PropertiesObject->SetStringField(TEXT("category"), Input.GetCategoryString());
    }
    
    // 添加默认属性
    if (Input.HasCategory())
    {
        TMap<FString, FString> DefaultProps = GetDefaultPropertiesForCategory(Input.Category);
        for (const auto& DefaultProp : DefaultProps)
        {
            // 跳过 category，已经添加过了
            if (DefaultProp.Key == TEXT("category"))
            {
                continue;
            }
            // 检查用户是否已经在 Input.Properties 中指定了该字段
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
    
    // shape 对象 - 根据 Category 生成不同的形状
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    
    switch (Input.Category)
    {
    case EMANodeCategory::Building:
        {
            // Building 类型 - Prism (棱柱)
            ShapeObject->SetStringField(TEXT("type"), TEXT("prism"));
            
            // 调用 ComputePrismFromActors 获取几何数据
            FMAPrismGeometry PrismGeometry = FMAGeometryUtils::ComputePrismFromActors(Actors);
            
            if (PrismGeometry.bIsValid)
            {
                // 添加 vertices 字段 (底面顶点)
                TArray<TSharedPtr<FJsonValue>> VerticesArray;
                for (const FVector& Vertex : PrismGeometry.BottomVertices)
                {
                    TArray<TSharedPtr<FJsonValue>> PointArray;
                    PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.X)));
                    PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Y)));
                    PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Z)));
                    VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                }
                ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                
                // 添加 height 字段
                ShapeObject->SetNumberField(TEXT("height"), PrismGeometry.Height);
                
                UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNodeV2: Building - Prism with %d vertices, height=%.2f"), 
                    PrismGeometry.BottomVertices.Num(), PrismGeometry.Height);
            }
            else
            {
                // 回退到边界框近似
                UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: Prism computation failed, using fallback"));
                
                // 使用第一个 Actor 的边界框
                if (Actors[0])
                {
                    FVector Origin, BoxExtent;
                    Actors[0]->GetActorBounds(false, Origin, BoxExtent);
                    
                    // 生成矩形底面顶点
                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    float MinZ = Origin.Z - BoxExtent.Z;
                    
                    TArray<FVector2D> Corners = {
                        FVector2D(Origin.X - BoxExtent.X, Origin.Y - BoxExtent.Y),
                        FVector2D(Origin.X + BoxExtent.X, Origin.Y - BoxExtent.Y),
                        FVector2D(Origin.X + BoxExtent.X, Origin.Y + BoxExtent.Y),
                        FVector2D(Origin.X - BoxExtent.X, Origin.Y + BoxExtent.Y)
                    };
                    
                    for (const FVector2D& Corner : Corners)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(MinZ)));
                        VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                    ShapeObject->SetNumberField(TEXT("height"), BoxExtent.Z * 2.0f);
                }
            }
        }
        break;
        
    case EMANodeCategory::TransFacility:
        {
            // TransFacility 类型处理逻辑:
            // - 如果 type 为 "intersection"，shape.type = "point"，center 是矩形 vertices 的几何中心
            // - 如果 type 为其他值，shape.type = "linestring"，points 是矩形短边的两个中点
            // - 两种情况都包含 vertices 字段 (OBB 矩形的四个角点)
            
            // 调用 ComputeOBBFromActors 获取 OBB 几何数据
            FMAOBBGeometry OBBGeometry = FMAGeometryUtils::ComputeOBBFromActors(Actors);
            
            // 判断是否为 intersection 类型
            bool bIsIntersection = Input.Type.Equals(TEXT("intersection"), ESearchCase::IgnoreCase);
            
            if (bIsIntersection)
            {
                // intersection 类型 - Point (矩形几何中心)
                ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
                
                if (OBBGeometry.bIsValid && OBBGeometry.CornerPoints.Num() == 4)
                {
                    // 计算矩形 vertices 的几何中心
                    FVector Center = FVector::ZeroVector;
                    for (const FVector& Corner : OBBGeometry.CornerPoints)
                    {
                        Center += Corner;
                    }
                    Center /= 4.0f;
                    
                    // 添加 center 字段
                    TArray<TSharedPtr<FJsonValue>> CenterArray;
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
                    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
                    
                    // 添加 vertices 字段 (OBB 四个角点)
                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector& Corner : OBBGeometry.CornerPoints)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.Z)));
                        VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                    
                    UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNodeV2: TransFacility (intersection) - Point at [%.2f, %.2f, %.2f], OBB with %d corners"), 
                        Center.X, Center.Y, Center.Z, OBBGeometry.CornerPoints.Num());
                }
                else
                {
                    // OBB 计算失败，使用 Actor 几何中心作为回退
                    UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: OBB computation failed for intersection, using actor center fallback"));
                    
                    FVector Center = FMAGeometryUtils::ComputeCenterFromActors(Actors);
                    TArray<TSharedPtr<FJsonValue>> CenterArray;
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
                    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
                    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
                }
            }
            else
            {
                // 非 intersection 类型 - LineString (矩形短边中点)
                ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
                
                if (OBBGeometry.bIsValid && OBBGeometry.CornerPoints.Num() == 4)
                {
                    // 计算矩形短边的两个中点作为 points
                    TArray<FVector> ShortEdgeMidpoints = FMAGeometryUtils::ComputeShortEdgeMidpoints(OBBGeometry.CornerPoints);
                    
                    TArray<TSharedPtr<FJsonValue>> PointsArray;
                    for (const FVector& Midpoint : ShortEdgeMidpoints)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Midpoint.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Midpoint.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Midpoint.Z)));
                        PointsArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("points"), PointsArray);
                    
                    // 添加 vertices 字段 (OBB 四个角点)
                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector& Corner : OBBGeometry.CornerPoints)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Corner.Z)));
                        VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                    
                    UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNodeV2: TransFacility - LineString with %d short edge midpoints, OBB with %d corners"), 
                        ShortEdgeMidpoints.Num(), OBBGeometry.CornerPoints.Num());
                }
                else
                {
                    // OBB 计算失败，使用旧的 LineString 计算逻辑作为回退
                    UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: OBB computation failed, using legacy linestring fallback"));
                    
                    TArray<FVector2D> LinePoints = ComputeLineString(Actors);
                    TArray<TSharedPtr<FJsonValue>> PointsArray;
                    for (const FVector2D& Point : LinePoints)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Point.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Point.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
                        PointsArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("points"), PointsArray);
                    
                    // 使用凸包作为 vertices 回退
                    TArray<FVector2D> ConvexHull = ComputeConvexHull(Actors);
                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector2D& Vertex : ConvexHull)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.X)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Y)));
                        PointArray.Add(MakeShareable(new FJsonValueNumber(0.0)));
                        VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                }
            }
        }
        break;
        
    case EMANodeCategory::Prop:
        {
            // Prop 类型 - Point (几何中心)
            ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
            
            // 调用 ComputeCenterFromActors 获取几何中心
            FVector Center = FMAGeometryUtils::ComputeCenterFromActors(Actors);
            
            // 添加 center 字段
            TArray<TSharedPtr<FJsonValue>> CenterArray;
            CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
            CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
            CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
            ShapeObject->SetArrayField(TEXT("center"), CenterArray);
            
            UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNodeV2: Prop - Point at [%.2f, %.2f, %.2f]"), 
                Center.X, Center.Y, Center.Z);
        }
        break;
        
    case EMANodeCategory::None:
    default:
        {
            // 未指定分类，回退到旧的逻辑
            UE_LOG(LogMAModifyWidget, Warning, TEXT("GenerateSceneGraphNodeV2: No category specified, falling back to legacy logic"));
            
            // 根据 Shape 字段判断
            if (Input.IsPolygon())
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("polygon"));
                TArray<FVector2D> Vertices = ComputeConvexHull(Actors);
                TArray<TSharedPtr<FJsonValue>> VerticesArray;
                for (const FVector2D& Vertex : Vertices)
                {
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
                ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
                TArray<FVector2D> Points = ComputeLineString(Actors);
                TArray<TSharedPtr<FJsonValue>> PointsArray;
                for (const FVector2D& Point : Points)
                {
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
                // 默认为 Point 类型
                ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
                FVector Center = Actors[0]->GetActorLocation();
                TArray<TSharedPtr<FJsonValue>> CenterArray;
                CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
                CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
                CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
                ShapeObject->SetArrayField(TEXT("center"), CenterArray);
            }
        }
        break;
    }
    
    RootObject->SetObjectField(TEXT("shape"), ShapeObject);
    
    // 序列化为 JSON 字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("GenerateSceneGraphNodeV2: Generated JSON for %d actors, Category=%s"), 
        Actors.Num(), *Input.GetCategoryString());
    return OutputString;
}

//=========================================================================
// ComputeConvexHull - 计算凸包顶点
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
// ValidateSelectionForCategory - 验证选择是否符合分类约束
//=========================================================================

bool UMAModifyWidget::ValidateSelectionForCategory(EMANodeCategory Category, int32 SelectionCount, FString& OutError)
{
    OutError.Empty();
    
    // Check if selection count is valid
    if (SelectionCount <= 0)
    {
        OutError = TEXT("Please select at least one Actor");
        UE_LOG(LogMAModifyWidget, Warning, TEXT("ValidateSelectionForCategory: No Actor selected"));
        return false;
    }
    
    switch (Category)
    {
    case EMANodeCategory::Building:
        if (SelectionCount != 1)
        {
            OutError = TEXT("Building type only supports single-select, please select only one Actor");
            UE_LOG(LogMAModifyWidget, Warning, TEXT("ValidateSelectionForCategory: Building requires single selection, got %d"), SelectionCount);
            return false;
        }
        break;
        
    case EMANodeCategory::TransFacility:
        // Any count is valid
        break;
        
    case EMANodeCategory::Prop:
        // Any count is valid
        break;
        
    case EMANodeCategory::None:
    default:
        // When no category specified, allow any count
        break;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ValidateSelectionForCategory: Category=%d, SelectionCount=%d, Valid=true"), 
        static_cast<int32>(Category), SelectionCount);
    return true;
}

//=========================================================================
// ParseAnnotationInputV2 - Parse new format annotation input (cate:xxx,type:xxx)
//
// Auto-set default shape based on cate value:
// - building → prism
// - trans_facility → linestring
// - prop → point
//=========================================================================

bool UMAModifyWidget::ParseAnnotationInputV2(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError)
{
    OutResult.Reset();
    OutError.Empty();
    
    // Check for empty input
    if (Input.IsEmpty())
    {
        OutError = TEXT("Input cannot be empty");
        return false;
    }
    
    // Parse "key:value, key:value" format
    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);
    
    bool bHasCate = false;
    bool bHasType = false;
    bool bHasExplicitShape = false;  // Whether user explicitly specified shape
    
    for (const FString& Pair : Pairs)
    {
        FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }
        
        // Split key:value
        FString Key, Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("Invalid key-value format: %s (expected key:value)"), *TrimmedPair);
            return false;
        }
        
        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();
        
        if (Key.IsEmpty())
        {
            OutError = TEXT("Key name cannot be empty");
            return false;
        }
        
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Value for key '%s' cannot be empty"), *Key);
            return false;
        }
        
        // Parse known fields
        if (Key == TEXT("cate") || Key == TEXT("category"))
        {
            EMANodeCategory ParsedCategory = FMAAnnotationInput::ParseCategoryFromString(Value);
            if (ParsedCategory == EMANodeCategory::None)
            {
                OutError = FString::Printf(TEXT("Invalid cate value: %s (expected building, trans_facility or prop)"), *Value);
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
            // Validate shape value (including new prism type)
            FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") && 
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle") &&
                ShapeLower != TEXT("prism"))
            {
                OutError = FString::Printf(TEXT("Invalid shape value: %s (expected polygon, linestring, point, rectangle or prism)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
            bHasExplicitShape = true;
        }
        else
        {
            // Store as extra property
            OutResult.Properties.Add(Key, Value);
        }
    }
    
    // Validate required fields
    if (!bHasCate)
    {
        OutError = TEXT("Missing required field: cate (expected building, trans_facility or prop)");
        return false;
    }
    
    if (!bHasType)
    {
        OutError = TEXT("Missing required field: type");
        return false;
    }
    
    // If no ID specified, auto-assign
    if (OutResult.Id.IsEmpty())
    {
        OutResult.Id = GetNextAvailableId();
        UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: Auto-assigned ID = %s"), *OutResult.Id);
    }
    // If user didn't explicitly specify shape, auto-set based on category
    // - building → prism
    // - trans_facility → linestring
    // - prop → point
    if (!bHasExplicitShape)
    {
        switch (OutResult.Category)
        {
        case EMANodeCategory::Building:
            OutResult.Shape = TEXT("prism");
            UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: Auto-set shape to 'prism' for building category"));
            break;
            
        case EMANodeCategory::TransFacility:
            OutResult.Shape = TEXT("linestring");
            UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: Auto-set shape to 'linestring' for trans_facility category"));
            break;
            
        case EMANodeCategory::Prop:
            OutResult.Shape = TEXT("point");
            UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: Auto-set shape to 'point' for prop category"));
            break;
            
        case EMANodeCategory::None:
        default:
            // When no category specified, don't auto-set shape
            break;
        }
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ParseAnnotationInputV2: %s"), *OutResult.ToString());
    return true;
}


//=========================================================================
// GetHintTextForCategory - Get corresponding hint text based on category
//=========================================================================

FString UMAModifyWidget::GetHintTextForCategory(EMANodeCategory Category) const
{
    switch (Category)
    {
    case EMANodeCategory::Building:
        return ModifyBuildingHintText;
        
    case EMANodeCategory::TransFacility:
        return ModifyTransFacilityHintText;
        
    case EMANodeCategory::Prop:
        return ModifyPropHintText;
        
    case EMANodeCategory::None:
    default:
        return ModifyDefaultHintText;
    }
}
