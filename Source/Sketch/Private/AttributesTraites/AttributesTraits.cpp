#include "AttributesTraites/AttributesTraits.h"

#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> sketch::Private::MakeNoEditor(const FAttributeHandle& Handle)
{
	return SNew(STextBlock).Text(INVTEXT("sketch::TAttributeTraits::MakeEditor has no overload for this type"));
}

FString sketch::Private::GenerateNoCode()
{
	return TEXT("sketch::TAttributeTraits::GenerateCode has no overload for this type");
}
