// MASelectionHUD.h
// 选择框 HUD - 绘制框选矩形和选中 Agent 的高亮圈

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MASelectionHUD.generated.h"

class AMACharacter;

UCLASS()
class MULTIAGENT_API AMASelectionHUD : public AHUD
{
    GENERATED_BODY()

public:
    AMASelectionHUD();

    virtual void DrawHUD() override;

    // ========== 框选状态 (由 PlayerController 设置) ==========
    
    UPROPERTY(BlueprintReadWrite, Category = "Selection")
    bool bIsBoxSelecting = false;

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

    // 选中圈颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor SelectionCircleColor = FLinearColor(0.f, 1.f, 0.f, 1.f);

    // 选中圈半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    float SelectionCircleRadius = 50.f;

    // 编组数字颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Style")
    FLinearColor ControlGroupTextColor = FLinearColor(1.f, 1.f, 0.f, 1.f);

private:
    // 绘制框选矩形
    void DrawSelectionBox();

    // 绘制选中 Agent 的高亮圈
    void DrawSelectedAgents();

    // 绘制编组信息
    void DrawControlGroupInfo();

    // 在 Agent 脚下绘制圆圈
    void DrawCircleAtAgent(AMACharacter* Agent, FLinearColor Color, float Radius);

    // 绘制当前鼠标模式
    void DrawMouseMode();
};
