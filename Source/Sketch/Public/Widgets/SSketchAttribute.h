#pragma once
#include "ImageColorAnimator.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SSketchHeaderRow;

namespace sketch
{
	struct FAttribute;
}

class SSketchAttribute : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchAttribute) {}
		SLATE_ARGUMENT(TWeakPtr<SSketchHeaderRow>, HeaderRow)
		/** Following settings prevail over HeaderRow state when provided, though doesn't change general column visibility */
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowLine);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowName);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowInteractivity);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowNumUsers);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, AllowCodePatching) = false;
	SLATE_END_ARGS()

	SKETCH_API void Construct(const FArguments& InArgs, sketch::FAttribute& Attribute);

private:
	FSlateColor GetBackgroundColor() const;

	FText GetNumUsers() const;
	bool TryPatchCode(bool bRemoveSketchInvocation);
	FReply PatchCode();
	FReply CopyCode();
	FReply Reset();
	EVisibility GetResetButtonVisibility() const;
	FReply OnBrowseSourceCode() const;

	TWeakPtr<sketch::FAttribute> WeakAttribute;

	TSharedPtr<SBox> EditorContainer;
	TSharedPtr<SImage> PatchCodeButtonIcon;
	FImageColorAnimator PatchCodeButtonAnimator;
	TWeakPtr<SSketchHeaderRow> WeakHeader;

	mutable uint16 NumDisplayedUsers = ~0;
	mutable FText NumDisplayedUsersText;
};
