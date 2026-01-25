// MAModifyWidget.h
// Modify 模式修改面板 Widget - 纯 C++ 实现
// 用于查看和编辑选中 Actor 的标签信息
// 支持单选和多选模式

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAModifyTypes.h"
#include "MAModifyWidget.generated.h"

class UMultiLineEditableTextBox;
class UMultiLineEditableText;
class UButton;
class UTextBlock;
class UVerticalBox;
class UBorder;
class UScrollBox;
struct FMASceneGraphNode;

/**
 * 确认修改委托 (单选模式)
 * @param Actor 当前选中的 Actor
 * @param LabelText 文本框内容
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModifyConfirmed, AActor*, Actor, const FString&, LabelText);

/**
 * 确认修改委托 (多选模式)
 * @param Actors 当前选中的 Actor 数组
 * @param LabelText 文本框内容
 * @param GeneratedJson 生成的 JSON 字符串
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMultiSelectModifyConfirmed, const TArray<AActor*>&, Actors, const FString&, LabelText, const FString&, GeneratedJson);

/**
 * Modify 模式修改面板 Widget - 纯 C++ 实现
 * 
 * 功能:
 * - 显示选中 Actor 的标签信息
 * - 提供可编辑的多行文本框
 * - 确认按钮提交修改
 * - 支持多选模式 (Shift+Click)
 * 
 */
UCLASS()
class MULTIAGENT_API UMAModifyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAModifyWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口 - 单选模式 (向后兼容)
    //=========================================================================

    /**
     * 设置选中的 Actor (单选模式)
     * @param Actor 选中的 Actor，nullptr 表示清除选择
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetSelectedActor(AActor* Actor);

    /**
     * 清除选中状态
     * 显示提示文字 "点击场景中的 Actor 进行选择"
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClearSelection();

    /**
     * 获取文本框内容
     * @return 当前文本框中的文本
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetLabelText() const;

    /**
     * 设置文本框内容
     * @param Text 要设置的文本
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetLabelText(const FString& Text);

    /**
     * 获取当前选中的 Actor (单选模式)
     * @return 当前选中的 Actor，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    AActor* GetSelectedActor() const { return SelectedActors.Num() > 0 ? SelectedActors[0] : nullptr; }

    //=========================================================================
    // 公共接口 - 多选模式
    //=========================================================================

    /**
     * 设置多个选中的 Actor (多选模式)
     * @param Actors 选中的 Actor 数组
     */
    UFUNCTION(BlueprintCallable, Category = "UI|MultiSelect")
    void SetSelectedActors(const TArray<AActor*>& Actors);

    /**
     * 获取所有选中的 Actor
     * @return 选中的 Actor 数组
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    TArray<AActor*> GetSelectedActors() const { return SelectedActors; }

    /**
     * 是否处于多选模式
     * @return true 如果选中了多个 Actor
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    bool IsMultiSelectMode() const { return SelectedActors.Num() > 1; }

    /**
     * 获取选中的 Actor 数量
     * @return 选中的 Actor 数量
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    int32 GetSelectionCount() const { return SelectedActors.Num(); }

    //=========================================================================
    // ID 分配
    //=========================================================================

    /**
     * 获取下一个可用的节点 ID
     * 从配置管理器获取场景图路径，找到最大 ID 并返回 +1
     * @return 下一个可用的 ID 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "UI|ID")
    FString GetNextAvailableId();

    //=========================================================================
    // 选择验证
    //=========================================================================

    /**
     * 验证选择是否符合分类约束
     * - Building 类型: 仅允许单选 (SelectionCount == 1)
     * - TransFacility 类型: 允许任意数量
     * - Prop 类型: 允许任意数量
     * @param Category 节点分类
     * @param SelectionCount 选中的 Actor 数量
     * @param OutError 错误信息 (如果验证失败)
     * @return 验证是否通过
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Validation")
    bool ValidateSelectionForCategory(EMANodeCategory Category, int32 SelectionCount, FString& OutError);

    //=========================================================================
    // 默认属性
    //=========================================================================

    /**
     * 根据分类获取默认属性字段
     * - Building: category, status, visibility, wind_condition, congestion, is_fire, is_spill
     * - TransFacility: 同 Building
     * - Prop: category, is_abnormal
     * @param Category 节点分类
     * @return 默认属性的键值对映射
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Properties")
    TMap<FString, FString> GetDefaultPropertiesForCategory(EMANodeCategory Category);

    //=========================================================================
    // 输入解析
    //=========================================================================

    /**
     * 解析标注输入字符串 (旧格式)
     * @param Input 输入文本，格式: "id:值, type:值" 或 "id:值, type:值, shape:polygon/linestring"
     * @param OutResult 解析结果
     * @param OutError 错误信息
     * @return 解析是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Parse")
    bool ParseAnnotationInput(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError);

    /**
     * 解析新格式的标注输入 (cate:xxx,type:xxx)
     * 支持格式:
     * - "cate:building,type:xxx" - 自动分配 ID
     * - "cate:trans_facility,type:xxx,shape:polygon"
     * - "cate:prop,type:xxx,id:123" - 手动指定 ID
     * @param Input 输入文本
     * @param OutResult 解析结果
     * @param OutError 错误信息
     * @return 解析是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Parse")
    bool ParseAnnotationInputV2(const FString& Input, FMAAnnotationInput& OutResult, FString& OutError);

    //=========================================================================
    // JSON 生成
    //=========================================================================

    /**
     * 生成场景图节点 JSON
     * @param Input 解析后的标注输入
     * @param Actors 选中的 Actor 数组
     * @return 生成的 JSON 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "UI|JSON")
    FString GenerateSceneGraphNode(const FMAAnnotationInput& Input, const TArray<AActor*>& Actors);

    /**
     * 生成场景图节点 JSON V2 (支持新的形状类型)
     * 根据 Category 调用不同的几何计算方法:
     * - Building: ComputePrismFromActors → Prism JSON
     * - TransFacility: ComputeOBBFromActors → LineString + Vertices JSON
     * - Prop: ComputeCenterFromActors → Point JSON
     * @param Input 解析后的标注输入
     * @param Actors 选中的 Actor 数组
     * @return 生成的 JSON 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "UI|JSON")
    FString GenerateSceneGraphNodeV2(const FMAAnnotationInput& Input, const TArray<AActor*>& Actors);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /**
     * 确认修改委托 (单选模式)
     * 当用户点击确认按钮时广播
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModifyConfirmed OnModifyConfirmed;

    /**
     * 确认修改委托 (多选模式)
     * 当用户在多选模式下点击确认按钮时广播
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMultiSelectModifyConfirmed OnMultiSelectModifyConfirmed;

    //=========================================================================
    // 主题支持
    //=========================================================================

    /**
     * 应用主题样式
     * @param InTheme 要应用的主题
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void ApplyTheme(class UMAUITheme* InTheme);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    class UMAUITheme* Theme;

    //=========================================================================
    // UI 控件 (动态创建)
    //=========================================================================

    /** 多行可编辑文本框 - 显示和编辑 Actor 标签 */
    UPROPERTY()
    UMultiLineEditableTextBox* LabelTextBox;

    /** JSON 预览文本框 - 只读，显示 Actor 对应的 JSON 片段 */
    UPROPERTY()
    UTextBlock* JsonPreviewText;

    /** 确认按钮 */
    UPROPERTY()
    UButton* ConfirmButton;

    /** 提示文本 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 模式指示器文本 */
    UPROPERTY()
    UTextBlock* ModeIndicatorText;

    /** 当前标注模式 */
    UPROPERTY()
    EMAAnnotationMode CurrentAnnotationMode = EMAAnnotationMode::AddNew;

    /** 编辑模式下的节点 ID */
    UPROPERTY()
    FString EditingNodeId;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 当前选中的 Actor 数组 (支持多选) */
    UPROPERTY()
    TArray<AActor*> SelectedActors;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 确认按钮点击处理
     */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /**
     * 构建 UI 布局
     */
    void BuildUI();

    /**
     * 生成 Actor 标签文本（占位符实现）
     * @param Actor 要生成标签的 Actor
     * @return 格式化的标签文本 "Actor: [ActorName]\nLabel: [placeholder]"
     */
    FString GenerateActorLabel(AActor* Actor) const;

    /**
     * 更新 JSON 预览文本 (单选模式)
     * 显示选中 Actor 对应的 JSON 片段
     * @param Actor 选中的 Actor
     */
    void UpdateJsonPreview(AActor* Actor);

    /**
     * 格式化 JSON 预览并高亮显示选中 Actor 的 GUID
     * @param Node 场景图节点
     * @param ActorGuid 选中 Actor 的 GUID
     * @return 格式化后的 JSON 字符串
     */
    FString FormatJsonPreviewWithHighlight(const FMASceneGraphNode& Node, const FString& ActorGuid) const;

    /**
     * 根据分类获取对应的提示文本
     * @param Category 节点分类
     * @return 对应的提示文本
     */
    FString GetHintTextForCategory(EMANodeCategory Category) const;

    /**
     * 更新 JSON 预览文本 (多选模式)
     * 显示将要生成的 JSON 结构预览
     * @param Actors 选中的 Actor 数组
     */
    void UpdateJsonPreviewMultiSelect(const TArray<AActor*>& Actors);

    /**
     * 计算凸包顶点 (用于 Polygon 类型)
     * @param Actors 选中的 Actor 数组
     * @return 凸包顶点数组 (2D, 逆时针顺序)
     */
    TArray<FVector2D> ComputeConvexHull(const TArray<AActor*>& Actors);

    /**
     * 计算线串点序列 (用于 LineString 类型)
     * @param Actors 选中的 Actor 数组
     * @return 排序后的点序列 (2D)
     */
    TArray<FVector2D> ComputeLineString(const TArray<AActor*>& Actors);

    /**
     * 收集所有选中 Actor 的 GUID
     * @param Actors 选中的 Actor 数组
     * @return GUID 字符串数组
     */
    TArray<FString> CollectActorGuids(const TArray<AActor*>& Actors);

    /**
     * 设置标注模式
     * @param Mode 新模式
     * @param NodeId 编辑模式下的节点 ID
     */
    void SetAnnotationMode(EMAAnnotationMode Mode, const FString& NodeId);

    /**
     * 更新模式指示器 UI
     */
    void UpdateModeIndicator();

    /**
     * 从节点属性填充输入框
     * @param Node 要编辑的节点
     */
    void PopulateInputFromNode(const FMASceneGraphNode& Node);

    /**
     * 生成编辑节点的 JSON
     * 只更新 properties，保留原有的 shape 和 Guid
     * @param Input 解析后的输入
     * @param NodeId 要编辑的节点 ID
     * @return JSON 字符串
     */
    FString GenerateEditNodeJson(const FMAAnnotationInput& Input, const FString& NodeId);

    /**
     * 查找选中 Actor 对应的已标记节点
     * @param ActorGuid Actor 的 GUID
     * @param OutNodes 输出匹配的节点数组
     * @return 是否找到匹配节点
     */
    bool FindMatchingNodes(const FString& ActorGuid, TArray<FMASceneGraphNode>& OutNodes);
};
