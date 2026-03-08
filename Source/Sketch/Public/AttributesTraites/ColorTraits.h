#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <>
	struct TAttributeTraits<FLinearColor> : public TCommonAttributesTraits<FLinearColor>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const FLinearColor& Color);
	};

	template <>
	struct TAttributeTraits<FSlateColor> : public TCommonAttributesTraits<FSlateColor>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const FSlateColor& Color);
	};
}
