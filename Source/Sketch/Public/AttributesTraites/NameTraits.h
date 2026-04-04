#pragma once
#include "AttributesTraits.h"
#include "Internationalization/Text.h"

namespace sketch
{
	struct FNameAttribute : public TCommonAttributeImplementation<FName>
	{
		using Super = TCommonAttributeImplementation<FName>;
		using Super::Super;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
		FText ToText() const { return FText::FromName(Value); }
		void FromText(const FText& InText) { Value = *InText.ToString(); }
	};

	template <>
	struct TAttributeTraits<FName> : public TCommonAttributeTraits<FNameAttribute> {};
}
