#pragma once
#include "Widgets/SCompoundWidget.h"

class SSearchBox;
class SMultiLineEditableTextBox;
class SSuggestionTextBox;

class SKETCH_API SSketchHeaderTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchHeaderTool) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> ListCommonPaths();
	void SelectCommonPath(const FStringView Path);
	void SelectProjectPath(const FStringView Path);

	FReply GenerateCode();

	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STextBlock> Lines;
	TSharedPtr<SMultiLineEditableTextBox> GeneratedCode;
	TSharedPtr<SEditableTextBox> PathEdit;
	TSharedPtr<SEditableTextBox> InclusionRootEdit;
};
