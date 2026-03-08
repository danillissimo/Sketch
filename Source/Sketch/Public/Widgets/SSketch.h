#pragma once
#include "Widgets/SCompoundWidget.h"

class SSketch : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketch) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TArray<FName>  names;
};
