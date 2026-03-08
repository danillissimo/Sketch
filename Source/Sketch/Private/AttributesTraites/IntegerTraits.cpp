#include "AttributesTraites/IntegerTraits.h"

#include "Widgets/Input/SSpinBox.h"

static sketch::FHeaderToolAttributeFilter GIntegerFilter([](FStringView Attribute)
{
	std::initializer_list<FStringView> Types{
		TEXT("uint8"),
		TEXT("uint16"),
		TEXT("uint32"),
		TEXT("uint64"),
		TEXT("int8"),
		TEXT("int16"),
		TEXT("int32"),
		TEXT("int64")
	};
	for (const auto& Type : Types)
		if (Attribute == Type)
			return true;
	return false;
});

template <class T>
TSharedRef<SWidget> sketch::TIntegerAttribute<T>::MakeEditor()
{
	return SNew(SSpinBox<T>)
		.Value(this, &TIntegerAttribute<T>::CopyValue)
		.OnValueChanged(this, &TIntegerAttribute<T>::ReceiveValue);
}

template <class T>
FString sketch::TIntegerAttribute<T>::GenerateCode() const
{
	constexpr bool bUint64 = std::is_same_v<T, uint64>;
	constexpr bool bInt64 = std::is_same_v<T, int64>;
	if constexpr (bUint64 || bInt64)
	{
		return FString::Printf(bUint64 ? TEXT("%llu") : TEXT("%lld"), Value);
	}
	else
	{
		return FString::FromInt(Value);
	}
}

template struct sketch::TIntegerAttribute<uint8>;
template struct sketch::TIntegerAttribute<uint16>;
template struct sketch::TIntegerAttribute<uint32>;
template struct sketch::TIntegerAttribute<uint64>;
template struct sketch::TIntegerAttribute<int8>;
template struct sketch::TIntegerAttribute<int16>;
template struct sketch::TIntegerAttribute<int32>;
template struct sketch::TIntegerAttribute<int64>;
