// MAInstructionPanel.h
// 指令输入面板 Widget - 包含指令输入框和发送按钮
// 从 MARightSidebarWidget 拆分出来，可通过按键独立切换显示/隐藏

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MAInstructionPanelCoordinator.h"
#include "MAInstructionPanel.generated.h"

class UVerticalBox;
class UMultiLineEditableTextBox;
class UBorder;
class UTextBlock;
class USizeBox;
class UMAStyledButton;
class UMAUITheme;

// 日志类别声明
DECLARE_LOG_CATEGORY_EXTERN(LogMAInstructionPanel, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 指令提交委托 - 当用户提交指令时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInstructionSubmitted, const FString&, Command);

/**
 * 指令输入面板 Widget
 * 
 * 独立的指令输入面板，从 MARightSidebarWidget 拆分而来。
 * 可通过按键 8 独立切换显示/隐藏状态。
 * 
 * 功能：
 * - 提供多行指令输入框
 * - 提供发送按钮
 * - 支持指令提交事件
 * - 支持主题切换
 * 
 */
UCLASS()
class MULTIAGENT_API UMAInstructionPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAInstructionPanel(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 指令提交事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInstructionSubmitted OnCommandSubmitted;

    //=========================================================================
    // 命令输入方法
    //=========================================================================

    /**
     * 获取当前输入的命令文本
     * @return 命令文本
     */
    UFUNCTION(BlueprintPure, Category = "Panel|Command")
    FString GetCommandText() const;

    /**
     * 设置命令输入文本
     * @param Text 命令文本
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Command")
    void SetCommandText(const FString& Text);

    /**
     * 清空命令输入
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Command")
    void ClearCommandInput();

    /**
     * 聚焦到命令输入框
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Command")
    void FocusCommandInput();

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "100.0", ClampMax = "600.0"))
    float PanelHeight = 400.0f;

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

    /** 面板容器 */
    UPROPERTY()
    UVerticalBox* PanelContainer;

    /** 命令输入区边框 */
    UPROPERTY()
    UBorder* CommandSectionBorder;

    /** 命令区标题 */
    UPROPERTY()
    UTextBlock* CommandTitle;

    /** 命令输入框 (多行) */
    UPROPERTY()
    UMultiLineEditableTextBox* CommandInput;

    /** 命令输入框背景边框 */
    UPROPERTY()
    UBorder* CommandInputBorder;

    /** 提交按钮 */
    UPROPERTY()
    UMAStyledButton* SubmitButton;

    /** 指令提交协调器 */
    FMAInstructionPanelCoordinator Coordinator;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    //=========================================================================
    // 事件回调
    //=========================================================================

    /** 提交按钮点击回调 */
    UFUNCTION()
    void OnSubmitClicked();
};
