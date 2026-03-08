#include "AttributesTraites/FontTraits.h"

#include "Sketch.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "Sketch.SFontEditor"

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

	friend uint32 GetTypeHash(const FFontId& Id) { return HashCombineFast(GetTypeHash(Id.Style), GetTypeHash(Id.Name)); }
	bool operator==(const FFontId& Other) const { return Style == Other.Style && Name == Other.Name; }
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

sketch::FFont::FFont(const FSlateFontInfo& Info)
	: FontInfo(Info)
{
	for (const FFontId& SomeFont : GetFonts())
	{
		if (SomeFont.GetFont() == FontInfo)
		[[unlikely]]
		{
			StyleName = SomeFont.Style->GetStyleSetName();
			FontName = SomeFont.Name;
			break;
		}
	}
}

template <>
struct TIsValidListItem<FFontId>
{
	enum
	{
		Value = true
	};
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

class SFontEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFontEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const sketch::FAttributeHandle& InHandle)
	{
		Handle = InHandle;

		// Locate current font and finalize its initialization
		{
			sketch::FFont& Initializer = Handle->GetValueChecked<sketch::FFont>();
			for (const FFontId& SomeFont : GetFonts())
			{
				if (SomeFont.GetFont() == Initializer.FontInfo)
				{
					Font = SomeFont;
					Initializer.StyleName = SomeFont.Style->GetStyleSetName();
					Initializer.FontName = SomeFont.Name;
					break;
				}
			}
		}

		// Make content
		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(Button, SComboButton)
				.ButtonContent()
				[
					SAssignNew(ButtonCaption, STextBlock)
					.Text(FText::FromString(Font.ToString()))
					.Font(Font.GetFont())
				]
				.MenuContent()
				[
					SNew(SBox)
					.MaxDesiredWidth(400)
					[
						SNew(SListView<FFontId>)
						.ListItemsSource(&GetFonts())
						.OnGenerateRow_Static(
							[](FFontId FontId, const TSharedRef<STableViewBase>& List)-> TSharedRef<ITableRow>
							{
								return SNew(STableRow<FFontId>, List)
									.Padding(4)
									[
										SNew(STextBlock)
										.Text(FText::FromString(FontId.ToString()))
										.Font(FontId.GetFont())
										.OverflowPolicy(ETextOverflowPolicy::Clip)
									];
							}
						)
						.OnSelectionChanged(this, &SFontEditor::OnFontSelected)
					]
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
	void OnFontSelected(FFontId NewFont, ESelectInfo::Type SelectInfo)
	{
		// Make sure it's a selection
		if (SelectInfo == ESelectInfo::Type::OnNavigation)
			return;

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
		Handle.SetValue<sketch::FFont>(
			{
				Font.GetFont(NewSize),
				Font.Style->GetStyleSetName(),
				Font.Name,
			}
		);
	}

	sketch::FAttributeHandle Handle;
	FFontId Font;
	TSharedPtr<SComboButton> Button;
	TSharedPtr<STextBlock> ButtonCaption;
	TSharedPtr<SSpinBox<float>> FontSize;
};

// template <>
// TSharedRef<SWidget> sketch::MakeEditor<sketch::FFont>(const FAttributeHandle& Handle)
// {
// 	return SNew(SFontEditor, Handle);
// }
//
// template <>
// FString sketch::GenerateCode<sketch::FFont>(const FFont& Font)
// {
// 	const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(Font.StyleName);
// 	if (!Style)
// 	[[unlikely]]
// 	{
// 		return {};
// 	}
//
// 	FString Result;
// 	FSlateFontInfo BaseFont = Style->GetFontStyle(Font.FontName);
// 	TSharedRef<const FCompositeFont> DefaultFontSample = FCoreStyle::GetDefaultFont();
// 	if (BaseFont.CompositeFont == DefaultFontSample)
// 	{
// 		Result = FString::Format(
// 			TEXT("FCoreStyle::GetDefaultFontStyle(\"{0}\", {1})"), {
// 				BaseFont.TypefaceFontName.ToString(),
// 				FString::SanitizeFloat(Font.FontInfo.Size, 0),
// 			}
// 		);
// 	}
// 	else if (Font.FontInfo.Size == BaseFont.Size)
// 	{
// 		Result = FString::Format(
// 			TEXT("FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\")"), {
// 				Font.StyleName.ToString(),
// 				Font.FontName.ToString(),
// 			}
// 		);
// 	}
// 	else
// 	{
// 		// Sigh, there's no adequate constructor for such cases
// 		Result = FString::Format(
// 			TEXT("[]{ auto Font = FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\"); Font.Size = {2}; return Font; }()")
// 			, {
// 				Font.StyleName.ToString(),
// 				Font.FontName.ToString(),
// 				FString::SanitizeFloat(Font.FontInfo.Size, 0),
// 			}
// 		);
// 	}
// 	return Result;
// }


TSharedRef<SWidget> sketch::TAttributeTraits<FSlateFontInfo>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SFontEditor, Handle);
}

FString sketch::TAttributeTraits<FSlateFontInfo>::GenerateCode(const FFont& Font)
{
	const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(Font.StyleName);
	if (!Style)
	[[unlikely]]
	{
		return {};
	}

	FString Result;
	FSlateFontInfo BaseFont = Style->GetFontStyle(Font.FontName);
	TSharedRef<const FCompositeFont> DefaultFontSample = FCoreStyle::GetDefaultFont();
	if (BaseFont.CompositeFont == DefaultFontSample)
	{
		Result = FString::Format(
			TEXT("FCoreStyle::GetDefaultFontStyle(\"{0}\", {1})"), {
				BaseFont.TypefaceFontName.ToString(),
				FString::SanitizeFloat(Font.FontInfo.Size, 0),
			}
		);
	}
	else if (Font.FontInfo.Size == BaseFont.Size)
	{
		Result = FString::Format(
			TEXT("FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\")"), {
				Font.StyleName.ToString(),
				Font.FontName.ToString(),
			}
		);
	}
	else
	{
		// Sigh, there's no adequate constructor for such cases
		Result = FString::Format(
			TEXT("[]{ auto Font = FSlateStyleRegistry::FindSlateStyle(\"{0}\")->GetFontStyle(\"{1}\"); Font.Size = {2}; return Font; }()")
			, {
				Font.StyleName.ToString(),
				Font.FontName.ToString(),
				FString::SanitizeFloat(Font.FontInfo.Size, 0),
			}
		);
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
