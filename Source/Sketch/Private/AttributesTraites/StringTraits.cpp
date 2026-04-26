#include "AttributesTraites/StringTraits.h"

#include "Sketch.h"
#include "SketchStringLiteral.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

static sketch::FHeaderToolAttributeFilter GStringFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FString");
});

TSharedRef<SWidget> sketch::FStringAttribute::MakeEditor()
{
	return SNew(SMultiLineEditableTextBox)
		.Text(TAttribute<FText>::CreateSP(this, &FStringAttribute::ToText))
		.OnTextChanged(FOnTextChanged::CreateSP(this, &FStringAttribute::FromText));
}

FString sketch::FStringAttribute::GenerateCode() const
{
	FString Result = SL"TEXT(\"";
	Result += Value.Replace(SL"\n", SL"\\n\"\n\"", ESearchCase::CaseSensitive);
	Result += SL"\")";
	return Result;
}
