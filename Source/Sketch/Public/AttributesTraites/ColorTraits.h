#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <class T>
	struct TColorAttribute : public TCommonAttributeImplementation<T>
	{
		using Super = TCommonAttributeImplementation<T>;
		using Super::Super;
		using Super::Value;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
	};

	extern template struct TColorAttribute<FLinearColor>;
	extern template struct TColorAttribute<FSlateColor>;

	template <>
	struct TAttributeTraits<FLinearColor> : public TCommonAttributeTraits<TColorAttribute<FLinearColor>> {};

	template <>
	struct TAttributeTraits<FSlateColor> : public TCommonAttributeTraits<TColorAttribute<FSlateColor>> {};
}
