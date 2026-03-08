#include "AttributesTraites/FractionalTraits.h"

#include "Widgets/Input/SSpinBox.h"
#include "Sketch.h"

TSharedRef<SWidget> sketch::TAttributeTraits<float>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SSpinBox<float>)
		.Font(Private::DefaultFont())
		.Delta(0.0001)
		.Value_Lambda([Handle] { return Handle.GetValue<float>(0.f); })
		.OnValueChanged_Lambda([Handle](float NewValue) { Handle.SetValue<float>(NewValue); });
}

FString sketch::TAttributeTraits<float>::GenerateCode(const float& Float)
{
	return FString::SanitizeFloat(Float);
}

TSharedRef<SWidget> sketch::TAttributeTraits<double>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SSpinBox<double>)
		.Font(Private::DefaultFont())
		.Delta(0.0001)
		.Value_Lambda([Handle] { return Handle.GetValue<double>(0.f); })
		.OnValueChanged_Lambda([Handle](double NewValue) { Handle.SetValue<double>(NewValue); });
}

FString sketch::TAttributeTraits<double>::GenerateCode(const double& Double)
{
	return FString::SanitizeFloat(Double);
}
