#pragma once
#include "AttributesTraits.h"
#include "Styling/SlateColor.h"

namespace sketch
{
	template <class T>
	struct TColorAttribute : public TCommonAttributeImplementation<T>
	{
		using Super = TCommonAttributeImplementation<T>;
		using Super::Super;
		using Super::Value;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
	};

	extern template SKETCH_API struct TColorAttribute<FLinearColor>;
	extern template SKETCH_API struct TColorAttribute<FSlateColor>;

	template <>
	struct TAttributeTraits<FLinearColor> : public TCommonAttributeTraits<TColorAttribute<FLinearColor>> {};

	template <>
	struct TAttributeTraits<FSlateColor> : public TCommonAttributeTraits<TColorAttribute<FSlateColor>> {};
}
