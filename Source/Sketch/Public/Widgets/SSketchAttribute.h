#pragma once
#include "Widgets/SCompoundWidget.h"

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

	void Construct(const FArguments& InArgs, sketch::FAttribute& Attribute);

private:
	FSlateColor GetBackgroundColor() const;

	FText GetNumUsers() const;
	bool TryPatchCode(bool bRemoveSketchInvocation);
	FReply PatchCode();
	EActiveTimerReturnType AnimatePatchCodeButton(double CurrentTime, float InDeltaTime);
	FReply CopyCode();
	void Reset();
	EVisibility GetResetButtonVisibility() const;
	FReply OnBrowseSourceCode() const;

	TWeakPtr<sketch::FAttribute> WeakAttribute;

	TSharedPtr<SBox> EditorContainer;
	TSharedPtr<SImage> PatchCodeButtonIcon;
	TWeakPtr<SSketchHeaderRow> WeakHeader;

	mutable uint16 NumDisplayedUsers = ~0;
	mutable FText NumDisplayedUsersText;
	FLinearColor CurrentPatchCodeButtonColor = FLinearColor::Black;
	TSharedPtr<FActiveTimerHandle> Animator;
};
