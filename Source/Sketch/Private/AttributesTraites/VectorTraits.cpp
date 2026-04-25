#include "AttributesTraites/VectorTraits.h"

#include "Widgets/Input/SVectorInputBox.h"

#define LOCTEXT_NAMESPACE "Sketch.FVectorAttribute"

static sketch::FHeaderToolAttributeFilter GVector2Filter([](FStringView Attribute)
{
	// Special cases
	static constexpr std::initializer_list<FStringView> SpecialCases = {
		TEXT("FVector"),
		TEXT("FVector2D"),
		TEXT("FVector4"),
	};
	for (const auto& SpecialCase : SpecialCases)
		if (Attribute == SpecialCase)
			return true;
	if (Attribute.EndsWith(TEXT("FDeprecateVector2DParameter")))
		return true;

	// Templated cases
	constexpr FStringView VectorName(TEXT("FVector"));
	for (int i = 0; i < 2; ++i)
	{
		if (!Attribute.IsValidIndex(VectorName.Len() + i))
			return false;
	}
	if (!Attribute.StartsWith(VectorName)) return false;
	const TCHAR Digit = Attribute[VectorName.Len()];
	if (!(Digit == TCHAR('2') || Digit == TCHAR('3') || Digit == TCHAR('4'))) return false;
	const TCHAR Type = Attribute[VectorName.Len() + 1];
	if (!(Type == TCHAR('f') || Type == TCHAR('d'))) return false;
	return true;
});

template <class T, int NumComponents>
TSharedRef<SWidget> sketch::TVectorAttribute<T, NumComponents>::MakeEditor()
{
	using TVectorInputBox = SNumericVectorInputBox<T, Private::TVector<T, NumComponents>, NumComponents>;
	auto Arguments =
		TVectorInputBox::FArguments()
		.AllowSpin(true)
		.SpinDelta(0.1)
		.bColorAxisLabels(true)
		.X(this, &TVectorAttribute<T, NumComponents>::GetX)
		.Y(this, &TVectorAttribute<T, NumComponents>::GetY)
		.OnXChanged(this, &TVectorAttribute<T, NumComponents>::ReceiveX)
		.OnYChanged(this, &TVectorAttribute<T, NumComponents>::ReceiveY)
		.XDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::X))
		.YDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::Y));
	if constexpr (NumComponents >= 3)
		Arguments.Z(this, &TVectorAttribute<T, NumComponents>::GetZ)
		         .OnZChanged(this, &TVectorAttribute<T, NumComponents>::ReceiveZ)
		         .ZDisplayName(AxisDisplayInfo::GetAxisDisplayName(EAxisList::Z));
	if constexpr (NumComponents >= 4)
		Arguments.W(this, &TVectorAttribute<T, NumComponents>::GetW)
		         .OnWChanged(this, &TVectorAttribute<T, NumComponents>::ReceiveW)
		         .WDisplayName(LOCTEXT("WAxisName", "W"));
	return SArgumentNew(Arguments, TVectorInputBox);
}

template <class T, int NumComponents>
FString sketch::TVectorAttribute<T, NumComponents>::GenerateCode() const
{
	FString Result;
	auto PrintSuffix = [&]()
	{
		if constexpr (std::is_same_v<T, float>)
			Result += TEXT("f");
	};

	Result = TEXT("FVector");
	Result += NumComponents == 2 ? TEXT("2") : NumComponents == 3 ? TEXT("3") : TEXT("4");
	Result += std::is_same_v<T, float> ? TEXT("f") : TEXT("d");
	Result += TEXT("(");
	Result += FString::SanitizeFloat(Value.X);
	PrintSuffix();
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Value.Y);
	PrintSuffix();
	if constexpr (NumComponents >= 3)
	{
		Result += TEXT(", ");
		Result += FString::SanitizeFloat(Value.Z);
		PrintSuffix();
	}
	if constexpr (NumComponents >= 4)
	{
		Result += TEXT(", ");
		Result += FString::SanitizeFloat(Value.W);
		PrintSuffix();
	}
	Result += TEXT(")");
	return Result;
}

template struct sketch::TVectorAttribute<float, 2>;
template struct sketch::TVectorAttribute<float, 3>;
template struct sketch::TVectorAttribute<float, 4>;
template struct sketch::TVectorAttribute<double, 2>;
template struct sketch::TVectorAttribute<double, 3>;
template struct sketch::TVectorAttribute<double, 4>;
