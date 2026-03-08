#include "AttributesTraites/BrushTraits.h"

#include "Sketch.h"
#include "SketchSandbox.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "Sketch.SBrushEditor"

static sketch::FHeaderToolAttributeFilter GBrushFilter([](FStringView Attribute)
{
	return Attribute == TEXT("const FSlateBrush*") || Attribute == TEXT("FSlateIcon");
});

static sketch::Sandbox::TItemInitializer<FSlateIcon> GSandboxBrush(TEXT("Brush"));

struct FBrushId
{
	FBrushId() = default;
	FBrushId(const FBrushId&) = default;
	FBrushId(FBrushId&&) = default;
	FBrushId& operator=(const FBrushId&) = default;
	FBrushId& operator=(FBrushId&&) = default;

	FBrushId(const ISlateStyle* InStyle, FName InName)
		: Style(InStyle)
		, Name(InName)
	{
		FString Result = Style ? Style->GetStyleSetName().ToString() : LOCTEXT("Invalid", "Invalid").ToString();
		Result += TEXT("::");
		Result += GetBrush()->GetImageSize().X ? TEXT("Image") : TEXT("Brush");
		Title = FText::FromString(MoveTemp(Result));
	}

	const ISlateStyle* Style = nullptr;
	FName Name;
	FText Title;

	const FSlateBrush* GetBrush() const { return Style ? Style->GetBrush(Name) : nullptr; }
	bool ContainsText(const FString& Text) const { return Title.ToString().Contains(Text) || Name.ToString().Contains(Text); }

	friend uint32 GetTypeHash(const FBrushId& Id) { return HashCombineFast(GetTypeHash(Id.Style), GetTypeHash(Id.Name)); }
	bool operator==(const FBrushId& Other) const { return Style == Other.Style && Name == Other.Name; }
};

template <>
struct TListTypeTraits<FBrushId>
{
public:
	typedef FBrushId NullableType;

	using MapKeyFuncs = TDefaultMapHashableKeyFuncs<FBrushId, TSharedRef<ITableRow>, false>;
	using MapKeyFuncsSparse = TDefaultMapHashableKeyFuncs<FBrushId, FSparseItemInfo, false>;
	using SetKeyFuncs = DefaultKeyFuncs<FBrushId>;

	template <typename U>
	static void AddReferencedObjects(
		FReferenceCollector& Collector,
		TArray<FBrushId>& ItemsWithGeneratedWidgets,
		TSet<FBrushId>& SelectedItems,
		TMap<const U*, FBrushId>& WidgetToItemMap
	)
	{
	}

	static bool IsPtrValid(const FBrushId& InValue)
	{
		return InValue.Style && InValue.Name != NAME_None;
	}

	static void ResetPtr(FBrushId& InValue)
	{
		InValue.Style = nullptr;
		InValue.Name = NAME_None;
	}

	static FBrushId MakeNullPtr()
	{
		return {};
	}

	static FBrushId NullableItemTypeConvertToItemType(const FBrushId& InValue)
	{
		return InValue;
	}

	static FString DebugDump(FBrushId InValue)
	{
		FString Result = InValue.Title.ToString() + TEXT("::") + InValue.Name.ToString();
		return Result;
	}

	class SerializerType
	{
	};
};

template <>
struct TIsValidListItem<FBrushId>
{
	enum
	{
		Value = true
	};
};

const TArray<FBrushId>& GetBrushes()
{
	static TArray<FBrushId> Brushes;
	if (Brushes.IsEmpty())
	[[unlikely]]
	{
		Brushes.Reserve(1024 * 6); // There's about 5500 out of box
		auto GatherBrushes = [](const ISlateStyle& StyleInterface)-> bool
		{
			class FStyle : public FSlateStyleSet
			{
			public:
				using FSlateStyleSet::BrushResources;
			};
			const auto& Style = static_cast<const FStyle&>(StyleInterface);
			for (const auto& [Name, _] : Style.BrushResources)
			{
				Brushes.Emplace(&StyleInterface, Name);
			}
			return true;
		};
		FSlateStyleRegistry::IterateAllStyles(GatherBrushes);
	}
	return Brushes;
}

template <class T>
class SIdList : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SWidget>, FOnGenerateRow, const T&, TAttribute<FText>&& TextToHighlight);
	DECLARE_DELEGATE_OneParam(FOnIdSelected, const T&);

	SLATE_BEGIN_ARGS(SIdList)
		{
		}

		SLATE_ITEMS_SOURCE_ARGUMENT(T, Items)
		SLATE_EVENT(FOnGenerateRow, OnGenerateRow)
		SLATE_EVENT(FOnIdSelected, OnIdSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Items = Args.GetItems().ArrayPointer;
		OnGenerateRow = Args._OnGenerateRow;
		OnIdSelected = Args._OnIdSelected;

		ChildSlot
		[
			SNew(SBox)
			.MinDesiredWidth(700.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.f, 4.f)
				[
					SAssignNew(SearchBox, SSearchBox)
					.OnTextChanged(this, &SIdList::OnFilterChanged)
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.)
				[
					SAssignNew(List, SListView<T>)
					.ListItemsSource(Args.GetItems())
					.OnGenerateRow(this, &SIdList::GenerateRow)
					.OnSelectionChanged(this, &SIdList::OnSelectionChanged)
				]
			]
		];
	}

	void SetUserFocus()
	{
		RegisterActiveTimer(0., FWidgetActiveTimerDelegate::CreateSPLambda(this, [this](double, float)
		{
			FSlateApplication::Get().SetAllUserFocus(SearchBox, EFocusCause::WindowActivate);
			return EActiveTimerReturnType::Stop;
		}));
	}

private:
	void OnFilterChanged(const FText& FilterText)
	{
		if (FilterText.IsEmpty())
		{
			List->SetItemsSource(Items);
		}
		else
		{
			FilteredItems.Reset();
			for (const T& Item : *Items)
			{
				if (Item.ContainsText(FilterText.ToString()))
				{
					FilteredItems.Add(Item);
				}
			}
			List->SetItemsSource(&FilteredItems);
			List->RequestListRefresh();
		}
	}

	TSharedRef<ITableRow> GenerateRow(T Item, const TSharedRef<STableViewBase>& Owner)
	{
		return SNew(STableRow<T>, Owner)
			.Padding(FMargin{ 8.f, 4.f })
			[
				OnGenerateRow.Execute(Item, TAttribute<FText>::CreateSP(SearchBox.Get(), &SSearchBox::GetText))
			];
	}

	void OnSelectionChanged(T Item, ESelectInfo::Type SelectionType)
	{
		if (SelectionType == ESelectInfo::OnNavigation)
			return;

		OnIdSelected.Execute(Item);
		FSlateApplication::Get().DismissAllMenus();
	}

	const TArray<T>* Items = nullptr;
	TArray<T> FilteredItems;

	TSharedPtr<SListView<T>> List;
	TSharedPtr<SSearchBox> SearchBox;

	FOnGenerateRow OnGenerateRow;
	FOnIdSelected OnIdSelected;
};

class SBrushEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBrushEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, sketch::FBrushAttribute& Attribute)
	{
		WeakAttribute = Attribute.AsWeakSubobject(&Attribute);
		auto Menu = SNew(SIdList<FBrushId>)
			.Items(&GetBrushes())
			.OnIdSelected(this, &SBrushEditor::OnBrushSelected)
			.OnGenerateRow_Static(
				[](const FBrushId& Brush, TAttribute<FText>&& TextToHighlight) -> TSharedRef<SWidget>
				{
					TSharedPtr<SWidget> Icon;
					if (Brush.GetBrush()->GetImageSize().X)
					{
						Icon = SNew(SImage).Image(Brush.GetBrush());
					}
					else
					{
						Icon = SNew(SBox).WidthOverride(32.f).HeightOverride(32.f)
						[
							SNew(SBorder).BorderImage(Brush.GetBrush())
						];
					}
					FColor StyleNameColor(GetTypeHash(Brush.Style->GetStyleSetName()));
					StyleNameColor.A = 255;

					return SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(Brush.Title)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5))
								.HighlightText(TextToHighlight)
								.ColorAndOpacity(StyleNameColor)
							]

							+ SVerticalBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::FromName(Brush.Name))
								.HighlightText(MoveTemp(TextToHighlight))
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							Icon.ToSharedRef()
						];
				}
			);
		ChildSlot
		[
			SNew(SComboButton)
			.ButtonContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ImagePreview, SImage)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					// Use SBox instead of slot padding 'cause slot padding space is, somewhy, taken from other slots
					SNew(SBox)
					.Padding(4.f, 0.f)
					[
						SAssignNew(ImageSizePreview, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(16.f)
					.HeightOverride(16.f)
					[
						SAssignNew(BrushPreview, SBorder)
					]
				]
			]
			.OnComboBoxOpened(Menu, &SIdList<FBrushId>::SetUserFocus)
			.MenuContent()
			[
				Menu
			]
		];
		UpdateBrushPreview(Attribute.Brush);
	}

private:
	void UpdateBrushPreview(const FSlateBrush* Brush)
	{
		const bool bImage = Brush && Brush->GetImageSize().X > 0;
		ImagePreview->SetVisibility(bImage ? EVisibility::Visible : EVisibility::Collapsed);
		ImageSizePreview->SetVisibility(bImage ? EVisibility::Visible : EVisibility::Collapsed);
		BrushPreview->SetVisibility(bImage ? EVisibility::Collapsed : EVisibility::Visible);
		if (bImage)
		{
			ImagePreview->SetImage(Brush);
			ImageSizePreview->SetText(FText::FromString(FString::FromInt(Brush->GetImageSize().X) + TEXT("x") + FString::FromInt(Brush->GetImageSize().Y)));
		}
		else
		{
			BrushPreview->SetBorderImage(Brush);
		}
	}

	void OnBrushSelected(const FBrushId& Brush)
	{
		if (TSharedPtr<sketch::FBrushAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
		{
			Attribute->Brush = Brush.GetBrush();
			Attribute->StyleName = Brush.Style ? Brush.Style->GetStyleSetName() : NAME_None;
			Attribute->BrushName = Brush.Name;
			Attribute->OnValueChanged.Broadcast();
		}
		UpdateBrushPreview(Brush.GetBrush());
	}

	TWeakPtr<sketch::FBrushAttribute> WeakAttribute;
	TSharedPtr<SImage> ImagePreview;
	TSharedPtr<SBorder> BrushPreview;
	TSharedPtr<STextBlock> ImageSizePreview;
};

TSharedRef<SWidget> sketch::FBrushAttribute::MakeEditor()
{
	return SNew(SBrushEditor, *this);
}

FString sketch::FBrushAttribute::GenerateCode() const
{
	return GenerateBaseCode() + TEXT(".GetBrush()");
}

bool sketch::FBrushAttribute::Equals(const IAttributeImplementation& InOther) const
{
	const FBrushAttribute& Other = static_cast<const FBrushAttribute&>(InOther);
	return StyleName == Other.StyleName && BrushName == Other.BrushName && Brush == Other.Brush;
}

void sketch::FBrushAttribute::Reinitialize(const IAttributeImplementation& From)
{
	const FBrushAttribute& Other = static_cast<const FBrushAttribute&>(From);
	Brush = Other.Brush;
	StyleName = Other.StyleName;
	BrushName = Other.BrushName;
}

sketch::FBrushAttribute::FBrushAttribute(const FSlateBrush* InBrush)
	: Brush(InBrush)
{
	if (Brush)
	{
		for (const FBrushId& SomeBrush : GetBrushes())
		{
			if (SomeBrush.GetBrush() == Brush)
			[[unlikely]]
			{
				StyleName = SomeBrush.Style->GetStyleSetName();
				BrushName = SomeBrush.Name;
				break;
			}
		}
	}
}

FString sketch::FBrushAttribute::GenerateBaseCode() const
{
	FString Result = TEXT("FSlateIcon(");
	if (StyleName == FAppStyle::GetAppStyleSetName())
	{
		Result += TEXT("FAppStyle::GetAppStyleSetName()");
	}
	else
	{
		Result += TEXT("\"");
		Result += StyleName.ToString();
		Result += TEXT("\"");
	}
	Result += TEXT(", \"");
	Result += BrushName.ToString();
	Result += TEXT("\")");
	return Result;
}

FString sketch::FIconAttribute::GenerateCode() const
{
	return GenerateBaseCode();
}
