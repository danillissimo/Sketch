#include "AttributesTraites/OptionalSizeTraits.h"

#include "Sketch.h"
#include "HeaderTool/StringLiteral.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"

using namespace sketch;

static sketch::FHeaderToolAttributeFilter GOptionalSizeFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FOptionalSize");
});

class SOptionalSizeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOptionalSizeEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FOptionalSizeAttribute& Attribute)
	{
		WeakAttribute = Attribute.AsWeakSubobject(&Attribute);
		const bool bIsSet = Attribute.Value.IsSet();
		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[

				SAssignNew(SpinBox, SSpinBox<float>)
				.Font(sketch::Private::DefaultFont())
				.Delta(1.f)
				.Value(this, &SOptionalSizeEditor::GetValue)
				.OnValueChanged(this, &SOptionalSizeEditor::OnValueChanged)
				.IsEnabled(bIsSet)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SOptionalSizeEditor::OnCheckBoxSwitched)
				.IsChecked(bIsSet ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			]
		];
	}

private:
	float GetValue() const
	{
		if (const TSharedPtr<FOptionalSizeAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
		{
			return Attribute->Value.Get();
		}
		return 0.f;
	}

	void OnValueChanged(float NewValue)
	{
		if (const TSharedPtr<FOptionalSizeAttribute> Attribute = WeakAttribute.Pin())[[likely]]
		{
			Attribute->SetValue({ NewValue });
		}
	}

	void OnCheckBoxSwitched(ECheckBoxState NewState)
	{
		const FOptionalSize NewValue = NewState == ECheckBoxState::Checked ? FOptionalSize{ 8.f } : FOptionalSize{};
		const bool bEnabled = NewState == ECheckBoxState::Checked;
		SpinBox->SetValue(NewValue.Get());
		SpinBox->SetEnabled(bEnabled);
		if (const TSharedPtr<FOptionalSizeAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
		{
			Attribute->SetValue(NewValue);
		}
	}

	TWeakPtr<FOptionalSizeAttribute> WeakAttribute;
	TSharedPtr<SSpinBox<float>> SpinBox;
};

TSharedRef<SWidget> FOptionalSizeAttribute::MakeEditor()
{
	return SNew(SOptionalSizeEditor, *this);
}

FString FOptionalSizeAttribute::GenerateCode() const
{
	FString Result = SL"FOptionalSize(";
	if (Value.IsSet())
		Result += FString::SanitizeFloat(Value.Get());
	Result += SL")";
	return Result;
}
