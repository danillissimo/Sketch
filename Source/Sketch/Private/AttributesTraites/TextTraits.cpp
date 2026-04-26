#include "AttributesTraites/TextTraits.h"

#include "Sketch.h"
#include "SketchStringLiteral.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

static sketch::FHeaderToolAttributeFilter GTextFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FText");
});

TSharedRef<SWidget> sketch::FTextAttribute::MakeEditor()
{
	return SNew(SMultiLineEditableTextBox)
		.Text(TAttribute<FText>::CreateSP(this, &FTextAttribute::CopyValue))
		.OnTextChanged(FOnTextChanged::CreateSP(this, &FTextAttribute::SetValue));
}

FString sketch::FTextAttribute::GenerateCode() const
{
	FString Result = SL"INVTEXT(\"";
	Result += Value.ToString().Replace(SL"\n", SL"\\n\"\n\"", ESearchCase::CaseSensitive);
	Result += SL"\")";
	return Result;
}

bool sketch::FTextAttribute::Equals(const IAttributeImplementation& Other) const
{
	return static_cast<const FTextAttribute&>(Other).Value.EqualTo(Value);
}
