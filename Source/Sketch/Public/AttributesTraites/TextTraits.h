#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <>
	struct TAttributeTraits<FText> : public TCommonAttributesTraits<FText>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const FText& Text);
	};
}
