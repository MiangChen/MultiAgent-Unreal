// MASpeechBubbleWidget.cpp
// 通用气泡文本框组件实现

#include "MASpeechBubbleWidget.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Domain/MASpeechBubbleModels.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"

//=============================================================================
// 三角形纹理生成（引脚用）
//=============================================================================

namespace
{
    UTexture2D* CreateDownTriangleTexture(const FLinearColor& Color, int32 TexW, int32 TexH)
    {
        UTexture2D* Tex = UTexture2D::CreateTransient(TexW, TexH, PF_B8G8R8A8);
        if (!Tex) return nullptr;

        FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
        void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
        uint8* Px = static_cast<uint8*>(RawData);

        const uint8 cB = FMath::Clamp(int32(Color.B * 255.f), 0, 255);
        const uint8 cG = FMath::Clamp(int32(Color.G * 255.f), 0, 255);
        const uint8 cR = FMath::Clamp(int32(Color.R * 255.f), 0, 255);
        const uint8 cA = FMath::Clamp(int32(Color.A * 255.f), 0, 255);
        const float HalfW = TexW * 0.5f;

        for (int32 Y = 0; Y < TexH; ++Y)
        {
            float Ratio = float(Y) / float(FMath::Max(TexH - 1, 1));
            float HalfRow = HalfW * (1.0f - Ratio);
            float Left = HalfW - HalfRow;
            float Right = HalfW + HalfRow;

            for (int32 X = 0; X < TexW; ++X)
            {
                int32 i = (Y * TexW + X) * 4;
                float fX = float(X);
                if (fX >= Left && fX <= Right)
                {
                    // 边缘抗锯齿
                    float EdgeDist = FMath::Min(fX - Left, Right - fX);
                    float AA = FMath::Clamp(EdgeDist / 1.5f, 0.0f, 1.0f);
                    Px[i+0] = cB; Px[i+1] = cG; Px[i+2] = cR;
                    Px[i+3] = uint8(cA * AA);
                }
                else
                {
                    Px[i+0] = 0; Px[i+1] = 0; Px[i+2] = 0; Px[i+3] = 0;
                }
            }
        }

        Mip.BulkData.Unlock();
        Tex->UpdateResource();
        return Tex;
    }
}

//=============================================================================
// 构造函数
//=============================================================================

UMASpeechBubbleWidget::UMASpeechBubbleWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetVisibility(ESlateVisibility::Collapsed);
}

//=============================================================================
// 消息控制
//=============================================================================

void UMASpeechBubbleWidget::ShowMessage(const FString& Message, float Duration)
{
    if (Message.IsEmpty())
    {
        HideMessage();
        return;
    }

    if (MessageText)
    {
        MessageText->SetText(FText::FromString(Message));
    }

    Coordinator.BeginShow(BubbleState, Duration);

    SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRenderOpacity(0.0f);
}

void UMASpeechBubbleWidget::HideMessage()
{
    Coordinator.BeginHide(BubbleState);
}

//=============================================================================
// 主题
//=============================================================================

void UMASpeechBubbleWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme) return;

    if (BubbleBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(
            BubbleBorder,
            FLinearColor(1.0f, 0.95f, 0.80f, 0.97f),  // 浅橙黄色
            BubbleCornerRadius
        );
    }

    if (ShadowBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(
            ShadowBorder,
            FLinearColor(0.0f, 0.0f, 0.0f, 0.30f),
            BubbleCornerRadius + 2.0f
        );
    }

    if (MessageText)
    {
        FSlateFontInfo FontInfo = Theme->BodyFont;
        FontInfo.Size = FontSize;
        FontInfo.TypefaceFontName = FName("Bold");
        MessageText->SetFont(FontInfo);
        MessageText->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));  // 黑色加粗
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

TSharedRef<SWidget> UMASpeechBubbleWidget::RebuildWidget()
{
    BuildUI();
    return Super::RebuildWidget();
}

void UMASpeechBubbleWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMASpeechBubbleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const FMASpeechBubbleAnimationFrame Frame = Coordinator.Step(BubbleState, InDeltaTime, FadeInDuration, FadeOutDuration);
    SetRenderOpacity(Frame.Opacity);
    if (Frame.bHideNow)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 动态定位引脚到气泡底部中央
    if (BubbleState.bVisible && BubbleBorder)
    {
        FVector2D BubbleSize = BubbleBorder->GetDesiredSize();
        if (BubbleSize.X > 0.0f && BubbleSize.Y > 0.0f)
        {
            float BubbleBottom = BubbleSize.Y;

            // 同步阴影大小（跟随气泡主体）
            if (ShadowBorder)
            {
                if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(ShadowBorder->Slot))
                {
                    S->SetPosition(FVector2D(ShadowOffsetX, ShadowOffsetY));
                    S->SetSize(FVector2D(BubbleSize.X, 0.0f));
                }
            }

            // 引脚位置 - 放在左下侧
            if (TailImage)
            {
                if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(TailImage->Slot))
                {
                    S->SetPosition(FVector2D(TailOffsetFromLeft, BubbleBottom - 1.0f));
                }
            }
            if (TailShadowImage)
            {
                if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(TailShadowImage->Slot))
                {
                    S->SetPosition(FVector2D(
                        TailOffsetFromLeft + ShadowOffsetX,
                        BubbleBottom - 1.0f + ShadowOffsetY * 0.5f));
                }
            }
        }
    }
}

//=============================================================================
// UI 构建
//=============================================================================

void UMASpeechBubbleWidget::BuildUI()
{
    if (!WidgetTree) return;

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BubbleRootCanvas"));
    if (!RootCanvas) return;
    WidgetTree->RootWidget = RootCanvas;

    // ---- 阴影层 ----
    ShadowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShadowBorder"));
    if (ShadowBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(
            ShadowBorder,
            FLinearColor(0.0f, 0.0f, 0.0f, 0.30f),
            BubbleCornerRadius + 2.0f
        );
        ShadowBorder->SetPadding(FMargin(BubblePaddingH + 2.0f, BubblePaddingV + 2.0f));

        RootCanvas->AddChild(ShadowBorder);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ShadowBorder->Slot))
        {
            Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Slot->SetPosition(FVector2D(ShadowOffsetX, ShadowOffsetY));
            Slot->SetSize(FVector2D(BubbleMaxWidth, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // ---- 引脚阴影 ----
    TailShadowImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TailShadowImage"));
    if (TailShadowImage)
    {
        UTexture2D* ShadowTex = CreateDownTriangleTexture(FLinearColor(0.0f, 0.0f, 0.0f, 0.25f), 32, 24);
        if (ShadowTex)
        {
            TailShadowImage->SetBrushFromTexture(ShadowTex);
            TailShadowImage->SetDesiredSizeOverride(FVector2D(TailWidth + 4.0f, TailHeight + 2.0f));
        }
        RootCanvas->AddChild(TailShadowImage);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TailShadowImage->Slot))
        {
            Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Slot->SetPosition(FVector2D(0.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // ---- 气泡主体 ----
    BubbleBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleBorder"));
    if (BubbleBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(
            BubbleBorder,
            FLinearColor(1.0f, 0.95f, 0.80f, 0.97f),  // 浅橙黄色背景
            BubbleCornerRadius
        );
        BubbleBorder->SetPadding(FMargin(BubblePaddingH, BubblePaddingV));

        ContentSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ContentSizeBox"));
        if (ContentSizeBox)
        {
            ContentSizeBox->SetMaxDesiredWidth(BubbleMaxWidth - BubblePaddingH * 2.0f);
            ContentSizeBox->SetMinDesiredWidth(120.0f);

            MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
            if (MessageText)
            {
                MessageText->SetText(FText::GetEmpty());
                MessageText->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));  // 黑色字体
                MessageText->SetAutoWrapText(true);

                // World 空间下使用较大字号，加粗以保证可读性
                FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
                MessageText->SetFont(FontInfo);

                ContentSizeBox->AddChild(MessageText);
            }

            BubbleBorder->SetContent(ContentSizeBox);
        }

        RootCanvas->AddChild(BubbleBorder);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(BubbleBorder->Slot))
        {
            Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Slot->SetPosition(FVector2D(0.0f, 0.0f));
            Slot->SetSize(FVector2D(BubbleMaxWidth, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // ---- 引脚三角形（白色） ----
    TailImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TailImage"));
    if (TailImage)
    {
        UTexture2D* TailTex = CreateDownTriangleTexture(FLinearColor(1.0f, 0.95f, 0.80f, 0.97f), 32, 24);  // 浅橙黄色引脚
        if (TailTex)
        {
            TailImage->SetBrushFromTexture(TailTex);
            TailImage->SetDesiredSizeOverride(FVector2D(TailWidth, TailHeight));
        }
        RootCanvas->AddChild(TailImage);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TailImage->Slot))
        {
            Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Slot->SetPosition(FVector2D(0.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }
}
