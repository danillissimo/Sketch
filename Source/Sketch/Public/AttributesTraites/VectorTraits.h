#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	namespace Private
	{
		template <class T, int NumComponents>
		using TVector = std::conditional_t<
			NumComponents == 2,
			UE::Math::TVector2<T>,
			std::conditional_t<
				NumComponents == 3,
				UE::Math::TVector<T>,
				UE::Math::TVector4<T>
			>
		>;
	}

	template <class T, int NumComponents>
	struct TVectorAttribute : public TCommonAttributeImplementation<Private::TVector<T, NumComponents>>
	{
		using Super = TCommonAttributeImplementation<Private::TVector<T, NumComponents>>;
		using Super::Super;
		using Super::Value;
		using Super::OnValueChanged;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;

		TOptional<T> GetX() const { return Value.X; }
		TOptional<T> GetY() const { return Value.Y; }
		TOptional<T> GetZ() const requires(NumComponents >= 3) { return Value.Z; }
		TOptional<T> GetW() const requires(NumComponents >= 4) { return Value.W; }
		void ReceiveX(T X) { Value.X = X, OnValueChanged.Broadcast(); }
		void ReceiveY(T Y) { Value.Y = Y, OnValueChanged.Broadcast(); }
		void ReceiveZ(T Z) requires(NumComponents >= 3) { Value.Z = Z, OnValueChanged.Broadcast(); }
		void ReceiveW(T W) requires(NumComponents >= 4) { Value.W = W, OnValueChanged.Broadcast(); }
	};

	extern template SKETCH_API struct TVectorAttribute<float, 2>;
	extern template SKETCH_API struct TVectorAttribute<float, 3>;
	extern template SKETCH_API struct TVectorAttribute<float, 4>;
	extern template SKETCH_API struct TVectorAttribute<double, 2>;
	extern template SKETCH_API struct TVectorAttribute<double, 3>;
	extern template SKETCH_API struct TVectorAttribute<double, 4>;

	template <>
	struct TAttributeTraits<FVector2f> : public TCommonAttributeTraits<TVectorAttribute<float, 2>> {};
	template <>
	struct TAttributeTraits<FVector3f> : public TCommonAttributeTraits<TVectorAttribute<float, 3>> {};
	template <>
	struct TAttributeTraits<FVector4f> : public TCommonAttributeTraits<TVectorAttribute<float, 4>> {};
	template <>
	struct TAttributeTraits<FVector2d> : public TCommonAttributeTraits<TVectorAttribute<double, 2>> {};
	template <>
	struct TAttributeTraits<FVector3d> : public TCommonAttributeTraits<TVectorAttribute<double, 3>> {};
	template <>
	struct TAttributeTraits<FVector4d> : public TCommonAttributeTraits<TVectorAttribute<double, 4>> {};

	template <>
	struct TAttributeTraits<UE::Slate::FDeprecateVector2DParameter>
	{
		static TVectorAttribute<float, 2> ConstructValue(UE::Slate::FDeprecateVector2DParameter&& Initializer) { return TVectorAttribute<float, 2>(Initializer); }
		static TVectorAttribute<float, 2> ConstructValue(const UE::Slate::FDeprecateVector2DParameter& Initializer) { return TVectorAttribute<float, 2>(Initializer); }
		static UE::Slate::FDeprecateVector2DParameter GetValue(const IAttributeImplementation& Attribute) { return static_cast<const TVectorAttribute<float, 2>&>(Attribute).GetValue(); }
	};
}
