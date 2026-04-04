#pragma once
#include "Runtime/SlateCore/Private/Application/ActiveTimerHandle.h"

class SImage;
class SWidget;

/** A simple utility that animates given image color from InitialColor to white with constant speed */
struct SKETCH_API FImageColorAnimator
{
	void Animate(SWidget& Owner, SImage& Image, FLinearColor InitialColor);

private:
	EActiveTimerReturnType Tick(double CurrentTime, float DeltaTime, SImage* Image);

	TSharedPtr<FActiveTimerHandle> Ticker;
	FLinearColor CurrentColor = FLinearColor::White;
};
