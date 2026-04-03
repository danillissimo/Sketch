#include "AttributesTraites/StringTraits.h"

#include "Sketch.h"
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
	return FString::Printf(TEXT("TEXT(\"%s\")"), *Value);
}
