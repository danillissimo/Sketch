#pragma once
#include "AttributesTraits.h"
#include "Internationalization/Text.h"

namespace sketch
{
	struct FTextAttribute : public TCommonAttributeImplementation<FText>
	{
		using Super = TCommonAttributeImplementation<FText>;
		using Super::Super;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
	};

	template <>
	struct TAttributeTraits<FText> : public TCommonAttributeTraits<FTextAttribute> {};
}
