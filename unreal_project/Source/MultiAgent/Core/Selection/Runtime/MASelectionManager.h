// MASelectionManager.h
// 选择管理器 - 星际争霸风格的单位选择和编组系统
// 
// 功能:
// - 鼠标框选 Agent
// - Ctrl+1~9 保存编组
// - 按 1~9 选中编组
// - 双击 1~9 选中并移动镜头

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MASelectionManager.generated.h"

class AMACharacter;

// 编组数量
constexpr int32 MAX_CONTROL_GROUPS = 10;  // 0~9

// ========== 委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, const TArray<AMACharacter*>&, SelectedAgents);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnControlGroupChanged, int32, GroupIndex, const TArray<AMACharacter*>&, Agents);

UCLASS()
class MULTIAGENT_API UMASelectionManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== 选择操作 ==========
    
    // 选中单个 Agent（清除之前的选择）
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void SelectAgent(AMACharacter* Agent);

    // 添加到选择（不清除之前的）
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void AddToSelection(AMACharacter* Agent);

    // 从选择中移除
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void RemoveFromSelection(AMACharacter* Agent);

    // 切换选择状态
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void ToggleSelection(AMACharacter* Agent);

    // 选中多个 Agent（框选结果）
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void SelectAgents(const TArray<AMACharacter*>& Agents);

    // 添加多个到选择
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void AddAgentsToSelection(const TArray<AMACharacter*>& Agents);

    // 清除选择
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void ClearSelection();

    // 全选
    UFUNCTION(BlueprintCallable, Category = "Selection")
    void SelectAll();

    // ========== 查询 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Selection")
    TArray<AMACharacter*> GetSelectedAgents() const { return SelectedAgents; }

    UFUNCTION(BlueprintCallable, Category = "Selection")
    int32 GetSelectedCount() const { return SelectedAgents.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Selection")
    bool IsSelected(AMACharacter* Agent) const;

    UFUNCTION(BlueprintCallable, Category = "Selection")
    bool HasSelection() const { return SelectedAgents.Num() > 0; }

    // ========== 编组操作 (Ctrl+1~9) ==========
    
    // 保存当前选择到编组
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    void SaveToControlGroup(int32 GroupIndex);

    // 选中编组
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    void SelectControlGroup(int32 GroupIndex);

    // 添加编组到当前选择（Shift+数字）
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    void AddControlGroupToSelection(int32 GroupIndex);

    // 获取编组内容
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    TArray<AMACharacter*> GetControlGroup(int32 GroupIndex) const;

    // 清除编组
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    void ClearControlGroup(int32 GroupIndex);

    // 编组是否为空
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    bool IsControlGroupEmpty(int32 GroupIndex) const;

    // 获取编组中心位置（用于镜头移动）
    UFUNCTION(BlueprintCallable, Category = "Selection|ControlGroup")
    FVector GetControlGroupCenter(int32 GroupIndex) const;

    // 获取选择中心位置
    UFUNCTION(BlueprintCallable, Category = "Selection")
    FVector GetSelectionCenter() const;

    // ========== 框选状态 ==========
    
    // 开始框选
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    void BeginBoxSelect(const FVector2D& ScreenPos);

    // 更新框选终点
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    void UpdateBoxSelect(const FVector2D& ScreenPos);

    // 结束框选并选中框内 Agent
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    void EndBoxSelect(APlayerController* PC, bool bAddToSelection = false);

    // 取消框选
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    void CancelBoxSelect();

    // 是否正在框选
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    bool IsBoxSelecting() const { return bIsBoxSelecting; }

    // 获取框选起点
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    FVector2D GetBoxSelectStart() const { return BoxSelectStart; }

    // 获取框选终点
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    FVector2D GetBoxSelectEnd() const { return BoxSelectEnd; }

    // 获取屏幕矩形内的所有 Agent
    UFUNCTION(BlueprintCallable, Category = "Selection|BoxSelect")
    TArray<AMACharacter*> GetAgentsInScreenRect(const FVector2D& StartPos, const FVector2D& EndPos, APlayerController* PC) const;

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Selection")
    FOnSelectionChanged OnSelectionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Selection")
    FOnControlGroupChanged OnControlGroupChanged;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // 当前选中的 Agent
    UPROPERTY()
    TArray<AMACharacter*> SelectedAgents;

    // 编组 0~9 (不使用 UPROPERTY，因为 UE 不支持嵌套容器的反射)
    TMap<int32, TArray<AMACharacter*>> ControlGroupsMap;

    // 框选状态
    bool bIsBoxSelecting = false;
    FVector2D BoxSelectStart;
    FVector2D BoxSelectEnd;

    // 清理无效引用
    void CleanupInvalidAgents(TArray<AMACharacter*>& Agents);

    // 更新 Agent 的选中状态视觉效果
    void UpdateAgentSelectionVisual(AMACharacter* Agent, bool bSelected);

    // 广播选择变化
    void BroadcastSelectionChanged();
};
