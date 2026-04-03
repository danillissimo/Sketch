#include "Widgets/ImageColorAnimator.h"

#include "Types/SlateEnums.h"
#include "Widgets/Images/SImage.h"

void FImageColorAnimator::Animate(SWidget& Owner, SImage& Image, FLinearColor InitialColor)
{
	CurrentColor = InitialColor;
	Image.SetColorAndOpacity(CurrentColor);
	if (!Ticker)
	{
		Image.RegisterActiveTimer(0, FWidgetActiveTimerDelegate::CreateSP(Owner.AsSharedSubobject(this), &FImageColorAnimator::Tick, &Image));
	}
}

EActiveTimerReturnType FImageColorAnimator::Tick(double CurrentTime, float DeltaTime, SImage* Image)
{
	constexpr float Speed = 0.33f;
	CurrentColor.R = FMath::Min(1.f, CurrentColor.R + DeltaTime * Speed);
	CurrentColor.G = FMath::Min(1.f, CurrentColor.G + DeltaTime * Speed);
	CurrentColor.B = FMath::Min(1.f, CurrentColor.B + DeltaTime * Speed);
	Image->SetColorAndOpacity(CurrentColor);
	if (CurrentColor == FLinearColor::White) [[unlikely]]
	{
		Ticker.Reset();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}
