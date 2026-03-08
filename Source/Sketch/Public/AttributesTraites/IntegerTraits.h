#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	/** All integral types are merged down to this type to reduce number of explicit cases to handle */
	struct FInteger
	{
		FInteger() = default;
		template <class T>
		FInteger(T&& Initializer);
		bool operator==(const FInteger&) const = default;

		int64 Value = 0;
		const UEnum* Enum = nullptr;
	};

	template <class T>
	concept CInteger = std::is_integral_v<T> || std::is_enum_v<T>;

	template<>
	struct TWrappedType<FInteger>
	{
		using Type = int64;
	};

	template <CInteger T>
	struct TAttributeTraits<T>
	{
		using FStorage = FInteger;

		static const T& GetValue(const FStorage& Storage)
		{
			// "return (const T&)Storage.Value" throws a "returning address of a local variable" err for some reason
			const T* Result = (const T*)&Storage.Value;
			return *Result;
		}

		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle) { return Private::MakeNoEditor(Handle); }
		SKETCH_API static FString GenerateCode(const FInteger& Integer) { return Private::GenerateNoCode(); }
	};
}

namespace sketch
{
	template <class T>
	FInteger::FInteger(T&& Initializer)
		: Value(int64(Initializer))
	{
		if constexpr (TIsUEnumClass<T>::Value)
		{
			Enum = StaticEnum<T>();
		}
	}
}
