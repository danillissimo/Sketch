#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	namespace Private
	{
		inline FVector2f ExtractShear(const TMatrix2x2<float>& Transform)
		{
			float Sin = 0.f;
			float Cos = 1.f;
			float Matrix[2][2];
			Transform.GetMatrix(Matrix[0][0], Matrix[0][1], Matrix[1][0], Matrix[1][1]);
			if (Matrix[1][0] != 0.f)
			{
				const double R = FMath::Sqrt(FMath::Square(Matrix[0][0]) + FMath::Square(Matrix[1][0]));
				Cos = Matrix[0][0] / R;
				Sin = -Matrix[1][0] / R;
			}
			return { Cos * Matrix[0][1] + Sin * Matrix[1][1], 0 };
		}
	}

	struct FSlateRenderTransformAttribute : public IAttributeImplementation
	{
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
		virtual void Reinitialize(const IAttributeImplementation& From) override;


		TOptional<float> GetTranslationX() const { return Translation.X; }
		TOptional<float> GetTranslationY() const { return Translation.Y; }
		void ReceiveTranslationX(float X) { Translation.X = X, OnValueChanged.Broadcast(); }
		void ReceiveTranslationY(float Y) { Translation.Y = Y, OnValueChanged.Broadcast(); }

		TOptional<float> GetScaleX() const { return Scale.X; }
		TOptional<float> GetScaleY() const { return Scale.Y; }
		TOptional<float> GetUniformScale() const { return Scale.X == Scale.Y ? Scale.X : TOptional<float>(); }
		void ReceiveScaleX(float X) { Scale.X = X, OnValueChanged.Broadcast(); }
		void ReceiveScaleY(float Y) { Scale.Y = Y, OnValueChanged.Broadcast(); }
		void ReceiveUniformScale(float UniformScale) { Scale.X = Scale.Y = UniformScale, OnValueChanged.Broadcast(); }

		TOptional<float> GetShearX() const { return Shear.X; }
		TOptional<float> GetShearY() const { return Shear.Y; }
		void ReceiveShearX(float X) { Shear.X = X, OnValueChanged.Broadcast(); }
		void ReceiveShearY(float Y) { Shear.Y = Y, OnValueChanged.Broadcast(); }

		float GetAngle() const { return Angle; }
		TOptional<float> GetOptionalAngle() const { return Angle; }
		void ReceiveAngle(float InAngle) { Angle = InAngle, OnValueChanged.Broadcast(); }

		FSlateRenderTransform GetValue() const
		{
			return ::Concatenate(
				FScale2D(Scale),
				FShear2D::FromShearAngles(Shear),
				FQuat2D(FMath::DegreesToRadians(Angle)),
				FVector2D(Translation)
			);
		}


		FSlateRenderTransformAttribute() = default;

		FSlateRenderTransformAttribute(const FVector2f& InTranslation)
			: Translation(InTranslation) {}

		FSlateRenderTransformAttribute(float UniformScale, const FVector2f& InTranslation = FVector2f::ZeroVector)
			: Translation(InTranslation)
			, Scale(UniformScale) {}

		FSlateRenderTransformAttribute(const TScale2<float>& InScale, const FVector2f& InTranslation = FVector2f::ZeroVector)
			: Translation(InTranslation)
			, Scale(InScale.GetVector()) {}

		FSlateRenderTransformAttribute(const TShear2<float>& InShear, const FVector2f& InTranslation = FVector2f::ZeroVector)
			: Translation(InTranslation)
			, Shear(InShear.GetVector()) {}

		FSlateRenderTransformAttribute(const TQuat2<float>& Rotation, const FVector2f& InTranslation = FVector2f::ZeroVector)
			: Translation(InTranslation)
			, Angle(FMath::Acos(Rotation.GetVector().X)) {}

		FSlateRenderTransformAttribute(const TMatrix2x2<float>& Transform, const FVector2f& InTranslation = FVector2f::ZeroVector)
			: Translation(InTranslation)
			, Scale(Transform.GetScale().GetVector())
			, Shear(Private::ExtractShear(Transform))
			, Angle(Transform.GetRotationAngle() / PI * 180) {}

		FSlateRenderTransformAttribute(const FSlateRenderTransform& Transform)
			: Translation(Transform.GetTranslation())
			, Scale(Transform.GetMatrix().GetScale().GetVector())
			, Shear(Private::ExtractShear(Transform.GetMatrix()))
			, Angle(Transform.GetMatrix().GetRotationAngle() / PI * 180) {}

		FSlateRenderTransformAttribute(const FSlateRenderTransformAttribute&) = default;
		FSlateRenderTransformAttribute(FSlateRenderTransformAttribute&&) = default;


		FVector2f Translation = FVector2f::ZeroVector;
		FVector2f Scale = FVector2f::UnitVector;
		FVector2f Shear = FVector2f::ZeroVector;
		float Angle = 0.f;
	};

	template <>
	struct TAttributeTraits<FSlateRenderTransform> : public TCommonAttributeTraits<FSlateRenderTransformAttribute> {};
}
