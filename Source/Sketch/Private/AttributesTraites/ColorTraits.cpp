#include "AttributesTraites/ColorTraits.h"

#include "Sketch.h"
#include "SketchSandbox.h"
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"
#include "Styling/StyleColors.h"
#include "Widgets/ImageColorAnimator.h"
#include "Widgets/Colors/SColorBlock.h"

#define LOCTEXT_NAMESPACE "Sketch.SColorEditor"

static sketch::FHeaderToolAttributeFilter GColorFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FSlateColor") || Attribute == TEXT("FLinearColor");
});

static sketch::Sandbox::TItemInitializer<FSlateColor> GSandboxColor(TEXT("Color"));

struct FAccessibleSlateColorFields : FSlateColor
{
public:
	using FSlateColor::SpecifiedColor;
	using FSlateColor::ColorUseRule;
};
struct FAllSlateColorFields
{
	FLinearColor SpecifiedColor;
	ESlateColorStylingMode ColorUseRule;
	EStyleColor ColorTableId;
};
static_assert(sizeof(FAllSlateColorFields) == sizeof(FSlateColor));
static_assert(alignof(FAllSlateColorFields) == alignof(FSlateColor));
static_assert(offsetof(FAllSlateColorFields, SpecifiedColor) == offsetof(FAccessibleSlateColorFields, SpecifiedColor));
static_assert(offsetof(FAllSlateColorFields, ColorUseRule) == offsetof(FAccessibleSlateColorFields, ColorUseRule));

template <class T>
class SColorEditor : public SCompoundWidget
{
	static constexpr bool bSlateColor = std::is_same_v<T, FSlateColor>;

public:
	SLATE_BEGIN_ARGS(SColorEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, sketch::TColorAttribute<T>& Attribute)
	{
		WeakAttribute = Attribute.AsSharedSubobject(&Attribute);
		Color = Attribute.Value;

		SHorizontalBox::FArguments Box;
		Box + SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SNew(SBorder)
				.Padding(1)
				[
					SNew(SColorBlock)
					.Size(FVector2D{ 64, 16 })
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
			Box
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ToolTipText(LOCTEXT("TryDetectSlateColor", "Try detect Slate color"))
					.IsEnabled(this, &SColorEditor::CanTryDetectSlateColor)
					.OnClicked(this, &SColorEditor::TryDetectSlateColor)
					[
						SAssignNew(TryConvertButtonImage, SImage)
						.Image(FSlateIcon("CoreStyle", "Icons.ArrowRight").GetIcon())
					]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(ComboButton, SComboButton)
					.OnGetMenuContent(this, &SColorEditor::ListSlateColors)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(this, &SColorEditor::GetSlateColorName)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				];
		}

		ChildSlot
		[
			SArgumentNew(Box, SHorizontalBox)
		];
	}

private:
	FLinearColor GetColor() const { return Color.GetSpecifiedColor(); }

	FReply OpenColorPicker(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		FColorPickerArgs Args;
		Args.bUseAlpha = true;
		Args.bClampValue = true;
		Args.InitialColor = Color.GetSpecifiedColor();
		Args.OnColorCommitted.BindSP(this, &SColorEditor::OnColorChanged);
		Args.OnColorPickerCancelled.BindSP(this, &SColorEditor::OnColorChanged);
		::OpenColorPicker(Args);
		return FReply::Handled();
	}

	void OnColorChanged(FLinearColor NewColor)
	{
		Color = NewColor;
		if (TSharedPtr<sketch::TColorAttribute<T>> Attribute = WeakAttribute.Pin())[[likely]]
		{
			Attribute->SetValue(NewColor);
		}
	}

	bool CanTryDetectSlateColor() const
	{
		const FAllSlateColorFields& ColorInternals = (const FAllSlateColorFields&)(Color);
		return ColorInternals.ColorUseRule == ESlateColorStylingMode::UseColor_Specified;
	}

	FReply TryDetectSlateColor()
	{
		if (TSharedPtr<sketch::TColorAttribute<T>> Attribute = WeakAttribute.Pin())[[likely]]
		{
			const FAllSlateColorFields& ColorInternals = (const FAllSlateColorFields&)(Color);
			for (uint8 ColorId = 0; ColorId < uint8(EStyleColor::MAX); ++ColorId)
			{
				FSlateColor SlateColor = EStyleColor(ColorId);
				if (ColorInternals.SpecifiedColor == SlateColor.GetSpecifiedColor())
				{
					Color = SlateColor;
					Attribute->SetValue(Color);
					Animator.Animate(*this, *TryConvertButtonImage.Get(), FLinearColor::Green);
					return FReply::Handled();
				}
			}
		}
		Animator.Animate(*this, *TryConvertButtonImage.Get(), FLinearColor::Red);
		return FReply::Handled();
	}

	TSharedRef<SWidget> ListSlateColors()
	{
		FMenuBuilder Menu(true, nullptr);
		for (const auto& [SpecialSlateColor, Label] : {
			     MakeTuple(FSlateColor::UseStyle(),LOCTEXT("SlateColor.UseStyle", "Style")),
			     MakeTuple(FSlateColor::UseForeground(),LOCTEXT("SlateColor.UseForeground", "Foreground")),
			     MakeTuple(FSlateColor::UseSubduedForeground(),LOCTEXT("SlateColor.UseSubduedForeground", "Subdued Foreground")),
		     })
		{
			FMenuEntryParams Entry;
			Entry.LabelOverride = Label;
			Entry.DirectActions.ExecuteAction.BindSP(this, &SColorEditor::OnSlateColorSelected, SpecialSlateColor);
			Menu.AddMenuEntry(Entry);
		}

		for (EStyleColor StyleColor = EStyleColor(0); StyleColor < EStyleColor::MAX; ++(std::underlying_type_t<EStyleColor>&)(StyleColor))
		{
			FMenuEntryParams Entry;
			Entry.DirectActions.ExecuteAction.BindSP(this, &SColorEditor::OnSlateColorSelected, FSlateColor(StyleColor));
			Entry.EntryWidget = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4., 0.)
				[
					SNew(SBorder)
					.Padding(0.9)
					[
						SNew(SColorBlock)
						.Size(FVector2D{ 32, 16 })
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
					.Text(StaticEnum<EStyleColor>()->GetDisplayNameTextByValue(static_cast<int>(StyleColor)))
				];
			Menu.AddMenuEntry(Entry);
		}
		return Menu.MakeWidget();
	}

	void OnSlateColorSelected(FSlateColor NewSlateColor)
	{
		Color = NewSlateColor;
		if (auto Attribute = WeakAttribute.Pin())[[likely]]
		{
			if constexpr (bSlateColor)
			{
				Attribute->SetValue(Color);
			}
			else
			{
				Attribute->SetValue(Color.GetSpecifiedColor());
			}
		}
	}

	FText GetSlateColorName() const
	{
		const FAllSlateColorFields& ColorInternals = (const FAllSlateColorFields&)(Color);
		switch (ColorInternals.ColorUseRule)
		{
		case ESlateColorStylingMode::UseColor_Specified: return LOCTEXT("SlateColor.UseSpecified", "Specified Color");
		case ESlateColorStylingMode::UseColor_ColorTable: return StaticEnum<EStyleColor>()->GetDisplayNameTextByValue(static_cast<int>(ColorInternals.ColorTableId));
			break;
		case ESlateColorStylingMode::UseColor_Foreground: return LOCTEXT("SlateColor.UseForeground", "Foreground");
		case ESlateColorStylingMode::UseColor_Foreground_Subdued: return LOCTEXT("SlateColor.UseSubduedForeground", "Subdued Foreground");
		case ESlateColorStylingMode::UseColor_UseStyle: return LOCTEXT("SlateColor.UseStyle", "Style");
			break;
		default: return LOCTEXT("Unknown", "Unknown");
		}
	}

	TWeakPtr<sketch::TColorAttribute<T>> WeakAttribute;
	FSlateColor Color;

	TSharedPtr<SImage> TryConvertButtonImage;
	FImageColorAnimator Animator;
	TSharedPtr<SComboButton> ComboButton;
};

template <class T>
TSharedRef<SWidget> sketch::TColorAttribute<T>::MakeEditor()
{
	return SNew(SColorEditor<T>, *this);
}

template <class T>
FString sketch::TColorAttribute<T>::GenerateCode() const
{
	constexpr bool bSlateColor = std::is_same_v<T, FSlateColor>;
	if constexpr (bSlateColor)
	{
		const FAllSlateColorFields& Color = (const FAllSlateColorFields&)(Value);
		switch (Color.ColorUseRule)
		{
		case ESlateColorStylingMode::UseColor_ColorTable:
			{
				FString Result = TEXT("FSlateColor(EStyleColor::");
				Result += StaticEnum<EStyleColor>()->GetNameStringByValue(static_cast<int>(Color.ColorTableId));
				Result += TEXT(")");
				return Result;
			}
		case ESlateColorStylingMode::UseColor_Foreground:
			return TEXT("FSlateColor::UseForeground()");
		case ESlateColorStylingMode::UseColor_Foreground_Subdued:
			return TEXT("FSlateColor::UseSubduedForeground()");
		case ESlateColorStylingMode::UseColor_UseStyle:
			return TEXT("FSlateColor::UseStyle()");
		case ESlateColorStylingMode::UseColor_Specified:
		default:
			break;
		}
	}

	FLinearColor Color;
	if constexpr (std::is_same_v<FSlateColor, T>)
	{
		Color = Value.GetSpecifiedColor();
	}
	else
	{
		Color = Value;
	}
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

template struct sketch::TColorAttribute<FLinearColor>;
template struct sketch::TColorAttribute<FSlateColor>;

#undef LOCTEXT_NAMESPACE
