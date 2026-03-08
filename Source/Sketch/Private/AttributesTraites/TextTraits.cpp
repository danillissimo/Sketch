#include "AttributesTraites/TextTraits.h"

#include "Sketch.h"

TSharedRef<SWidget> sketch::TAttributeTraits<FText>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SEditableTextBox)
		.Text_Lambda([Handle] { return Handle.GetValue<FText>({}); })
		.OnTextChanged_Lambda([Handle](const FText& NewText) { Handle.SetValue<FText>(NewText); });
}

FString sketch::TAttributeTraits<FText>::GenerateCode(const FText& Text)
{
	return FString::Printf(TEXT("INVTEXT(\"%s\")"), *Text.ToString());
}
