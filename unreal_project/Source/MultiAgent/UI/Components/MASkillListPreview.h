#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"
#include "UI/Components/Domain/MASkillListPreviewModels.h"
#include "MASkillListPreview.generated.h"

class UTextBlock;
class UBorder;
class USizeBox;
class UMAUITheme;

UCLASS()
class MULTIAGENT_API UMASkillListPreview : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASkillListPreview(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdatePreview(const FMASkillAllocationData& Data);

    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

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
    int32 HoveredBarIndex = -1;
    FMASkillListPreviewModel CurrentModel;
    TArray<FMASkillListPreviewBarRenderData> SkillBarRenderData;

    float TopMargin = 25.0f;
    float LeftMargin = 60.0f;
    float RightMargin = 10.0f;
    float BottomMargin = 5.0f;
    float RowHeight = 20.0f;
    float BarPadding = 4.0f;

    FLinearColor BackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);
    FLinearColor GridLineColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.5f);
    FLinearColor TextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
    FLinearColor PendingColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);
    FLinearColor TooltipBgColor = FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);
    FLinearColor TooltipTextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
    FLinearColor HoverHighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);

    void BuildUI();
    void RefreshRenderData(const FGeometry& AllottedGeometry);
    void ApplyModelState();

    void DrawGantt(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    FLinearColor GetStatusColor(ESkillExecutionStatus Status) const;
    int32 FindSkillBarAtPosition(const FVector2D& LocalPosition) const;
};
