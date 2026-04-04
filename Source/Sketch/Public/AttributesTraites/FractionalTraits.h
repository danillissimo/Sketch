#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <class T>
	struct TFractionalAttribute : public TCommonAttributeImplementation<T>
	{
		using Super = TCommonAttributeImplementation<T>;
		using Super::Super;
		using Super::Value;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
	};

	extern template SKETCH_API struct TFractionalAttribute<float>;
	extern template SKETCH_API struct TFractionalAttribute<double>;

	template <>
	struct TAttributeTraits<float> : public TCommonAttributeTraits<TFractionalAttribute<float>> {};

	template <>
	struct TAttributeTraits<double> : public TCommonAttributeTraits<TFractionalAttribute<double>> {};
}
