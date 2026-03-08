#include "AttributesTraites/OptionalSizeTraits.h"

#include "Sketch.h"
#include "Widgets/Input/SSpinBox.h"

using namespace sketch;

class SOptionalSizeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOptionalSizeEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FAttributeHandle& InHandle)
	{
		Handle = InHandle;
		const bool bIsSet = Handle.GetValue<FOptionalSize>().IsSet();
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
		return Handle.GetValue<FOptionalSize>().Get();
	}

	void OnValueChanged(float NewValue)
	{
		Handle.SetValue<FOptionalSize>({NewValue});
	}

	void OnCheckBoxSwitched(ECheckBoxState NewState)
	{
		const FOptionalSize NewValue = NewState == ECheckBoxState::Checked ? FOptionalSize{8.f} : FOptionalSize{};
		const bool bEnabled = NewState == ECheckBoxState::Checked;
		Handle.SetValue<FOptionalSize>({NewValue});
		SpinBox->SetValue(NewValue.Get());
		SpinBox->SetEnabled(bEnabled);
	}

	FAttributeHandle Handle;
	TSharedPtr<SSpinBox<float>> SpinBox;
};

TSharedRef<SWidget> TAttributeTraits<FOptionalSize>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SOptionalSizeEditor, Handle);
}
