#include "AttributesTraites/ColorTraits.h"

#include "Sketch.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"
#include "Styling/StyleColors.h"
#include "Widgets/Colors/SColorBlock.h"

#define LOCTEXT_NAMESPACE "Sketch.SColorEditor"

template <class T>
class SColorEditor : public SCompoundWidget
{
	static constexpr bool bSlateColor = std::is_same_v<T, FSlateColor>;
	enum ESlateColor { SC_GlobalStyle, SC_Style, SC_Foreground, SC_SubduedForeground };

public:
	SLATE_BEGIN_ARGS(SColorEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const sketch::FAttributeHandle& InHandle)
	{
		Handle = InHandle;
		Color = bSlateColor ? Handle.GetValue<FSlateColor>().GetSpecifiedColor() : Handle.GetValue<FLinearColor>();

		SHorizontalBox::FArguments Box;
		Box + SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBorder)
			.Padding(1)
			[
				SNew(SColorBlock)
				.Size(FVector2D{64, 16})
				.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
				.AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
				.ShowBackgroundForAlpha(true)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
				.OnMouseButtonDown(this, &SColorEditor::OpenColorPicker)
				.Color(this, &SColorEditor::GetColor)
			]
		];

		if constexpr (bSlateColor)
		{
			Box + SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(ComboButton, SComboButton)
				.OnGetMenuContent(this, &SColorEditor::ListSlateColors)
				.IsEnabled(false)
				.ButtonContent()
				[
					SNew(STextBlock).Text(LOCTEXT("SlateColor", "Slate Color"))
				]
			];
		}

		ChildSlot
		[
			SArgumentNew(Box, SHorizontalBox)
		];
	}

private:
	FLinearColor GetColor() const { return Color; }

	FReply OpenColorPicker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		FColorPickerArgs Args;
		Args.bUseAlpha = true;
		Args.bClampValue = true;
		Args.InitialColor = Color;
		Args.OnColorCommitted.BindSP(this, &SColorEditor::OnColorChanged);
		Args.OnColorPickerCancelled.BindSP(this, &SColorEditor::OnColorChanged);
		::OpenColorPicker(Args);
		return FReply::Handled();
	}

	void OnColorChanged(FLinearColor NewColor)
	{
		Color = Handle.SetValue(T(NewColor)) ? NewColor : FLinearColor::Black;
	}

	TSharedRef<SWidget> ListSlateColors()
	{
		FMenuBuilder Menu(true, nullptr);
		for (const auto& [SpecialSlateColor, Label, Type] : {
			     MakeTuple(FSlateColor::UseStyle(),LOCTEXT("SlateColor.UseStyle", "Use Style"), SC_Style),
			     MakeTuple(FSlateColor::UseForeground(),LOCTEXT("SlateColor.UseForeground", "Use Foreground"), SC_Foreground),
			     MakeTuple(FSlateColor::UseSubduedForeground(),LOCTEXT("SlateColor.UseSubduedForeground", "Use Subdued Foreground"), SC_SubduedForeground),
		     })
		{
			FMenuEntryParams Entry;
			Entry.LabelOverride = Label;
			Entry.DirectActions.ExecuteAction.BindSP(this, &SColorEditor::OnSlateColorSelected, SpecialSlateColor, Type);
			Menu.AddMenuEntry(Entry);
		}

		for (EStyleColor StyleColor = EStyleColor(0); StyleColor < EStyleColor::MAX; ++(std::underlying_type_t<EStyleColor>&)(StyleColor))
		{
			FMenuEntryParams Entry;
			Entry.DirectActions.ExecuteAction.BindSP(this, &SColorEditor::OnSlateColorSelected, FSlateColor(StyleColor), SC_GlobalStyle);
			Entry.EntryWidget = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4., 0.)
				[
					SNew(SBorder)
					.Padding(0.9)
					[
						SNew(SColorBlock)
						.Size(FVector2D{32, 16})
						.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
						.AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
						.ShowBackgroundForAlpha(true)
						.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
						.Color(FSlateColor(StyleColor).GetSpecifiedColor())
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4., 0.)
				[
					SNew(STextBlock)
					.Text(StaticEnum<EStyleColor>()->GetDisplayNameTextByIndex(static_cast<int>(StyleColor)))
				];
			Menu.AddMenuEntry(Entry);
		}
		return Menu.MakeWidget();
	}

	void OnSlateColorSelected(FSlateColor NewSlateColor, ESlateColor Type)
	{
		SlateColor = NewSlateColor;
		Color = NewSlateColor.GetSpecifiedColor();
		// ComboButton->SetContent(
		// 	SNew(STextBlock).Text(NewSlateColor.)
		// );
	}

	sketch::FAttributeHandle Handle;
	FLinearColor Color = FLinearColor::Black;
	TSharedPtr<SColorBlock> ColorBlock;

	FSlateColor SlateColor;
	TSharedPtr<SComboButton> ComboButton;
};

TSharedRef<SWidget> sketch::TAttributeTraits<FLinearColor>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SColorEditor<FLinearColor>, Handle);
}

FString sketch::TAttributeTraits<FLinearColor>::GenerateCode(const FLinearColor& Color)
{
	FString Result;
	Result.Reserve(32);
	Result += TEXT("FLinearColor{ ");
	Result += FString::SanitizeFloat(Color.R, 0);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Color.G, 0);
	Result += TEXT(", ");
	Result += FString::SanitizeFloat(Color.B, 0);
	if (Color.A != 1)
	{
		Result += TEXT(", ");
		Result += FString::SanitizeFloat(Color.A, 0);
	}
	Result += TEXT(" }");
	return Result;
}

TSharedRef<SWidget> sketch::TAttributeTraits<FSlateColor>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SColorEditor<FSlateColor>, Handle);
}

FString sketch::TAttributeTraits<FSlateColor>::GenerateCode(const FSlateColor& Color)
{
	return TAttributeTraits<FLinearColor>::GenerateCode(Color.GetSpecifiedColor());
}

#undef LOCTEXT_NAMESPACE
