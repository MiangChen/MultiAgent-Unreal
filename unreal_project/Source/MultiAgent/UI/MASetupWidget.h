// MASetupWidget.h
// 游戏启动前的配置界面 - 配置小队成员、选择场景
// 类似 RTS 游戏的"部署阶段"

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MASetupWidget.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UButton;
class UTextBlock;
class UComboBoxString;
class UScrollBox;
class UEditableTextBox;

/**
 * 待部署的智能体配置
 */
USTRUCT(BlueprintType)
struct FMAAgentSetupConfig
{
    GENERATED_BODY()

    /** 智能体类型: DronePhantom4, DroneInspire2, RobotDog, Human */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AgentType;

    /** 显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    /** 数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 1;

    FMAAgentSetupConfig() : AgentType(TEXT("DronePhantom4")), DisplayName(TEXT("Phantom 4")), Count(1) {}
    FMAAgentSetupConfig(const FString& InType, const FString& InName, int32 InCount = 1)
        : AgentType(InType), DisplayName(InName), Count(InCount) {}
};

/**
 * 开始仿真委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartSimulation);

/**
 * 配置界面 Widget
 * 
 * 功能:
 * - 选择场景地图
 * - 添加/删除智能体
 * - 配置智能体数量
 * - 开始仿真
 */
UCLASS()
class MULTIAGENT_API UMASetupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASetupWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 开始仿真委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStartSimulation OnStartSimulation;

    //=========================================================================
    // 公共接口
    //=========================================================================

    /** 获取当前配置的智能体列表 */
    UFUNCTION(BlueprintPure, Category = "Setup")
    TArray<FMAAgentSetupConfig> GetAgentConfigs() const { return AgentConfigs; }

    /** 获取选择的场景名称 */
    UFUNCTION(BlueprintPure, Category = "Setup")
    FString GetSelectedScene() const { return SelectedScene; }

    /** 获取配置的智能体总数 */
    UFUNCTION(BlueprintPure, Category = "Setup")
    int32 GetTotalAgentCount() const;

protected:
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 控件
    //=========================================================================

    /** 场景选择下拉框 */
    UPROPERTY()
    UComboBoxString* SceneComboBox;

    /** 智能体类型选择下拉框 */
    UPROPERTY()
    UComboBoxString* AgentTypeComboBox;

    /** 数量输入框 */
    UPROPERTY()
    UEditableTextBox* CountInputBox;

    /** 添加按钮 */
    UPROPERTY()
    UButton* AddButton;

    /** 清空按钮 */
    UPROPERTY()
    UButton* ClearButton;

    /** 开始按钮 */
    UPROPERTY()
    UButton* StartButton;

    /** 智能体列表容器 */
    UPROPERTY()
    UScrollBox* AgentListScrollBox;

    /** 总数显示 */
    UPROPERTY()
    UTextBlock* TotalCountText;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 当前配置的智能体列表 */
    UPROPERTY()
    TArray<FMAAgentSetupConfig> AgentConfigs;

    /** 选择的场景 */
    UPROPERTY()
    FString SelectedScene;

    /** 可用的智能体类型 */
    TMap<FString, FString> AvailableAgentTypes;

    /** 可用的场景列表 */
    TArray<FString> AvailableScenes;

    /** 删除按钮到索引的映射 */
    UPROPERTY()
    TMap<UButton*, int32> RemoveButtonIndexMap;

    /** 减号按钮到索引的映射 */
    UPROPERTY()
    TMap<UButton*, int32> DecreaseButtonIndexMap;

    /** 加号按钮到索引的映射 */
    UPROPERTY()
    TMap<UButton*, int32> IncreaseButtonIndexMap;

private:
    //=========================================================================
    // UI 构建
    //=========================================================================

    void BuildUI();
    void InitializeData();
    void RefreshAgentList();
    void UpdateTotalCount();

    //=========================================================================
    // 事件处理
    //=========================================================================

    UFUNCTION()
    void OnSceneSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnAddButtonClicked();

    UFUNCTION()
    void OnClearButtonClicked();

    UFUNCTION()
    void OnStartButtonClicked();

    void OnRemoveAgentClicked(int32 Index);

    /** 删除按钮点击回调 (通过映射查找索引) */
    UFUNCTION()
    void OnRemoveButtonClicked();

    /** 减号按钮点击回调 - 减少数量 */
    UFUNCTION()
    void OnDecreaseButtonClicked();

    /** 减少指定索引的 Agent 数量 */
    void OnDecreaseAgentCount(int32 Index);

    /** 加号按钮点击回调 - 增加数量 */
    UFUNCTION()
    void OnIncreaseButtonClicked();

    /** 增加指定索引的 Agent 数量 */
    void OnIncreaseAgentCount(int32 Index);
};
