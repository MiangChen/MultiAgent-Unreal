// MATextDisplay.h
// 滚动文本显示屏 - LED 屏幕效果

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MATextDisplay.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

/**
 * 单个字符数据
 */
USTRUCT()
struct FCharacterSlot
{
    GENERATED_BODY()

    /** 字符文本组件 */
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> TextComponent = nullptr;

    /** 字符内容 */
    FString Character;

    /** 当前 Y 位置 */
    float CurrentY = 0.f;

    /** 字符宽度 */
    float CharWidth = 0.f;

    /** 是否激活 */
    bool bIsActive = false;
};

/**
 * 滚动文本显示屏
 * 
 * 模拟 LED 显示屏效果：
 * - 背景板带边框
 * - 字符从右边依次弹出，平滑移动到左边消失
 * - 始终面向摄像机 (Billboard)
 */
UCLASS()
class MULTIAGENT_API AMATextDisplay : public AActor
{
    GENERATED_BODY()

public:
    AMATextDisplay();

    /** 设置显示文本并开始滚动 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void SetText(const FString& InText);

    /** 开始滚动 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void StartScrolling();

    /** 停止滚动 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void StopScrolling();

    /** 设置滚动速度 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void SetScrollSpeed(float InSpeed) { ScrollSpeed = InSpeed; }

    /** 设置字符弹出间隔 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void SetCharacterInterval(float InInterval) { CharacterInterval = InInterval; }

    /** 设置显示屏尺寸 */
    UFUNCTION(BlueprintCallable, Category = "TextDisplay")
    void SetDisplaySize(float Width, float Height);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 根组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> RootScene;

    /** 背景板 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BackgroundMesh;

    /** 边框 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BorderMesh;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 显示屏宽度 (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float DisplayWidth = 200.f;

    /** 显示屏高度 (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float DisplayHeight = 40.f;

    /** 边框厚度 (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float BorderThickness = 10.f;

    /** 滚动速度 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float ScrollSpeed = 50.f;  // 降低速度减少残影

    /** 字符弹出间隔 (秒) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float CharacterInterval = 0.15f;

    /** 背景颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.05f, 1.f);

    /** 边框颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FLinearColor BorderColor = FLinearColor(1.f, 1.f, 1.f, 1.f);  // 白色边框

    /** 文字颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FColor TextColor = FColor(255, 200, 50);

    /** 文字大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float TextSize = 30.f;

private:
    /** 显示的文本 */
    FString DisplayText;

    /** 字符槽位数组 */
    UPROPERTY()
    TArray<FCharacterSlot> CharacterSlots;

    /** 当前要弹出的字符索引 */
    int32 NextCharIndex = 0;

    /** 距离下一个字符弹出的时间 */
    float TimeToNextChar = 0.f;

    /** 是否正在滚动 */
    bool bIsScrolling = false;

    /** 是否已完成所有字符弹出 */
    bool bAllCharsSpawned = false;

    /** 动态材质实例 */
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> BackgroundMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> BorderMaterial;

    /** 创建背景和边框 */
    void CreateDisplayComponents();

    /** 更新字符位置 */
    void UpdateCharacters(float DeltaTime);

    /** 弹出下一个字符 */
    void SpawnNextCharacter();

    /** 面向摄像机 */
    void FaceCameraUpdate();

    /** 获取字符宽度 */
    float GetCharWidth(TCHAR Char) const;

    /** 清理所有字符 */
    void ClearAllCharacters();

    /** 屏幕右边界 Y 坐标 */
    float GetRightBoundary() const { return DisplayWidth * 0.5f - 10.f; }

    /** 屏幕左边界 Y 坐标 */
    float GetLeftBoundary() const { return -DisplayWidth * 0.5f + 10.f; }
};
