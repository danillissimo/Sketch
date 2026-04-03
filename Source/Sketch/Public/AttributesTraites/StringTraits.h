#pragma once
#include "AttributesTraits.h"
#include "Internationalization/Text.h"

namespace sketch
{
	struct FStringAttribute : public TCommonAttributeImplementation<FString>
	{
		using Super = TCommonAttributeImplementation<FString>;
		using Super::Super;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		FText ToText() const { return FText::FromString(Value); }
		void FromText(const FText& InText) { Value = InText.ToString(); }
	};

	template <>
	struct TAttributeTraits<FString> : public TCommonAttributeTraits<FStringAttribute> {};
}
