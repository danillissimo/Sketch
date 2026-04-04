#pragma once
#include "AttributesTraits.h"
#include "Layout/Margin.h"

namespace sketch
{
	struct FMarginAttribute : public TCommonAttributeImplementation<FMargin>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
	};

	template <>
	struct TAttributeTraits<FMargin> : public TCommonAttributeTraits<FMarginAttribute> {};
}
