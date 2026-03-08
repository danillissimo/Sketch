#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <>
	struct TAttributeTraits<float> : public TCommonAttributesTraits<float>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const float& Float);
	};

	template <>
	struct TAttributeTraits<double> : public TCommonAttributesTraits<double>
	{
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const double& Double);
	};
}
