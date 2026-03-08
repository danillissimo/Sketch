#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FFont
	{
		FFont() = default;
		SKETCH_API FFont(const FSlateFontInfo& Info);

		FFont(const FSlateFontInfo& FontInfo, const FName& StyleName, const FName& FontName)
			: FontInfo(FontInfo)
			  , StyleName(StyleName)
			  , FontName(FontName)
		{
		}

		FSlateFontInfo FontInfo;
		FName StyleName;
		FName FontName;
	};

	template <>
	struct TWrappedType<FFont>
	{
		using Type = FSlateFontInfo;
	};

	template <>
	struct TAttributeTraits<FSlateFontInfo>
	{
		using FStorage = FFont;

		static const FSlateFontInfo& GetValue(const FFont& Font) { return Font.FontInfo; }
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const FFont& Font);
	};
}
