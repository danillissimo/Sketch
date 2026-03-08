#include "AttributesTraites/AttributesTraits.h"

#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> sketch::IAttributeImplementation::MakeEditor()
{
	return SNew(STextBlock)
		.Text(INVTEXT("sketch::IAttributeImplementation::MakeEditor has no overload for this type"))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8));
}

FString sketch::IAttributeImplementation::GenerateCode() const
{
	return TEXT("sketch::IAttributeImplementation::GenerateCode has no overload for this type");
}

TArray<TFunction<bool(FStringView Type)>>& sketch::GetSupportedAttributeFilters()
{
	static TArray<TFunction<bool(FStringView Type)>> Filters;
	return Filters;
}
