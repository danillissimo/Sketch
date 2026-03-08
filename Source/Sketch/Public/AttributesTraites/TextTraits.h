#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FTextAttribute : public TCommonAttributeImplementation<FText>
	{
		using Super = TCommonAttributeImplementation<FText>;
		using Super::Super;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
	};

	template <>
	struct TAttributeTraits<FText> : public TCommonAttributeTraits<FTextAttribute> {};
}
