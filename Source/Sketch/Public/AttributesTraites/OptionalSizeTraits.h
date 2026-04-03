#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FOptionalSizeAttribute : public TCommonAttributeImplementation<FOptionalSize>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
	};

	template <>
	struct TAttributeTraits<FOptionalSize> : public TCommonAttributeTraits<FOptionalSizeAttribute> {};
}
