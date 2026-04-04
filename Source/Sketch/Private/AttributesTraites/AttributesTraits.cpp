#include "AttributesTraites/AttributesTraits.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/STextScroller.h"

bool GCodeGeneratorOverloaded = false;

TSharedRef<SWidget> sketch::IAttributeImplementation::MakeEditor()
{
	return
			SNew(SBox)
			.VAlign(VAlign_Center)
			[
				SNew(STextScroller)
				[
					SNew(STextBlock)
					.Text(INVTEXT("sketch::IAttributeImplementation::MakeEditor has no overload for this type"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
			];
}

FString sketch::IAttributeImplementation::GenerateCode() const
{
	GCodeGeneratorOverloaded = false;
	return TEXT("sketch::IAttributeImplementation::GenerateCode has no overload for this type");
}

TOptional<FString> sketch::IAttributeImplementation::TryGenerateCode() const
{
	GCodeGeneratorOverloaded = true;
	FString Result = GenerateCode();
	return GCodeGeneratorOverloaded ? TOptional<FString>(MoveTemp(Result)) : TOptional<FString>();
}

TArray<TFunction<bool(FStringView Type)>>& sketch::GetSupportedAttributeFilters()
{
	static TArray<TFunction<bool(FStringView Type)>> Filters;
	return Filters;
}
