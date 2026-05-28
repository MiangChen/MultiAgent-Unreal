// MASystemLogPanel.h
// 系统日志面板 Widget - 独立的日志显示面板
// 从原 MARightSidebarWidget 拆分出来，可通过按键独立切换显示/隐藏

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MASystemLogPanelCoordinator.h"
#include "MASystemLogPanel.generated.h"

class UVerticalBox;
class UScrollBox;
class UBorder;
class UTextBlock;
class USizeBox;
class UMAUITheme;

// 日志类别声明
DECLARE_LOG_CATEGORY_EXTERN(LogMASystemLogPanel, Log, All);

//=============================================================================
// 日志条目结构体
//=============================================================================

/**
 * 日志条目结构体
 * 用于存储单条日志的信息
 */
USTRUCT(BlueprintType)
struct FMALogEntry
{
    GENERATED_BODY()

    /** 日志消息内容 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    FString Message;

    /** 是否为错误消息 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    bool bIsError = false;

    /** 日志时间戳 */
    UPROPERTY(BlueprintReadOnly, Category = "Log")
    FDateTime Timestamp;

    FMALogEntry()
        : Message(TEXT(""))
        , bIsError(false)
        , Timestamp(FDateTime::Now())
    {
    }

    FMALogEntry(const FString& InMessage, bool InIsError = false)
        : Message(InMessage)
        , bIsError(InIsError)
        , Timestamp(FDateTime::Now())
    {
    }
};

/**
 * 系统日志面板 Widget
 * 
 * 独立的日志显示面板，从 MARightSidebarWidget 拆分而来。
 * 可通过按键 6 独立切换显示/隐藏状态。
 * 
 * 功能：
 * - 显示系统运行日志信息
 * - 支持追加日志、清空日志
 * - 自动滚动到最新日志
 * - 支持主题切换
 * 
 */
UCLASS()
class MULTIAGENT_API UMASystemLogPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASystemLogPanel(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 日志方法
    //=========================================================================

    /**
     * 追加日志消息
     * @param Message 日志消息
     * @param bIsError 是否为错误消息（错误消息会高亮显示）
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Log")
    void AppendLog(const FString& Message, bool bIsError = false);

    /**
     * 清空所有日志
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Log")
    void ClearLogs();

    /**
     * 获取日志条目数量
     * @return 日志条目数量
     */
    UFUNCTION(BlueprintPure, Category = "Panel|Log")
    int32 GetLogCount() const { return LogEntries.Num(); }

    //=========================================================================
    // 主题支持
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

    /** 面板宽度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200.0", ClampMax = "800.0"))
    float PanelWidth = 480.0f;

    /** 面板高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "50.0", ClampMax = "400.0"))
    float PanelHeight = 150.0f;

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

    /** 根 SizeBox - 控制面板尺寸 */
    UPROPERTY()
    USizeBox* RootSizeBox;

    /** 面板背景边框 */
    UPROPERTY()
    UBorder* PanelBackground;

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

    /** 日志协调器 */
    FMASystemLogPanelCoordinator Coordinator;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建日志区 */
    UWidget* CreateLogSection();

    /** 刷新日志显示 */
    void RefreshLogDisplay();

    /** 创建日志条目 Widget */
    UWidget* CreateLogEntryWidget(const FMALogEntry& Entry);

    /** 滚动到日志底部 */
    void ScrollToLogBottom();
};
