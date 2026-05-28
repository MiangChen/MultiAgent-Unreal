// MARoundedBorderUtils.cpp
// 圆角边框工具类实现

#include "MARoundedBorderUtils.h"
#include "MAUITheme.h"
#include "Components/Border.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

//=============================================================================
// 画刷创建方法
//=============================================================================

FSlateBrush MARoundedBorderUtils::CreateRoundedBoxBrush(
    const FLinearColor& Color, 
    float CornerRadius)
{
    // 处理无效圆角半径 - 使用 0 表示标准矩形
    const float SafeRadius = FMath::Max(0.0f, CornerRadius);
    
    // 使用 FSlateRoundedBoxBrush 创建圆角画刷
    // 参数: 背景颜色, 圆角半径, 轮廓颜色, 轮廓宽度
    FSlateRoundedBoxBrush RoundedBrush(
        Color,                          // 背景颜色
        SafeRadius,                     // 圆角半径
        FLinearColor::Transparent,      // 无轮廓
        0.0f                            // 轮廓宽度为 0
    );
    
    return RoundedBrush;
}

FSlateBrush MARoundedBorderUtils::CreateRoundedBoxBrush(
    const FLinearColor& Color,
    float TopLeft,
    float TopRight,
    float BottomRight,
    float BottomLeft)
{
    // 处理无效圆角半径
    const float SafeTopLeft = FMath::Max(0.0f, TopLeft);
    const float SafeTopRight = FMath::Max(0.0f, TopRight);
    const float SafeBottomRight = FMath::Max(0.0f, BottomRight);
    const float SafeBottomLeft = FMath::Max(0.0f, BottomLeft);
    
    // 创建圆角向量 (顺序: TopLeft, TopRight, BottomRight, BottomLeft)
    FVector4 CornerRadii(SafeTopLeft, SafeTopRight, SafeBottomRight, SafeBottomLeft);
    
    // 使用 FSlateRoundedBoxBrush 创建不同角半径的圆角画刷
    FSlateRoundedBoxBrush RoundedBrush(
        Color,                          // 背景颜色
        CornerRadii,                    // 各角圆角半径
        FLinearColor::Transparent,      // 无轮廓
        0.0f                            // 轮廓宽度为 0
    );
    
    return RoundedBrush;
}

//=============================================================================
// 圆角应用方法
//=============================================================================

void MARoundedBorderUtils::ApplyRoundedCorners(
    UBorder* Border,
    const FLinearColor& Color,
    float CornerRadius)
{
    // 检查 Border 是否有效
    if (!Border)
    {
        UE_LOG(LogTemp, Warning, TEXT("MARoundedBorderUtils::ApplyRoundedCorners - Border is null"));
        return;
    }
    
    // 创建圆角画刷
    FSlateBrush RoundedBrush = CreateRoundedBoxBrush(Color, CornerRadius);
    
    // 应用画刷到 Border
    Border->SetBrush(RoundedBrush);
}

void MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
    UBorder* Border,
    const UMAUITheme* Theme,
    EMARoundedElementType ElementType,
    float CustomRadius)
{
    // 检查 Border 是否有效
    if (!Border)
    {
        UE_LOG(LogTemp, Warning, TEXT("MARoundedBorderUtils::ApplyRoundedCornersFromTheme - Border is null"));
        return;
    }
    
    // 获取圆角半径
    float CornerRadius = GetCornerRadiusForType(Theme, ElementType);
    
    // 如果是自定义类型，使用传入的自定义半径
    if (ElementType == EMARoundedElementType::Custom)
    {
        CornerRadius = FMath::Max(0.0f, CustomRadius);
    }
    
    // 获取背景颜色 - 从主题或使用默认值
    FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
    if (Theme)
    {
        BackgroundColor = Theme->BackgroundColor;
    }
    
    // 应用圆角
    ApplyRoundedCorners(Border, BackgroundColor, CornerRadius);
}

//=============================================================================
// 辅助方法
//=============================================================================

float MARoundedBorderUtils::GetCornerRadiusForType(
    const UMAUITheme* Theme,
    EMARoundedElementType ElementType)
{
    // 如果主题为 null，使用默认值
    if (!Theme)
    {
        UE_LOG(LogTemp, Warning, TEXT("MARoundedBorderUtils::GetCornerRadiusForType - Theme is null, using default values"));
        
        switch (ElementType)
        {
        case EMARoundedElementType::Panel:
            return DefaultPanelCornerRadius;
        case EMARoundedElementType::Button:
            return DefaultButtonCornerRadius;
        case EMARoundedElementType::Custom:
        default:
            return DefaultPanelCornerRadius;
        }
    }
    
    // 从主题获取圆角半径
    switch (ElementType)
    {
    case EMARoundedElementType::Panel:
        return Theme->CornerRadius > 0.0f ? Theme->CornerRadius : DefaultPanelCornerRadius;
    case EMARoundedElementType::Button:
        return Theme->ButtonCornerRadius > 0.0f ? Theme->ButtonCornerRadius : DefaultButtonCornerRadius;
    case EMARoundedElementType::Custom:
    default:
        return Theme->CornerRadius > 0.0f ? Theme->CornerRadius : DefaultPanelCornerRadius;
    }
}
