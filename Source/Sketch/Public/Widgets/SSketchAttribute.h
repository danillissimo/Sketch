#pragma once
#include "SketchTypes.h"
#include "Widgets/SCompoundWidget.h"

namespace sketch
{
	struct FAttribute;
}

class SSketchAttribute : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchAttribute) {}
		SLATE_ARGUMENT(TWeakPtr<SHeaderRow>, Header)
		SLATE_ARGUMENT_DEFAULT(bool, ShowLine) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowName) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowInteractivity) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowNumUsers) = true;
		SLATE_ARGUMENT_DEFAULT(bool, AllowCodePatching) = false;
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const sketch::FAttributeHandle& AttributeHandle);

private:
	FText GetNumUsers() const;
	FReply PatchCode();
	FReply CopyCode();
	void Reset();
	EVisibility GetResetButtonVisibility() const;
	FReply OnBrowseSourceCode(const FGeometry& Geometry, const FPointerEvent& PointerEvent) const;

	sketch::FAttributeHandle Handle;

	TSharedPtr<SBox> EditorContainer;

	mutable uint16 NumDisplayedUsers = ~0;
	mutable FText NumDisplayedUsersText;
};
