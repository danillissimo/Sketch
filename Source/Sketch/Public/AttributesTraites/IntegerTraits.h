#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	template <class T>
	struct TIntegerAttribute : public TCommonAttributeImplementation<T>
	{
		using Super = TCommonAttributeImplementation<T>;
		using Super::Super;
		using Super::Value;

		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
	};

	extern template SKETCH_API struct TIntegerAttribute<uint8>;
	extern template SKETCH_API struct TIntegerAttribute<uint16>;
	extern template SKETCH_API struct TIntegerAttribute<uint32>;
	extern template SKETCH_API struct TIntegerAttribute<uint64>;
	extern template SKETCH_API struct TIntegerAttribute<int8>;
	extern template SKETCH_API struct TIntegerAttribute<int16>;
	extern template SKETCH_API struct TIntegerAttribute<int32>;
	extern template SKETCH_API struct TIntegerAttribute<int64>;

	template <class T>
		requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
	struct TAttributeTraits<T> : public TCommonAttributeTraits<TIntegerAttribute<T>> {};
}
