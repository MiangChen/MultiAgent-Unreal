// MAEmergencyModal.h
// 紧急事件模态窗口 - 用于显示和处理紧急情况
// 继承自 MABaseModalWidget，提供紧急事件详情显示和用户干预
// Requirements: 6.2, 6.3, 12.3, 12.4, 12.5

#pragma once

#include "CoreMinimal.h"
#include "MABaseModalWidget.h"
#include "MAEmergencyModal.generated.h"

class AMACharacter;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UEditableTextBox;
class UMultiLineEditableTextBox;
class UImage;
class UTextureRenderTarget2D;

//=============================================================================
// 紧急事件数据结构
//=============================================================================

/**
 * 紧急事件数据
 * 存储紧急事件的详细信息
 */
USTRUCT(BlueprintType)
struct FMAEmergencyEventData
{
    GENERATED_BODY()

    /** 来源 Agent ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString SourceAgentId;

    /** 来源 Agent 名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString SourceAgentName;

    /** 事件类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString EventType;

    /** 事件时间戳 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FDateTime Timestamp;

    /** 事件描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString Description;

    /** 事件位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FVector Location;

    /** 用户干预响应 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString UserResponse;

    FMAEmergencyEventData()
        : Timestamp(FDateTime::Now())
        , Location(FVector::ZeroVector)
    {
    }
};

//=============================================================================
// 委托声明
//=============================================================================

/** 紧急事件确认提交委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmergencyConfirmed, const FMAEmergencyEventData&, Data);

/** 紧急事件拒绝委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEmergencyRejected);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAEmergencyModal, Log, All);

//=============================================================================
// UMAEmergencyModal - 紧急事件模态窗口
//=============================================================================

/**
 * 紧急事件模态窗口
 * 
 * 功能:
 * - 显示紧急事件详情：source agent, event type, timestamp
 * - 提供可编辑字段用于用户干预
 * - 直接进入编辑模式（不同于其他模态的只读→编辑流程）
 * - 提供 Confirm/Reject 按钮操作
 * 
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                     Emergency Event                                  │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │ Event Details                                                │   │
 * │  │ ┌─────────────────────────────────────────────────────────┐ │   │
 * │  │ │ Source Agent: [Agent Name]                              │ │   │
 * │  │ │ Event Type:   [Emergency Type]                          │ │   │
 * │  │ │ Timestamp:    [2025-01-17 10:30:45]                     │ │   │
 * │  │ │ Location:     [X: 100, Y: 200, Z: 50]                   │ │   │
 * │  │ └─────────────────────────────────────────────────────────┘ │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │ Description                                                  │   │
 * │  │ [Event description text...]                                  │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │ User Response (Editable)                                     │   │
 * │  │ [Enter your intervention response here...]                   │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │                    [Submit] [Reject]                                │
 * └─────────────────────────────────────────────────────────────────────┘
 * 
 * Requirements: 6.2, 6.3, 12.3, 12.4, 12.5
 */
UCLASS()
class MULTIAGENT_API UMAEmergencyModal : public UMABaseModalWidget
{
    GENERATED_BODY()

public:
    UMAEmergencyModal(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 加载紧急事件数据
     * @param Data 紧急事件数据
     */
    UFUNCTION(BlueprintCallable, Category = "EmergencyModal")
    void LoadEmergencyEvent(const FMAEmergencyEventData& Data);

    /**
     * 从 Agent 加载紧急事件
     * @param SourceAgent 触发事件的 Agent
     * @param InEventType 事件类型
     * @param InDescription 事件描述
     */
    UFUNCTION(BlueprintCallable, Category = "EmergencyModal")
    void LoadFromAgent(AMACharacter* SourceAgent, const FString& InEventType, const FString& InDescription);

    /**
     * 获取当前紧急事件数据
     * @return 当前紧急事件数据（包含用户响应）
     */
    UFUNCTION(BlueprintPure, Category = "EmergencyModal")
    FMAEmergencyEventData GetEmergencyEventData() const;

    /**
     * 获取用户响应文本
     * @return 用户输入的响应文本
     */
    UFUNCTION(BlueprintPure, Category = "EmergencyModal")
    FString GetUserResponse() const;

    /**
     * 设置用户响应文本
     * @param Response 响应文本
     */
    UFUNCTION(BlueprintCallable, Category = "EmergencyModal")
    void SetUserResponse(const FString& Response);

    /**
     * 获取响应 JSON 字符串
     * @return 紧急事件响应的 JSON 表示
     */
    UFUNCTION(BlueprintPure, Category = "EmergencyModal")
    FString GetResponseJson() const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 紧急事件确认提交委托 */
    UPROPERTY(BlueprintAssignable, Category = "EmergencyModal|Events")
    FOnEmergencyConfirmed OnEmergencyConfirmed;

    /** 紧急事件拒绝委托 */
    UPROPERTY(BlueprintAssignable, Category = "EmergencyModal|Events")
    FOnEmergencyRejected OnEmergencyRejected;

protected:
    //=========================================================================
    // UMABaseModalWidget 重写
    //=========================================================================

    /** 编辑模式变化回调 */
    virtual void OnEditModeChanged(bool bEditable) override;

    /** 获取模态标题 */
    virtual FText GetModalTitleText() const override;

    /** 构建内容区域 */
    virtual void BuildContentArea(UVerticalBox* InContentContainer) override;

    /** 主题应用后回调 */
    virtual void OnThemeApplied() override;

    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 创建事件详情区域 */
    UBorder* CreateEventDetailsArea();

    /** 创建描述区域 */
    UBorder* CreateDescriptionArea();

    /** 创建用户响应区域 */
    UBorder* CreateUserResponseArea();

    /** 更新事件详情显示 */
    void UpdateEventDetailsDisplay();

    /** 处理确认按钮点击 */
    UFUNCTION()
    void HandleConfirm();

    /** 处理拒绝按钮点击 */
    UFUNCTION()
    void HandleReject();

    /** 创建详情行 */
    UHorizontalBox* CreateDetailRow(const FString& Label, UTextBlock*& OutValueText);

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 事件详情容器 */
    UPROPERTY()
    UBorder* EventDetailsContainer;

    /** 来源 Agent 文本 */
    UPROPERTY()
    UTextBlock* SourceAgentText;

    /** 事件类型文本 */
    UPROPERTY()
    UTextBlock* EventTypeText;

    /** 时间戳文本 */
    UPROPERTY()
    UTextBlock* TimestampText;

    /** 位置文本 */
    UPROPERTY()
    UTextBlock* LocationText;

    /** 描述容器 */
    UPROPERTY()
    UBorder* DescriptionContainer;

    /** 描述文本 */
    UPROPERTY()
    UTextBlock* DescriptionText;

    /** 用户响应容器 */
    UPROPERTY()
    UBorder* UserResponseContainer;

    /** 用户响应标签 */
    UPROPERTY()
    UTextBlock* UserResponseLabel;

    /** 用户响应输入框 */
    UPROPERTY()
    UMultiLineEditableTextBox* UserResponseInput;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 当前紧急事件数据 */
    FMAEmergencyEventData EventData;

    /** 来源 Agent 引用 */
    TWeakObjectPtr<AMACharacter> SourceAgentRef;
};
