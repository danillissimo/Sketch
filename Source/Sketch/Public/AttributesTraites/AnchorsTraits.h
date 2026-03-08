#pragma once
#include "VectorTraits.h"

#include "Widgets/Layout/Anchors.h"

namespace sketch
{
	template <>
	struct TAttributeTraits<FAnchors>
	{
		template <class... ArgTypes>
		static TVectorAttribute<double, 4> ConstructValue(ArgTypes&&... Args)
		{
			FAnchors Anchors(std::forward<ArgTypes>(Args)...);
			return TAttributeTraits<FVector4d>::ConstructValue(Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y);
		}

		static FAnchors GetValue(const IAttributeImplementation& Attribute)
		{
			const FVector4d& Value = static_cast<const TVectorAttribute<double, 4>&>(Attribute).GetValue();
			return FAnchors(Value.X, Value.Y, Value.Z, Value.W);
		}
	};
}
