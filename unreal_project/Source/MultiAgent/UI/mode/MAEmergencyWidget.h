// MAEmergencyWidget.h
// Emergency Event Details Widget - Pure C++ Implementation
// Displays camera feed, info text, action buttons and input box

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAEmergencyWidget.generated.h"

class UImage;
class UMultiLineEditableTextBox;
class UButton;
class UCanvasPanel;
class UBorder;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMACameraSensorComponent;

/**
 * Button click delegate
 * @param ButtonIndex Button index (0=Expand Search Area, 1=Ignore and Return, 2=Switch to Firefighting)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionButtonClicked, int32, ButtonIndex);

/**
 * Message sent delegate
 * @param Message User input message content
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageSent, const FString&, Message);

/**
 * Emergency Event Details Widget
 * 
 * Layout:
 * - Center-left: Camera feed area (CameraFeedImage)
 * - Center-right: Info display area (InfoTextBox)
 * - Right side: Three action buttons (ActionButton1/2/3)
 * - Bottom: Input box and send button (InputTextBox, SendButton)
 * 
 */
UCLASS()
class MULTIAGENT_API UMAEmergencyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAEmergencyWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // Camera Source Management
    //=========================================================================

    /**
     * Set camera source, connect to render target
     * @param Camera Camera sensor component
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Camera")
    void SetCameraSource(UMACameraSensorComponent* Camera);

    /**
     * Clear camera source, show black screen
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Camera")
    void ClearCameraSource();

    //=========================================================================
    // Info Display
    //=========================================================================

    /**
     * Set info display text
     * @param Text Text to display
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void SetInfoText(const FString& Text);

    /**
     * Append info text (append mode)
     * @param Text Text to append
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void AppendInfoText(const FString& Text);

    /**
     * Clear info display
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void ClearInfoText();

    //=========================================================================
    // Input Control
    //=========================================================================

    /**
     * Set focus to input box
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Input")
    void FocusInputBox();

    /**
     * Clear input box content
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Input")
    void ClearInputBox();

    /**
     * Get current text from input box
     * @return Text content in input box
     */
    UFUNCTION(BlueprintPure, Category = "Emergency|Input")
    FString GetInputText() const;

    //=========================================================================
    // Event Delegates
    //=========================================================================

    /** Button click delegate */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnActionButtonClicked OnActionButtonClicked;

    /** Message sent delegate */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMessageSent OnMessageSent;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI Components (dynamically created)
    //=========================================================================

    /** Root panel */
    UPROPERTY()
    UCanvasPanel* RootCanvas;
    
    /** Background border */
    UPROPERTY()
    class UBorder* BackgroundBorder;

    /** Camera feed display */
    UPROPERTY()
    UImage* CameraFeedImage;

    /** Info display text box */
    UPROPERTY()
    UMultiLineEditableTextBox* InfoTextBox;

    /** Action button 1: Expand Search Area */
    UPROPERTY()
    UButton* ActionButton1;

    /** Action button 2: Ignore and Return */
    UPROPERTY()
    UButton* ActionButton2;

    /** Action button 3: Switch to Firefighting */
    UPROPERTY()
    UButton* ActionButton3;

    /** Input text box */
    UPROPERTY()
    UMultiLineEditableTextBox* InputTextBox;

    /** Send button */
    UPROPERTY()
    UButton* SendButton;

    //=========================================================================
    // Camera Render Resources
    //=========================================================================

    /** Render target */
    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    /** Camera material instance */
    UPROPERTY()
    UMaterialInstanceDynamic* CameraMaterial;

    /** Current camera source */
    UPROPERTY()
    TWeakObjectPtr<UMACameraSensorComponent> CurrentCameraSource;

private:
    //=========================================================================
    // Internal Methods
    //=========================================================================

    /**
     * Build UI layout
     */
    void BuildUI();

    /**
     * Create camera render resources
     */
    void CreateCameraRenderResources();

    /**
     * Bind button events
     */
    void BindButtonEvents();

    //=========================================================================
    // Event Handling
    //=========================================================================

    /** Action button 1 click handler */
    UFUNCTION()
    void OnActionButton1Clicked();

    /** Action button 2 click handler */
    UFUNCTION()
    void OnActionButton2Clicked();

    /** Action button 3 click handler */
    UFUNCTION()
    void OnActionButton3Clicked();

    /** Send button click handler */
    UFUNCTION()
    void OnSendButtonClicked();

    /** Input box enter key submit handler */
    UFUNCTION()
    void OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /** Submit message */
    void SubmitMessage();
};