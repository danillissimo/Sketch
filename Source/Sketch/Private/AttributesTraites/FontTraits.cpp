#include "AttributesTraites/FontTraits.h"

#include "Sketch.h"
#include "SketchSandbox.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Views/STreeView.h"
#include "SIdList.h"

#define LOCTEXT_NAMESPACE "Sketch.SFontEditor"

static sketch::FHeaderToolAttributeFilter GFontFilter([](FStringView Attribute)
{
	return Attribute == TEXT("FSlateFontInfo");
});

static sketch::Sandbox::TItemInitializer<FSlateFontInfo> GSandboxFont(TEXT("Font"));

struct FFontId
{
	const ISlateStyle* Style = nullptr;
	FName Name;

	FSlateFontInfo GetFont() const { return Style ? Style->GetFontStyle(Name) : FSlateFontInfo(); }

	FSlateFontInfo GetFont(float Size) const
	{
		FSlateFontInfo Font = GetFont();
		Font.Size = FMath::Clamp<float>(Size, 0.f, std::numeric_limits<uint16>::max());
		return Font;
	}

	FString ToString() const
	{
		FString Result = Style ? Style->GetStyleSetName().ToString() : LOCTEXT("Invalid", "Invalid").ToString();
		Result += TCHAR('.');
		Name.AppendString(Result);
		return Result;
	}

	bool ContainsText(const FString& Text) const { return Name.ToString().Contains(Text); }

	friend uint32 GetTypeHash(const FFontId& Id) { return HashCombineFast(GetTypeHash(Id.Style), GetTypeHash(Id.Name)); }
	bool operator==(const FFontId& Other) const { return Style == Other.Style && Name == Other.Name; }
};

template <>
struct TListTypeTraits<FFontId>
{
public:
	typedef FFontId NullableType;

	using MapKeyFuncs = TDefaultMapHashableKeyFuncs<FFontId, TSharedRef<ITableRow>, false>;
	using MapKeyFuncsSparse = TDefaultMapHashableKeyFuncs<FFontId, FSparseItemInfo, false>;
	using SetKeyFuncs = DefaultKeyFuncs<FFontId>;

	template <typename U>
	static void AddReferencedObjects(
		FReferenceCollector& Collector,
		TArray<FFontId>& ItemsWithGeneratedWidgets,
		TSet<FFontId>& SelectedItems,
		TMap<const U*, FFontId>& WidgetToItemMap
	)
	{
	}

	static bool IsPtrValid(const FFontId& InValue)
	{
		return InValue.Style && InValue.Name != NAME_None;
	}

	static void ResetPtr(FFontId& InValue)
	{
		InValue.Style = nullptr;
		InValue.Name = NAME_None;
	}

	static FFontId MakeNullPtr()
	{
		return {};
	}

	static FFontId NullableItemTypeConvertToItemType(const FFontId& InValue)
	{
		return InValue;
	}

	static FString DebugDump(FFontId InValue)
	{
		return InValue.ToString();
	}

	class SerializerType
	{
	};
};

template <>
struct TIsValidListItem<FFontId>
{
	enum
	{
		Value = true
	};
};

const TArray<FFontId>& GetFonts()
{
	static TArray<FFontId> Fonts;
	if (Fonts.IsEmpty())
	[[unlikely]]
	{
		Fonts.Reserve(256);
		auto GatherFonts = [](const ISlateStyle& StyleInterface)-> bool
		{
			class FStyle : public FSlateStyleSet
			{
			public:
				using FSlateStyleSet::FontInfoResources;
			};
			const auto& Style = static_cast<const FStyle&>(StyleInterface);
			for (const auto& [Name, _] : Style.FontInfoResources)
			{
				Fonts.Emplace(&StyleInterface, Name);
			}
			return true;
		};
		FSlateStyleRegistry::IterateAllStyles(GatherFonts);
	}
	return Fonts;
}

class SFontEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFontEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, sketch::FFontAttribute& Attribute)
	{
		WeakAttribute = Attribute.AsWeakSubobject(&Attribute);

		// Locate current font
		{
			for (const FFontId& SomeFont : GetFonts())
			{
				if (SomeFont.GetFont() == Attribute.FontInfo)
				{
					Font = SomeFont;
					break;
				}
			}
		}

		auto Menu = SNew(SIdList<FFontId>)
			.Items(&GetFonts())
			.OnIdSelected(this, &SFontEditor::OnFontSelected)
			.OnGenerateRow_Static(
				[](const FFontId& FontId, TAttribute<FText>&& TextToHighlight) -> TSharedRef<SWidget>
				{
					return SNew(STextBlock)
						.Text(FText::FromString(FontId.ToString()))
						.Font(FontId.GetFont())
						.OverflowPolicy(ETextOverflowPolicy::Clip)
						.HighlightText(MoveTemp(TextToHighlight));
				}
			);

		// Make content
		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(Button, SComboButton)
				.ButtonContent()
				[
					SAssignNew(ButtonCaption, STextBlock)
					.Text(FText::FromString(Font.ToString()))
					.Font(Font.GetFont())
				]
				.OnComboBoxOpened(Menu, &SIdList<FFontId>::SetUserFocus)
				.MenuContent()
				[
					MoveTemp(Menu)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(FontSize, SSpinBox<float>)
				.Delta(1)
				.Value(Font.GetFont().Size)
				.OnValueChanged(this, &SFontEditor::OnFontSizeChanged)
			]
		];
	}

private:
	void OnFontSelected(const FFontId& NewFont)
	{
		// Update self
		Font = NewFont;

		// Update visual state
		if (TSharedPtr<SWindow> Menu = Button->GetMenuWindow())
		[[likely]]
		{
			Menu->RequestDestroyWindow();
		}
		ButtonCaption->SetText(FText::FromString(Font.ToString()));
		FSlateFontInfo FontInfo = Font.GetFont();
		ButtonCaption->SetFont(FontInfo);
		FontSize->SetValue(FontInfo.Size); // This triggers OnFontSizeChanged which does the update
	}

	void OnFontSizeChanged(float NewSize)
	{
		ButtonCaption->SetFont(Font.GetFont(FontSize->GetValue()));

		// Update attribute
		if (TSharedPtr<sketch::FFontAttribute> Attribute = WeakAttribute.Pin())[[likely]]
		{
			Attribute->FontInfo = Font.GetFont(NewSize);
			Attribute->StyleName = Font.Style ? Font.Style->GetStyleSetName() : NAME_None;
			Attribute->FontName = Font.Name;
			Attribute->OnValueChanged.Broadcast();
		}
	}

	TWeakPtr<sketch::FFontAttribute> WeakAttribute;
	FFontId Font;
	TSharedPtr<SComboButton> Button;
	TSharedPtr<STextBlock> ButtonCaption;
	TSharedPtr<SSpinBox<float>> FontSize;
};

TSharedRef<SWidget> sketch::FFontAttribute::MakeEditor()
{
	return SNew(SFontEditor, *this);
}

FString sketch::FFontAttribute::GenerateCode() const
{
	const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(StyleName);
	if (!Style)
	[[unlikely]]
	{
		return {};
	}

	FString Result;
	FSlateFontInfo BaseFont = Style->GetFontStyle(FontName);
	TSharedRef<const FCompositeFont> DefaultFontSample = FCoreStyle::GetDefaultFont();
	if (BaseFont.CompositeFont == DefaultFontSample)
	{
		Result = FString::Format(
			TEXT("FCoreStyle::GetDefaultFontStyle(\"{0}\", {1})"), {
				BaseFont.TypefaceFontName.ToString(),
				FString::SanitizeFloat(FontInfo.Size, 0),
			}
		);
	}
	else if (FontInfo.Size == BaseFont.Size)
	{
		Result = FString::Format(
			TEXT("FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\")"), {
				StyleName.ToString(),
				FontName.ToString(),
			}
		);
	}
	else
	{
		// Sigh, there's no adequate constructor for such cases
		Result = FString::Format(
			TEXT("[]{ auto Font = FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\"); Font.Size = {2}; return Font; }()")
			, {
				StyleName.ToString(),
				FontName.ToString(),
				FString::SanitizeFloat(FontInfo.Size, 0),
			}
		);
	}
	return Result;
}

bool sketch::FFontAttribute::Equals(const IAttributeImplementation& InOther) const
{
	const FFontAttribute& Other = static_cast<const FFontAttribute&>(InOther);
	return StyleName == Other.StyleName && FontName == Other.FontName && FontInfo == Other.FontInfo;
}

void sketch::FFontAttribute::Reinitialize(const IAttributeImplementation& From)
{
	const FFontAttribute& Other = static_cast<const FFontAttribute&>(From);
	FontInfo = Other.FontInfo;
	StyleName = Other.StyleName;
	FontName = Other.FontName;
}

sketch::FFontAttribute::FFontAttribute(FSlateFontInfo&& InInfo)
// All text widgets uses fallback font if no font is specified explicitly
// No font at all - is an explicit font as well. It's like setting TOptional<FSlateFontInfo*> to nullptr
// So when no font is provided - use default one
	: FontInfo(InInfo.TypefaceFontName.IsNone() ? FCoreStyle::GetDefaultFontStyle("Regular", 10) : MoveTemp(InInfo))
{
	for (const FFontId& SomeFont : GetFonts())
	{
		if (SomeFont.GetFont() == FontInfo)
		[[unlikely]]
		{
			StyleName = SomeFont.Style->GetStyleSetName();
			FontName = SomeFont.Name;
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE
