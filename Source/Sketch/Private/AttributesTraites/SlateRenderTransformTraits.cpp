#include "AttributesTraites/SlateRenderTransformTraits.h"

#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "Sketch.FSlateRenderTransformAttribute"

static const sketch::FHeaderToolAttributeFilter GSlateRenderTransformFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FSlateRenderTransform");
});

TSharedRef<SWidget> sketch::FSlateRenderTransformAttribute::MakeEditor()
{
	using SVector2InputBox = SNumericVectorInputBox<float, UE::Math::TVector2<float>, 2>;
	using SVector4InputBox = SNumericVectorInputBox<float, UE::Math::TVector4<float>, 4>;
	return SNew(SExpandableArea)
		.InitiallyCollapsed(true)
		.HeaderContent()
		[
			SNew(SVector4InputBox)
			.AllowSpin(true)
			.SpinDelta(0.05)
			.bColorAxisLabels(true)
			.X(this, &FSlateRenderTransformAttribute::GetTranslationX)
			.Y(this, &FSlateRenderTransformAttribute::GetTranslationY)
			.Z(this, &FSlateRenderTransformAttribute::GetUniformScale)
			.W(this, &FSlateRenderTransformAttribute::GetOptionalAngle)
			.OnXChanged(this, &FSlateRenderTransformAttribute::ReceiveTranslationX)
			.OnYChanged(this, &FSlateRenderTransformAttribute::ReceiveTranslationY)
			.OnZChanged(this, &FSlateRenderTransformAttribute::ReceiveUniformScale)
			.OnWChanged(this, &FSlateRenderTransformAttribute::ReceiveAngle)
			.XDisplayName(LOCTEXT("TranslationXName", "Translation X"))
			.YDisplayName(LOCTEXT("TranslationYName", "Translation Y"))
			.ZDisplayName(LOCTEXT("UniformScaleName", "Uniform Scale"))
			.WDisplayName(LOCTEXT("AngleName", "Angle"))
		]
		.BodyContent()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVector2InputBox)
				.AllowSpin(true)
				.SpinDelta(1)
				.bColorAxisLabels(true)
				.X(this, &FSlateRenderTransformAttribute::GetTranslationX)
				.Y(this, &FSlateRenderTransformAttribute::GetTranslationY)
				.OnXChanged(this, &FSlateRenderTransformAttribute::ReceiveTranslationX)
				.OnYChanged(this, &FSlateRenderTransformAttribute::ReceiveTranslationY)
				.XDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::X))
				.YDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::Y))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVector2InputBox)
				.AllowSpin(true)
				.SpinDelta(0.05)
				.bColorAxisLabels(true)
				.X(this, &FSlateRenderTransformAttribute::GetScaleX)
				.Y(this, &FSlateRenderTransformAttribute::GetScaleY)
				.OnXChanged(this, &FSlateRenderTransformAttribute::ReceiveScaleX)
				.OnYChanged(this, &FSlateRenderTransformAttribute::ReceiveScaleY)
				.XDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::X))
				.YDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::Y))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVector2InputBox)
				.AllowSpin(true)
				.SpinDelta(1)
				.bColorAxisLabels(true)
				.X(this, &FSlateRenderTransformAttribute::GetShearX)
				.Y(this, &FSlateRenderTransformAttribute::GetShearY)
				.OnXChanged(this, &FSlateRenderTransformAttribute::ReceiveShearX)
				.OnYChanged(this, &FSlateRenderTransformAttribute::ReceiveShearY)
				.XDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::X))
				.YDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::Y))
				.MinVector(UE::Math::TVector2{ -89.f, -89.f })
				.MaxVector(UE::Math::TVector2{ 89.f, 89.f })
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSpinBox<float>)
				.Delta(1)
				.MinValue(-180)
				.MaxValue(180)
				.Value(this, &FSlateRenderTransformAttribute::GetAngle)
				.OnValueChanged(this, &FSlateRenderTransformAttribute::ReceiveAngle)
			]
		];
}

FString sketch::FSlateRenderTransformAttribute::GenerateCode() const
{
	const FSlateRenderTransform Value = GetValue();
	float Matrix[2][2];
	Value.GetMatrix().GetMatrix(Matrix[0][0], Matrix[0][1], Matrix[1][0], Matrix[1][1]);
	FString Result;
	Result.Reserve(128);
	Result += TEXT("{ {");
	Result += FString::SanitizeFloat(Matrix[0][0]);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Matrix[0][1]);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Matrix[1][0]);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Matrix[1][1]);
	Result += TEXT("}, {");
	Result += FString::SanitizeFloat(Value.GetTranslation().X);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Value.GetTranslation().Y);
	Result += TEXT("} }");
	return Result;
}

bool sketch::FSlateRenderTransformAttribute::Equals(const IAttributeImplementation& Other) const
{
	const FSlateRenderTransformAttribute& OtherTransform = static_cast<const FSlateRenderTransformAttribute&>(Other);
	return
		Translation == OtherTransform.Translation
		&& Scale == OtherTransform.Scale
		&& Shear == OtherTransform.Shear
		&& Angle == OtherTransform.Angle;
}

void sketch::FSlateRenderTransformAttribute::Reinitialize(const IAttributeImplementation& From)
{
	const FSlateRenderTransformAttribute& Other = static_cast<const FSlateRenderTransformAttribute&>(From);
	Translation = Other.Translation;
	Scale = Other.Scale;
	Shear = Other.Shear;
	Angle = Other.Angle;
}

#undef LOCTEXT_NAMESPACE
