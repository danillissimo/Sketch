#pragma once
#include "Widgets/Views/SHeaderRow.h"

class SKETCH_API SSketchHeaderRow : public SHeaderRow
{
public:
	SLATE_BEGIN_ARGS(SSketchHeaderRow) {}
		SLATE_ARGUMENT_DEFAULT(bool, ShowLine) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowName) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowInteractivity) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowNumUsers) = true;
		SLATE_ARGUMENT_DEFAULT(bool, AllowCodePatching) = false;
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	struct FColumnProperties
	{
		FName Id;
		float Size = 0.f;
		EColumnSizeMode::Type SizeMode = EColumnSizeMode::Type(0);
		FText Label;
	};
	/** @note Is a method so data can be updated by live coding instead of by a reboot */
	static std::array<FColumnProperties, 7> GetColumnsProperties();

	static const FText InteractivityTooltip;
	static const FText NumUsersTooltip;
};
