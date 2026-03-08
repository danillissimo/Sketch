#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FMarginAttribute : public TCommonAttributeImplementation<FMargin>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
	};

	template <>
	struct TAttributeTraits<FMargin> : public TCommonAttributeTraits<FMarginAttribute> {};
}
