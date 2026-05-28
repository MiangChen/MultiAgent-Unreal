#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "UI/Components/Domain/MATaskGraphPreviewModels.h"
#include "MATaskGraphPreview.generated.h"

class UTextBlock;
class UBorder;
class USizeBox;
class UMAUITheme;

UCLASS()
class MULTIAGENT_API UMATaskGraphPreview : public UUserWidget
{
    GENERATED_BODY()

public:
    UMATaskGraphPreview(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdatePreview(const FMATaskGraphData& Data);

    UFUNCTION(BlueprintCallable, Category = "Preview")
    void ClearPreview();

    UFUNCTION(BlueprintPure, Category = "Preview")
    bool HasData() const { return bHasData; }

    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                              const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                              int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "50.0", ClampMax = "300.0"))
    float PreviewHeight = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    UPROPERTY()
    USizeBox* RootSizeBox;

    UPROPERTY()
    UBorder* ContentBorder;

    UPROPERTY()
    UTextBlock* StatsText;

    UPROPERTY()
    UTextBlock* EmptyText;

    bool bHasData = false;
    int32 HoveredNodeIndex = -1;
    FMATaskGraphPreviewModel CurrentModel;
    TArray<FMATaskGraphPreviewNodeRenderData> NodeRenderData;
    TArray<FMATaskGraphPreviewEdgeRenderData> EdgeRenderData;

    float NodeWidth = 50.0f;
    float NodeHeight = 24.0f;
    float TopMargin = 25.0f;
    float LeftMargin = 10.0f;

    FLinearColor BackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);
    FLinearColor EdgeColor = FLinearColor(0.5f, 0.5f, 0.6f, 0.8f);
    FLinearColor PendingColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);
    FLinearColor HoverHighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
    FLinearColor TooltipBgColor = FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);
    FLinearColor TooltipTextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
    FLinearColor TextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    FLinearColor HintTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    void BuildUI();
    void RefreshRenderData(const FGeometry& AllottedGeometry);
    void ApplyModelState();

    void DrawDAG(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawNode(const FMATaskGraphPreviewNodeRenderData& Node, int32 NodeIndex, const FGeometry& AllottedGeometry,
                  FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawEdge(const FMATaskGraphPreviewEdgeRenderData& Edge, const FGeometry& AllottedGeometry,
                  FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                   const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    FLinearColor GetStatusColor(EMATaskExecutionStatus Status) const;
    int32 FindNodeAtPosition(const FVector2D& LocalPosition) const;
};
