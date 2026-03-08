#include "AttributesTraites/TextTraits.h"

#include "Sketch.h"

static sketch::FHeaderToolAttributeFilter GTextFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FText");
});

TSharedRef<SWidget> sketch::FTextAttribute::MakeEditor()
{
	return SNew(SEditableTextBox)
		.Text(TAttribute<FText>::CreateSP(this, &FTextAttribute::CopyValue))
		.OnTextChanged(FOnTextChanged::CreateSP(this, &FTextAttribute::SetValue));
}

FString sketch::FTextAttribute::GenerateCode() const
{
	return FString::Printf(TEXT("INVTEXT(\"%s\")"), *Value.ToString());
}

bool sketch::FTextAttribute::Equals(const IAttributeImplementation& Other) const
{
	return static_cast<const FTextAttribute&>(Other).Value.EqualTo(Value);
}
