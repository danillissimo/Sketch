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
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
	};

	extern template struct TFractionalAttribute<float>;
	extern template struct TFractionalAttribute<double>;

	template <>
	struct TAttributeTraits<float> : public TCommonAttributeTraits<TFractionalAttribute<float>> {};

	template <>
	struct TAttributeTraits<double> : public TCommonAttributeTraits<TFractionalAttribute<double>> {};
}
