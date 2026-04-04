#pragma once
#include "AttributesTraits.h"
#include "Types/SlateStructs.h"

namespace sketch
{
	struct FOptionalSizeAttribute : public TCommonAttributeImplementation<FOptionalSize>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
	};

	template <>
	struct TAttributeTraits<FOptionalSize> : public TCommonAttributeTraits<FOptionalSizeAttribute> {};
}
