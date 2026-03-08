#include "AttributesTraites/NameTraits.h"

#include "Sketch.h"

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
	return FString::Printf(TEXT("TEXT(\"%s\")"), *Value.ToString());
}
