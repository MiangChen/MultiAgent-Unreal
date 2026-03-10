// MASelectionHUD.h
// 选择框 HUD - 绘制框选矩形和选中 Agent 的高亮圈

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "../../Input/MAPlayerController.h"
#include "MASelectionHUD.generated.h"

class AMACharacter;
class UMAUITheme;
class FMASelectionHUDCoordinator;
struct FMASelectionHUDCircleEntry;
struct FMASelectionHUDControlGroupEntry;
struct FMASelectionHUDStatusTextEntry;

UCLASS()
class MULTIAGENT_API AMASelectionHUD : public AHUD
{
    GENERATED_BODY()

    friend class FMASelectionHUDCoordinator;

public:
    AMASelectionHUD();

    virtual void DrawHUD() override;

    //=========================================================================
    // 主题系统
    //=========================================================================

    /** 应用主题样式 */
    void ApplyTheme(UMAUITheme* InTheme);

    //=========================================================================
    // 框选状态设置 (由 PlayerController 调用)
    //=========================================================================

    /** 设置是否正在框选 */
    void SetBoxSelecting(bool bSelecting) { bIsBoxSelecting = bSelecting; }

    /** 设置框选起点 */
    void SetBoxStart(const FVector2D& Start) { BoxStart = Start; }

    /** 设置框选终点 */
    void SetBoxEnd(const FVector2D& End) { BoxEnd = End; }

    /** 切换 Agent 圆环高亮显示/隐藏 */
    void ToggleAgentCircles() { bShowAgentCircles = !bShowAgentCircles; }

    /** 获取 Agent 圆环是否显示 */
    bool IsAgentCirclesVisible() const { return bShowAgentCircles; }

protected:
    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    // ========== 框选状态 (由 PlayerController 设置) ==========
    
    UPROPERTY(BlueprintReadWrite, Category = "Selection")
    bool bIsBoxSelecting = false;

    // ========== Agent 圆环显示控制 ==========
    
    /** 是否显示 Agent 圆环高亮 ([ 键切换) */
    UPROPERTY(BlueprintReadWrite, Category = "Selection")
    bool bShowAgentCircles = true;

    UPROPERTY(BlueprintReadWrite, Category = "Selection")
    FVector2D BoxStart;

    UPROPERTY(BlueprintReadWrite, Category = "Selection")
    FVector2D BoxEnd;

    // ========== 样式设置 ==========
    
    // 框选矩形颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor BoxColor = FLinearColor(0.f, 1.f, 0.f, 0.3f);

    // 框选边框颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor BoxBorderColor = FLinearColor(0.f, 1.f, 0.f, 1.f);

    // 选中 Agent 圈颜色 (绿色)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor SelectionCircleColor = FLinearColor(0.f, 1.f, 0.f, 1.f);

    // 未选中 Agent 圈颜色 (橙色)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor UnselectedCircleColor = FLinearColor(1.f, 0.5f, 0.f, 1.f);

    // 圈线条粗细 (选中和未选中统一)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    float CircleThickness = 8.f;

    // 选中圈半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    float SelectionCircleRadius = 60.f;

    // 编组数字颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor ControlGroupTextColor = FLinearColor(1.f, 1.f, 0.f, 1.f);

    // 部署模式框选颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor DeploymentBoxColor = FLinearColor(0.2f, 0.6f, 1.f, 0.3f);

    // 部署模式边框颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor DeploymentBoxBorderColor = FLinearColor(0.2f, 0.6f, 1.f, 1.f);

private:
    // 绘制框选矩形
    void DrawSelectionBox(bool bIsDeploymentMode = false);

    // 绘制所有 Agent 的圆圈（选中/未选中使用不同颜色）
    void DrawAllAgentCircles(const TArray<FMASelectionHUDCircleEntry>& CircleEntries);

    // 绘制编组信息
    void DrawControlGroupInfo(const TArray<FMASelectionHUDControlGroupEntry>& ControlGroupEntries);

    // 在 Agent 脚下绘制圆圈
    void DrawCircleAtLocation(const FVector& WorldLocation, FLinearColor Color, float Radius);

    // 绘制当前鼠标模式
    void DrawMouseMode();

    // 绘制部署模式信息
    void DrawDeploymentInfo();

    // 检查是否有全屏 Widget 可见（通过 MAUIManager）
    bool IsAnyFullscreenWidgetVisible() const;

    bool ShouldDrawMouseMode() const;

    bool RuntimeIsAnyFullscreenWidgetVisible() const;
    bool RuntimeHasBlockingVisibleWidget() const;
    void RuntimeBuildCircleEntries(TArray<FMASelectionHUDCircleEntry>& OutEntries) const;
    void RuntimeBuildControlGroupEntries(TArray<FMASelectionHUDControlGroupEntry>& OutEntries) const;
    void RuntimeBuildStatusTextEntries(TArray<FMASelectionHUDStatusTextEntry>& OutEntries) const;

    // 获取模式颜色（带 fallback 逻辑）
    FLinearColor GetModeColor(EMAMouseMode Mode) const;

    // 绘制所有 Agent 的状态文字
    void DrawAllAgentStatusText(const TArray<FMASelectionHUDStatusTextEntry>& StatusEntries);
};
