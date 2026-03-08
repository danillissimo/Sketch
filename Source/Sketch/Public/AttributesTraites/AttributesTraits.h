#pragma once

namespace sketch
{
	struct FAttributeHandle;

	namespace Private
	{
		SKETCH_API TSharedRef<SWidget> MakeNoEditor(const FAttributeHandle& Handle);
		SKETCH_API FString GenerateNoCode();
	}

	///
	/// Old
	///
	template <class T>
	struct TCommonAttributesTraits
	{
		using FStorage = T;

		static const T& GetValue(const T& Storage) { return Storage; }
		static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle) { return Private::MakeNoEditor(Handle); }
		static FString GenerateCode(const T& Value) { return Private::GenerateNoCode(); }
	};

	template <class T>
	struct TWrappedType
	{
		using Type = T;
	};

	template <class T>
	struct TAttributeTraits;

	template<class T>
	using TWrappedAttributeTraits = TAttributeTraits<typename TWrappedType<T>::Type>;

	///
	/// New
	///
	// class IAttribute
	// {
	// public:
	// 	virtual ~IAttribute() = default;
	//
	// 	virtual TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle) { return Private::MakeNoEditor(Handle); }
	// 	virtual FString GenerateCode() { return Private::GenerateNoCode(); }
	// };
}
