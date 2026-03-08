#include "AttributesTraites/BooleanTraits.h"

static sketch::FHeaderToolAttributeFilter GBooleanFilter([](FStringView Attribute)
{
	return Attribute == TEXT("bool");
});

TSharedRef<SWidget> sketch::FBooleanAttribute::MakeEditor()
{
	return SNew(SCheckBox)
		.IsChecked(this, &FBooleanAttribute::GetState)
		.OnCheckStateChanged(this, &FBooleanAttribute::SetState);
}

FString sketch::FBooleanAttribute::GenerateCode() const
{
	return Value ? "true" : "false";
}
