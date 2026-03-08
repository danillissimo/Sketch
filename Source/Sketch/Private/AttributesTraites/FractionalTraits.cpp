#include "AttributesTraites/FractionalTraits.h"

#include "Widgets/Input/SSpinBox.h"
#include "Sketch.h"

static sketch::FHeaderToolAttributeFilter GFractionalFilter([](FStringView Attribute)
{
	return Attribute == TEXT("float") || Attribute == TEXT("double");
});

template <class T>
TSharedRef<SWidget> sketch::TFractionalAttribute<T>::MakeEditor()
{
	return SNew(SSpinBox<T>)
		.Delta(0.0001)
		.Value(static_cast<Super*>(this), &Super::CopyValue)
		.OnValueChanged(static_cast<Super*>(this), &Super::ReceiveValue);
}

template <class T>
FString sketch::TFractionalAttribute<T>::GenerateCode() const
{
	FString Result = FString::SanitizeFloat(Value);
	if constexpr (std::is_same_v<T, float>)
	{
		Result += TEXT("f");
	}
	return Result;
}

template struct sketch::TFractionalAttribute<float>;
template struct sketch::TFractionalAttribute<double>;
