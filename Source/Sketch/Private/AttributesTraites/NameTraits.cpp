#include "AttributesTraites/NameTraits.h"

#include "Sketch.h"
#include "HeaderTool/StringLiteral.h"

static sketch::FHeaderToolAttributeFilter GNameFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FName");
});

TSharedRef<SWidget> sketch::FNameAttribute::MakeEditor()
{
	return SNew(SEditableTextBox)
		.Text(TAttribute<FText>::CreateSP(this, &FNameAttribute::ToText))
		.OnTextChanged(FOnTextChanged::CreateSP(this, &FNameAttribute::FromText));
}

FString sketch::FNameAttribute::GenerateCode() const
{
	if (Value.IsNone()) return SL"NAME_None";
	FString Result = SL"TEXT(\"";
	Result += Value.ToString().Replace(SL"\n", SL"\\n\"\n\"", ESearchCase::CaseSensitive);
	Result += SL"\")";
	return Result;
}
