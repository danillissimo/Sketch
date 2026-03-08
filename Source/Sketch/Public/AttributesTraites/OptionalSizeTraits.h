#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <>
	struct TAttributeTraits<FOptionalSize> : public TCommonAttributesTraits<FOptionalSize>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		// SKETCH_API static FString GenerateCode(const FMargin& Margin);
	};
}
