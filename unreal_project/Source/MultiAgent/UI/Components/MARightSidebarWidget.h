// MARightSidebarWidget.h
// 右侧边栏 Widget - 包含命令输入、任务图预览、技能列表预览和系统日志
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MARightSidebarWidget.generated.h"

class UVerticalBox;
class UEditableTextBox;
class UMultiLineEditableTextBox;
class UScrollBox;
class UBorder;
class UTextBlock;
class USizeBox;
class UMAStyledButton;
class UMAUITheme;
class UMATaskGraphPreview;
class UMASkillListPreview;

//=============================================================================
// 委托声明
//=============================================================================

/** 命令提交委托 - 当用户提交命令时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandSubmitted, const FString&, Command);

//=============================================================================
// 日志条目数据结构
//=============================================================================

/**
 * 日志条目数据
 * 存储单条日志的信息
 */
USTRUCT(BlueprintType)
struct FMALogEntry
{
    GENERATED_BODY()

    /** 日志消息 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    FString Message;

    /** 是否为错误消息 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    bool bIsError = false;

    /** 时间戳 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    FDateTime Timestamp;

    FMALogEntry()
        : Timestamp(FDateTime::Now())
    {
    }

    FMALogEntry(const FString& InMessage, bool bInIsError = false)
        : Message(InMessage)
        , bIsError(bInIsError)
        , Timestamp(FDateTime::Now())
    {
    }
};

//=============================================================================
// MARightSidebarWidget 类
//=============================================================================

/**
 * 右侧边栏 Widget
 * 
 * 包含以下组件：
 * - 命令输入区：文本输入框 + 提交按钮
 * - 任务图预览：紧凑的任务图可视化
 * - 技能列表预览：紧凑的技能分配可视化
 * - 系统日志：可滚动的日志消息列表
 * 
 * Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6
 */
UCLASS()
class MULTIAGENT_API UMARightSidebarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMARightSidebarWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 命令提交事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCommandSubmitted OnCommandSubmitted;

    //=========================================================================
    // 预览更新方法
    //=========================================================================

    /**
     * 更新任务图预览
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Preview")
    void UpdateTaskGraphPreview(const FMATaskGraphData& Data);

    /**
     * 更新技能列表预览
     * @param Data 技能分配数据
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Preview")
    void UpdateSkillListPreview(const FMASkillAllocationData& Data);

    //=========================================================================
    // 日志方法
    //=========================================================================

    /**
     * 追加日志消息
     * @param Message 日志消息
     * @param bIsError 是否为错误消息（错误消息会高亮显示）
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Log")
    void AppendLog(const FString& Message, bool bIsError = false);

    /**
     * 清空所有日志
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Log")
    void ClearLogs();

    /**
     * 获取日志条目数量
     * @return 日志条目数量
     */
    UFUNCTION(BlueprintPure, Category = "Sidebar|Log")
    int32 GetLogCount() const { return LogEntries.Num(); }

    //=========================================================================
    // 命令输入方法
    //=========================================================================

    /**
     * 获取当前输入的命令文本
     * @return 命令文本
     */
    UFUNCTION(BlueprintPure, Category = "Sidebar|Command")
    FString GetCommandText() const;

    /**
     * 设置命令输入文本
     * @param Text 命令文本
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Command")
    void SetCommandText(const FString& Text);

    /**
     * 清空命令输入
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Command")
    void ClearCommandInput();

    /**
     * 聚焦到命令输入框
     */
    UFUNCTION(BlueprintCallable, Category = "Sidebar|Command")
    void FocusCommandInput();

    //=========================================================================
    // 主题应用
    //=========================================================================

    /**
     * 应用主题
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /**
     * 获取当前主题
     * @return 当前主题
     */
    UFUNCTION(BlueprintPure, Category = "Theme")
    UMAUITheme* GetTheme() const { return Theme; }

    //=========================================================================
    // 组件访问
    //=========================================================================

    /**
     * 获取任务图预览组件
     * @return 任务图预览组件
     */
    UFUNCTION(BlueprintPure, Category = "Sidebar|Components")
    UMATaskGraphPreview* GetTaskGraphPreview() const { return TaskGraphPreview; }

    /**
     * 获取技能列表预览组件
     * @return 技能列表预览组件
     */
    UFUNCTION(BlueprintPure, Category = "Sidebar|Components")
    UMASkillListPreview* GetSkillListPreview() const { return SkillListPreview; }

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 边栏宽度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200.0", ClampMax = "800.0"))
    float SidebarWidth = 480.0f;

    /** 日志区高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "50.0", ClampMax = "300.0"))
    float LogSectionHeight = 150.0f;

    /** 最大日志条目数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log", meta = (ClampMin = "10", ClampMax = "500"))
    int32 MaxLogEntries = 100;

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 SizeBox - 控制边栏宽度 */
    UPROPERTY()
    USizeBox* RootSizeBox;

    /** 边栏背景边框 */
    UPROPERTY()
    UBorder* SidebarBackground;

    /** 边栏容器 (VerticalBox) */
    UPROPERTY()
    UVerticalBox* SidebarContainer;

    //--- 命令输入区 ---

    /** 命令输入区边框 */
    UPROPERTY()
    UBorder* CommandSectionBorder;

    /** 命令输入框 (多行) */
    UPROPERTY()
    UMultiLineEditableTextBox* CommandInput;

    /** 提交按钮 */
    UPROPERTY()
    UMAStyledButton* SubmitButton;

    //--- 任务图预览区 ---

    /** 任务图预览区边框 */
    UPROPERTY()
    UBorder* TaskGraphSectionBorder;

    /** 任务图预览标题 */
    UPROPERTY()
    UTextBlock* TaskGraphTitle;

    /** 任务图预览组件 */
    UPROPERTY()
    UMATaskGraphPreview* TaskGraphPreview;

    //--- 技能列表预览区 ---

    /** 技能列表预览区边框 */
    UPROPERTY()
    UBorder* SkillListSectionBorder;

    /** 技能列表预览标题 */
    UPROPERTY()
    UTextBlock* SkillListTitle;

    /** 技能列表预览组件 */
    UPROPERTY()
    UMASkillListPreview* SkillListPreview;

    //--- 系统日志区 ---

    /** 日志区边框 */
    UPROPERTY()
    UBorder* LogSectionBorder;

    /** 日志标题 */
    UPROPERTY()
    UTextBlock* LogTitle;

    /** 日志滚动框 */
    UPROPERTY()
    UScrollBox* LogScrollBox;

    /** 日志容器 */
    UPROPERTY()
    UVerticalBox* LogContainer;

    //=========================================================================
    // 状态
    //=========================================================================

    /** 日志条目列表 */
    TArray<FMALogEntry> LogEntries;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建命令输入区 */
    UWidget* CreateCommandSection();

    /** 创建任务图预览区 */
    UWidget* CreateTaskGraphSection();

    /** 创建技能列表预览区 */
    UWidget* CreateSkillListSection();

    /** 创建系统日志区 */
    UWidget* CreateLogSection();

    /** 创建分隔线 */
    UWidget* CreateSeparator();

    /** 刷新日志显示 */
    void RefreshLogDisplay();

    /** 创建日志条目 Widget */
    UWidget* CreateLogEntryWidget(const FMALogEntry& Entry);

    /** 滚动到日志底部 */
    void ScrollToLogBottom();

    //=========================================================================
    // 事件回调
    //=========================================================================

    /** 提交按钮点击回调 */
    UFUNCTION()
    void OnSubmitClicked();

    /** 命令输入框提交回调 (按 Enter 键) */
    UFUNCTION()
    void OnCommandInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);
};
