// MAEmergencyModal.h
// 紧急事件模态窗口 - 用于显示和处理紧急情况
// 继承自 MABaseModalWidget，提供紧急事件详情显示和用户干预
// Requirements: 6.2, 6.3, 12.3, 12.4, 12.5

#pragma once

#include "CoreMinimal.h"
#include "MABaseModalWidget.h"
#include "Dom/JsonObject.h"
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
class UMACameraSensorComponent;
class USizeBox;
class UMAStyledButton;

//=============================================================================
// 紧急事件数据结构
//=============================================================================

/**
 * 紧急事件数据
 * 存储紧急事件的详细信息
 * Requirements: 1.1, 1.2, 1.3, 1.4
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAEmergencyEventData
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

    /** 可供执行的选项列表 - Requirements: 1.2 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    TArray<FString> AvailableOptions;

    /** 用户选择的选项 - Requirements: 6.1, 6.5 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString SelectedOption;

    /** 用户输入的文本 - Requirements: 7.5 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emergency")
    FString UserInputText;

    FMAEmergencyEventData()
        : Timestamp(FDateTime::Now())
        , Location(FVector::ZeroVector)
    {
    }

    //=========================================================================
    // 验证和默认值方法 - Requirements: 1.3, 1.4
    //=========================================================================

    /**
     * 检查事件数据是否有效
     * @return true 如果所有必需字段都有效
     */
    bool IsValid() const;

    /**
     * 为缺失字段应用默认值
     * Requirements: 1.4
     */
    void ApplyDefaults();

    //=========================================================================
    // 序列化方法 - Requirements: 2.2, 2.4
    //=========================================================================

    /**
     * 将事件数据序列化为 JSON 字符串
     * @return JSON 格式的字符串
     */
    FString ToJson() const;

    /**
     * 从 JSON 字符串解析事件数据
     * @param JsonString JSON 格式的字符串
     * @param OutData 输出的事件数据
     * @return true 如果解析成功
     */
    static bool FromJson(const FString& JsonString, FMAEmergencyEventData& OutData);

    /**
     * 从 JSON 对象解析事件数据
     * @param JsonObject JSON 对象
     * @param OutData 输出的事件数据
     * @return true 如果解析成功
     */
    static bool FromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FMAEmergencyEventData& OutData);
};

//=============================================================================
// 委托声明
//=============================================================================

/** 紧急事件确认提交委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmergencyConfirmed, const FMAEmergencyEventData&, Data);

/** 紧急事件拒绝委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEmergencyRejected);

/** 紧急事件选项按钮点击委托 - 用于发送 button_event */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEmergencyOptionClicked, const FString&, OptionText, const FMAEmergencyEventData&, Data);

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
    // 相机源管理
    //=========================================================================

    /**
     * 设置相机源，连接到渲染目标
     * @param Camera 相机传感器组件
     */
    UFUNCTION(BlueprintCallable, Category = "EmergencyModal|Camera")
    void SetCameraSource(UMACameraSensorComponent* Camera);

    /**
     * 清除相机源，显示黑屏
     */
    UFUNCTION(BlueprintCallable, Category = "EmergencyModal|Camera")
    void ClearCameraSource();

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

    /** 创建选项按钮区域 - Requirements: 5.3, 6.1 */
    UBorder* CreateOptionsArea();

    /** 创建用户响应区域 */
    UBorder* CreateUserResponseArea();

    /** 创建相机视图区域 */
    UBorder* CreateCameraViewArea();

    /** 更新事件详情显示 */
    void UpdateEventDetailsDisplay();

    /** 更新选项按钮显示 - Requirements: 5.3 */
    void UpdateOptionsDisplay();

    /** 选项按钮点击处理 - Requirements: 6.1 */
    void OnOptionButtonClicked(int32 OptionIndex);

    /** 更新选项按钮样式 - 高亮选中按钮 */
    void UpdateOptionButtonStyles();

    /** 处理确认按钮点击 */
    UFUNCTION()
    void HandleConfirm();

    /** 处理拒绝按钮点击 */
    UFUNCTION()
    void HandleReject();

    /** 处理选项按钮点击 - Requirements: 6.1 */
    UFUNCTION()
    void HandleOptionButtonClicked();

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

    /** 选项按钮容器 - Requirements: 5.3 */
    UPROPERTY()
    UBorder* OptionsContainer;

    /** 选项按钮垂直布局 */
    UPROPERTY()
    UVerticalBox* OptionsVBox;

    /** 选项按钮数组 - Requirements: 6.1 - 使用 UMAStyledButton 统一样式 */
    UPROPERTY()
    TArray<UMAStyledButton*> OptionButtons;

    /** 选项按钮边框数组 - 用于显示选中状态 */
    UPROPERTY()
    TArray<UBorder*> OptionButtonBorders;

    /** 选项按钮文本数组 */
    UPROPERTY()
    TArray<UTextBlock*> OptionButtonTexts;

    /** 当前选中的选项索引 - Requirements: 6.1 */
    int32 SelectedOptionIndex = -1;

    //=========================================================================
    // 相机视图组件
    //=========================================================================

    /** 相机视图容器 */
    UPROPERTY()
    UBorder* CameraViewContainer;

    /** 相机画面显示 */
    UPROPERTY()
    UImage* CameraFeedImage;

    /** 当前相机源 */
    UPROPERTY()
    TWeakObjectPtr<UMACameraSensorComponent> CurrentCameraSource;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 当前紧急事件数据 */
    FMAEmergencyEventData EventData;

    /** 来源 Agent 引用 */
    TWeakObjectPtr<AMACharacter> SourceAgentRef;
};
