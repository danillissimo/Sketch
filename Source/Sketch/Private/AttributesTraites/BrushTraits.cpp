#include "AttributesTraites/BrushTraits.h"

#include "Sketch.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "Sketch.SBrushEditor"

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
struct TIsValidListItem<FBrushId>
{
	enum
	{
		Value = true
	};
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

sketch::FBrush::FBrush(const FSlateBrush* InBrush)
	: Brush(InBrush)
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
		RegisterActiveTimer(0., FWidgetActiveTimerDelegate::CreateSP(this, &SIdList::GetFocus));
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
		}
	}

	TSharedRef<ITableRow> GenerateRow(T Item, const TSharedRef<STableViewBase>& Owner)
	{
		return SNew(STableRow<T>, Owner)
			.Padding(FMargin{8.f, 4.f})
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

	EActiveTimerReturnType GetFocus(double CurrentTime, float DeltaTime)
	{
		FSlateApplication::Get().SetAllUserFocus(SearchBox, EFocusCause::WindowActivate);
		return EActiveTimerReturnType::Stop;
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

	void Construct(const FArguments& InArgs, const sketch::FAttributeHandle& InHandle)
	{
		Handle = InHandle;
		ChildSlot
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &SBrushEditor::ListBrushes)
			.ButtonContent()
			[
				SAssignNew(TitleIcon, SImage)
				.Image(Handle.GetValue<sketch::FBrush>().Brush)
			]
		];
	}

private:
	TSharedRef<SWidget> ListBrushes()
	{
		return SNew(SIdList<FBrushId>)
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
	}

	void OnBrushSelected(const FBrushId& Brush)
	{
		Handle.SetValue<sketch::FBrush>(
			sketch::FBrush(
				Brush.GetBrush(),
				Brush.Style->GetStyleSetName(),
				Brush.Name
			)
		);
		TitleIcon->SetImage(Brush.GetBrush());
	}

	sketch::FAttributeHandle Handle;
	TSharedPtr<SImage> TitleIcon;
};

TSharedRef<SWidget> sketch::TAttributeTraits<const FSlateBrush*>::MakeEditor(const FAttributeHandle& Handle)
{
	return SNew(SBrushEditor, Handle);
}

FString sketch::TAttributeTraits<const FSlateBrush*>::GenerateCode(const FBrush& Brush)
{
	if (Brush.StyleName == FAppStyle::GetAppStyleSetName())
	{
		return FString::Printf(TEXT("FSlateIcon(FAppStyle::GetAppStyleSetName(), \"%s\").GetIcon()"), *Brush.BrushName.ToString());
	}
	else
	{
		return FString::Printf(TEXT("FSlateIcon(\"%s\", \"%s\").GetIcon()"), *Brush.StyleName.ToString(), *Brush.BrushName.ToString());
	}
}
