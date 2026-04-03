#include "AttributesTraites/MarginTraits.h"

#include "Sketch.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "Sketch.SMarginEditor"

static sketch::FHeaderToolAttributeFilter GMarginFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FMargin");
});

class SMarginEditor : public SCompoundWidget
{
	enum ECheckBoxID { X2, X4 };

public:
	SLATE_BEGIN_ARGS(SMarginEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, sketch::FMarginAttribute& Attribute)
	{
		WeakAttribute = Attribute.AsSharedSubobject(&Attribute);

		const FMargin Margin = Attribute.Value;
		const bool bUniform = Margin.Left == Margin.Top && Margin.Left == Margin.Bottom && Margin.Left == Margin.Right;
		const bool bHorizontalVertical = Margin.Left == Margin.Right && Margin.Top == Margin.Bottom;
		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(x1, SSpinBox<float>)
				.Font(sketch::Private::DefaultFont())
				.MinValue(0)
				.MaxFractionalDigits(1)
				.Value(Margin.Left)
				.OnValueChanged(this, &SMarginEditor::OnValueChanged)
				.ToolTipText(LOCTEXT("UnifromOrLeftOrHorizontal", "Uniform/Horizontal/Left"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(bX2, SCheckBox)
				.OnCheckStateChanged(this, &SMarginEditor::OnXChanged, X2)
				.IsChecked(!bUniform ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.ToolTipText(LOCTEXT("DissociateHorizontalAndVerticalMargins", "Dissociate horizontal/vertical margins"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(x2, SSpinBox<float>)
				.Font(sketch::Private::DefaultFont())
				.MinValue(0)
				.MaxFractionalDigits(1)
				.Value(Margin.Top)
				.OnValueChanged(this, &SMarginEditor::OnValueChanged)
				.IsEnabled(!bUniform)
				.ToolTipText(LOCTEXT("VerticalOrTop", "Vertical/Top"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(bX4, SCheckBox)
				.OnCheckStateChanged(this, &SMarginEditor::OnXChanged, X4)
				.IsChecked(!bUniform && !bHorizontalVertical ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.ToolTipText(LOCTEXT("DissociateAllMargins", "Dissociate all margins"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(x3, SSpinBox<float>)
				.Font(sketch::Private::DefaultFont())
				.MinValue(0)
				.MaxFractionalDigits(1)
				.Value(Margin.Right)
				.OnValueChanged(this, &SMarginEditor::OnValueChanged)
				.IsEnabled(!bUniform && !bHorizontalVertical)
				.ToolTipText(LOCTEXT("Right", "Right"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(x4, SSpinBox<float>)
				.Font(sketch::Private::DefaultFont())
				.MinValue(0)
				.MaxFractionalDigits(1)
				.Value(Margin.Bottom)
				.OnValueChanged(this, &SMarginEditor::OnValueChanged)
				.IsEnabled(!bUniform && !bHorizontalVertical)
				.ToolTipText(LOCTEXT("Bottom", "Bottom"))
			]
		];
	}

private:
	void OnXChanged(ECheckBoxState NewState, ECheckBoxID CheckBoxID)
	{
		if (CheckBoxID == X2)
		{
			if (!bX2->IsChecked())
			{
				bX4->SetIsChecked(ECheckBoxState::Unchecked);
			}
		}
		else if (CheckBoxID == X4)
		{
			if (bX4->IsChecked())
			{
				bX2->SetIsChecked(ECheckBoxState::Checked);
			}
		}

		OnValueChanged(0);
	}

	void OnValueChanged(float NewValue)
	{
		// Do not allow recursion
		if (bUpdating)
			return;
		TGuardValue Guard(bUpdating, true);

		TSharedPtr<sketch::FMarginAttribute> Attribute = WeakAttribute.Pin();
		if (!Attribute)
		[[unlikely]]
		{
			bX2->SetIsChecked(ECheckBoxState::Undetermined);
			bX4->SetIsChecked(ECheckBoxState::Undetermined);
			x1->SetValue(0);
			x2->SetValue(0);
			x3->SetValue(0);
			x4->SetValue(0);
			x1->SetEnabled(false);
			x2->SetEnabled(false);
			x3->SetEnabled(false);
			x4->SetEnabled(false);
			return;
		}

		// Uniform margin
		FMargin& Value = Attribute->Value;
		if (!bX2->IsChecked())
		{
			x2->SetValue(x1->GetValue());
			x3->SetValue(x1->GetValue());
			x4->SetValue(x1->GetValue());
			x1->SetEnabled(true);
			x2->SetEnabled(false);
			x3->SetEnabled(false);
			x4->SetEnabled(false);
			Value = FMargin{ x1->GetValue() };
			return;
		}

		// Horizontal + vertical margin
		if (!bX4->IsChecked())
		{
			x3->SetValue(x1->GetValue());
			x4->SetValue(x2->GetValue());
			x1->SetEnabled(true);
			x2->SetEnabled(true);
			x3->SetEnabled(false);
			x4->SetEnabled(false);
			Value = FMargin{ x1->GetValue(), x2->GetValue() };
			return;
		}

		// Complex margin
		x1->SetEnabled(true);
		x2->SetEnabled(true);
		x3->SetEnabled(true);
		x4->SetEnabled(true);
		Value = FMargin{ x1->GetValue(), x2->GetValue(), x3->GetValue(), x4->GetValue() };
	}

	bool bUpdating = false;
	TWeakPtr<sketch::FMarginAttribute> WeakAttribute;
	TSharedPtr<SSpinBox<float>> x1;
	TSharedPtr<SSpinBox<float>> x2;
	TSharedPtr<SSpinBox<float>> x3;
	TSharedPtr<SSpinBox<float>> x4;
	TSharedPtr<SCheckBox> bX2;
	TSharedPtr<SCheckBox> bX4;
};

TSharedRef<SWidget> sketch::FMarginAttribute::MakeEditor()
{
	return SNew(SMarginEditor, *this);
}

FString sketch::FMarginAttribute::GenerateCode() const
{
	FString Result;
	Result.Reserve(32);
	Result += FString::SanitizeFloat(Value.Left, 0);
	if (Value.Left == Value.Top && Value.Left == Value.Right && Value.Left == Value.Bottom)
	{
		return Result;
	}
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Value.Top, 0);
	if (Value.Left == Value.Right && Value.Top == Value.Bottom)
	{
		return Result;
	}
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Value.Right, 0);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Value.Bottom, 0);
	return Result;
}

#undef LOCTEXT_NAMESPACE
