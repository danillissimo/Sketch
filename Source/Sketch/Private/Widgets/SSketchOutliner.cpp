#include "Widgets/SSketchOutliner.h"

#include "SketchCore.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchWidget.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SSketchOutliner"

void SSketchOutliner::Construct(const FArguments& InArgs)
{
	SetRoot(InArgs._Root);
	if (InArgs._AttributeCollection)
		WeakCollection = StaticCastWeakPtr<SSketchAttributeCollection>(InArgs._AttributeCollection->AsWeak());
}

void SSketchOutliner::SetRoot(SSketchWidget* Root)
{
	if (!RootAsArray.IsEmpty())
	{
		if (TSharedPtr<SSketchWidget> OldRoot = RootAsArray[0].Pin())
		{
			OldRoot->OnModification.Remove(RootListener);
			RootListener.Reset();
		}
	}
	RootAsArray.Empty(1);
	if (Root)
	{
		RootAsArray.Add(SharedThis(Root).ToWeakPtr());
		RootListener = Root->OnModification.AddSP(this, &SSketchOutliner::OnSketchUpdated);
	}
	Rebuild();
	OnSelectionChanged(nullptr, ESelectInfo::Direct);
}

void SSketchOutliner::Rebuild()
{
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(Hint, STextBlock)
			.Text(LOCTEXT("EditHint", "Note: right click row to edit it"))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 6.75))
			.ColorAndOpacity(FLinearColor{ 1, 1, 1, 0.5 })
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoAttachedWidget", "No attached widget"))
			.Visibility(RootAsArray.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed)
		]

		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(Tree, STreeView<TWeakPtr<SSketchWidget>>)
			.TreeItemsSource(&RootAsArray)
			.OnGetChildren_Static(&SSketchOutliner::OnGetChildren)
			.OnGenerateRow(this, &SSketchOutliner::OnGenerateRow)
			.OnSelectionChanged(this, &SSketchOutliner::OnSelectionChanged)
			.OnContextMenuOpening(this, &SSketchOutliner::OnMakeContextMenu)
		]
	];
}

void SSketchOutliner::OnGetChildren(TWeakPtr<SSketchWidget> InItem, TArray<TWeakPtr<SSketchWidget>>& Children)
{
	TSharedPtr<SSketchWidget> Item = InItem.Pin();
	if (!Item) [[unlikely]] return;

	// Gather all slots
	sketch::FFactory::FUniqueSlots UniqueSlots = Item->CollectUniqueSlots();
	auto& Slots = Item->GetDynamicSlots();
	Children.Reserve(UniqueSlots.Num() + Slots.Num());

	// Enumerate unique slots first as they are persistent
	for (SSketchWidget* Slot : UniqueSlots)
	{
		check(Slot);
		Children.Emplace(StaticCastWeakPtr<SSketchWidget>(Slot->AsWeak()));
	}

	// Enumerate dynamic slots
	for (auto TypedSlots = Item->GetDynamicSlots().CreateConstIterator(); TypedSlots; ++TypedSlots)
	{
		for (const SSketchWidget::FSlot& Slot : TypedSlots.Value())
		{
			Children.Emplace(Slot.Widget);
		}
	}
}

class SSketchOutlinerRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchOutlinerRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, SSketchOutliner* Owner, TWeakPtr<SSketchWidget> InItem)
	{
		// Sanitize
		TSharedPtr<SSketchWidget> Item = InItem.Pin();
		if (!Item) [[unlikely]]
		{
			ChildSlot
			[
				SNew(SBox)
				.VAlign(VAlign_Center)
				.Content()
				[
					SNew(STextBlock)
					.Text(INVTEXT("ERROR"))
				]

			];
			return;
		}

		// Determine factory name
		const sketch::FFactoryHandle& FactoryHandle = Item->GetContentFactory();
		FString Name;
		Name.Reserve(64);
		const FName UniqueSlotName = Item->GetTag();
		if (!UniqueSlotName.IsNone())
		{
			UniqueSlotName.AppendString(Name);
			Name += TEXT(": ");
		}
		FactoryHandle.Name.AppendString(Name);

		// Make "add slot" button
		FName SlotType;
		if (const sketch::FFactory* Factory = FactoryHandle.Resolve(); Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
		{
			sketch::FFactory::FDynamicSlotTypes SlotTypes = Factory->EnumerateDynamicSlotTypes(Item->GetContent());
			if (!SlotTypes.IsEmpty())
			{
				SlotType = SlotTypes[0];
			}
		}
		TSharedRef<SMenuAnchor> MenuAnchor =
			SNew(SMenuAnchor)
			.OnGetMenuContent(Owner, &SSketchOutliner::MakeNewSlotMenu, InItem, SlotType)
			.Visibility(SlotType.IsNone() ? EVisibility::Hidden : EVisibility::Visible);
		constexpr FLinearColor ThreeThirdsWhite{ 1, 1, 1, 0.75 };
		TSharedRef<SButton> AddSlotButton =
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.ToolTipText(LOCTEXT("AddSlot", "Add slot"))
			.OnClicked_Lambda([WeakMenuAnchor = SharedThis(&MenuAnchor.Get()).ToWeakPtr()]()-> FReply
			{
				if (TSharedPtr<SMenuAnchor> MenuAnchor = WeakMenuAnchor.Pin()) [[likely]]
				{
					MenuAnchor->SetIsOpen(true);
				}
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FSlateIcon("CoreStyle", "Icons.PlusCircle").GetIcon())
				.ColorAndOpacity(ThreeThirdsWhite)
			];
		MenuAnchor->SetContent(MoveTemp(AddSlotButton));

		// Determine owning slot type
		SSketchWidget::FSlotReference OwningSlot;
		SSketchWidget* Parent = Item->GetParent();
		FOnClicked OnRemoveSlot;
		if (Parent)
		{
			OwningSlot = Parent->FindSlotFor(Item.Get());
			OnRemoveSlot.BindSP(Owner, &SSketchOutliner::OnRemoveSlotWithReply, Parent->AsWeakSubobject(Parent), OwningSlot.Type, OwningSlot.Index);
		}

		// Make full widget
		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MoveTemp(Name)))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(Controls, SHorizontalBox)
				.Visibility(EVisibility::Hidden)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ToolTipText(LOCTEXT("RemoveSlot", "Remove slot"))
					.OnClicked(MoveTemp(OnRemoveSlot))
					.Visibility(OwningSlot.Data ? EVisibility::Visible : EVisibility::Hidden)
					[
						SNew(SImage)
						.Image(FSlateIcon("CoreStyle", "GenericCommands.Delete").GetIcon())
						.ColorAndOpacity(ThreeThirdsWhite)
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					MoveTemp(MenuAnchor)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ToolTipText(LOCTEXT("CopyToClipboard", "Copy to clipboard"))
					.OnClicked_Static(&SSketchOutliner::OnExportRow, InItem)
					[
						SNew(SImage)
						.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Export").GetIcon())
					]
				]
			]
		];
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		Controls->SetVisibility(EVisibility::Visible);
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		Controls->SetVisibility(EVisibility::Hidden);
	}

private:
	TSharedPtr<SWidget> Controls;
};

TSharedRef<ITableRow> SSketchOutliner::OnGenerateRow(TWeakPtr<SSketchWidget> InItem, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TWeakPtr<SSketchWidget>>, Owner)
		[
			SNew(SSketchOutlinerRow, this, InItem)
		];
}

FReply SSketchOutliner::OnExportRow(TWeakPtr<SSketchWidget> InItem)
{
	TSharedPtr<SSketchWidget> Item = InItem.Pin();
	if (Item) [[likely]]
	{
		FString Code = Item->GenerateCode();
		FPlatformApplicationMisc::ClipboardCopy(*Code);
	}
	return FReply::Handled();
}

void SSketchOutliner::OnSelectionChanged(TWeakPtr<SSketchWidget> InItem, ESelectInfo::Type SelectionType)
{
	if (TSharedPtr<SSketchWidget> SelectedWidget = WeakSelectedWidget.Pin())
	{
		SelectedWidget->Unhighlight();
		WeakSelectedWidget.Reset();
	}

	TSharedPtr<SSketchAttributeCollection> Collection = WeakCollection.Pin();
	TSharedPtr<SSketchWidget> Item = InItem.Pin();
	if (!Item)
	{
		if (Collection)
		{
			Collection->SetSlotAttributes({});
			Collection->SetAttributes({});
		}
		return;
	}

	{
		// Update slot attribute if any
		auto DisplaySlotAttributes = [&]
		{
			if (Item->IsRoot()) return false;
			SSketchWidget* SlotContainer = Item->GetParent();
			if (!SlotContainer) return false;
			SSketchWidget::FSlot* Slot = SlotContainer->FindSlotFor(Item.Get()).Data;
			if (!Slot) return false;
			Collection->SetSlotAttributes(Slot->Attributes);
			return true;
		};
		if (!DisplaySlotAttributes())[[unlikely]]
			Collection->SetSlotAttributes({});

		// Update widget attributes
		Collection->SetAttributes(Item->AsSharedSubobject(&Item->GetAttributes()));
	}

	Item->Highlight();
	WeakSelectedWidget = Item;
}

TSharedPtr<SWidget> SSketchOutliner::OnMakeContextMenu()
{
	// Remove hint
	Hint->SetVisibility(EVisibility::Collapsed);

	// Sanitize
	TArray<TWeakPtr<SSketchWidget>> SelectedItems = Tree->GetSelectedItems();
	if (SelectedItems.Num() != 1) [[unlikely]] return nullptr;
	TSharedPtr<SSketchWidget> Item = SelectedItems[0].Pin();
	if (!Item) [[unlikely]] return nullptr;

	// List slot controls if content is present and supports slots
	FMenuBuilder Menu(true, nullptr);
	if (const sketch::FFactory* Factory = Item->GetContentFactory().Resolve(); Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
	{
		// First list new slot controls
		static const FName NAME_Slots = TEXT("Slots");
		Menu.BeginSection(NAME_Slots, LOCTEXT("Slots", "Slots"));
		const sketch::FFactory::FDynamicSlotTypes SlotTypes = Factory->EnumerateDynamicSlotTypes(Item->GetContent());
		for (const FName& SlotType : SlotTypes)
		{
			FMenuEntryParams Entry;
			FString Label = TEXT("New ");
			SlotType.AppendString(Label);
			Entry.LabelOverride = FText::FromString(Label);
			Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnMakeEmptySlot, SelectedItems[0], SlotType);
			Entry.EntryBuilder.BindSP(this, &SSketchOutliner::MakeNewSlotMenu, SelectedItems[0], SlotType);
			Entry.bIsSubMenu = true;

			Menu.AddMenuEntry(Entry);
		}
		Menu.EndSection();

		// Second list all existing slots controls
		// Menu.AddSeparator(TEXT("existing slots"));
		// for (auto It = Item->GetDynamicSlots().CreateConstIterator(); It; ++It)
		// {
		// 	const FName& Type = It->Key;
		// 	const auto& TypedSlots = It->Value;
		// 	FString TypeString = Type.ToString();
		// 	for (auto Slot = TypedSlots.CreateConstIterator(); Slot; ++Slot)
		// 	{
		// 		FMenuEntryParams Entry;
		// 		FString Caption = TypeString + TEXT("#") + FString::FromInt(Slot.GetIndex());
		// 		Entry.LabelOverride = FText::FromString(Caption);
		// 		Entry.EntryBuilder.BindSP(this, &SSketchOutliner::MakeExistingSlotMenu, SelectedItems[0], Type, Slot.GetIndex());
		// 		Entry.bIsSubMenu = true;
		// 		Entry.bIsRecursivelySearchable = false;
		//
		// 		Menu.AddMenuEntry(Entry);
		// 	}
		// }
	}

	// Detect owning slot type
	SSketchWidget::FSlotReference Slot;
	SSketchWidget* Parent = Item->GetParent();
	if (Parent)[[likely]]
	{
		Slot = Parent->FindSlotFor(Item.Get());
	}

	// List slot controls when present
	if (!Slot.Type.IsNone())
	{
		{
			FMenuEntryParams Entry;
			Entry.IconOverride = FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.DiffersFromDefault");
			Entry.LabelOverride = LOCTEXT("Clear", "Clear");
			Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnClearWidget, Item.ToWeakPtr());
			Menu.AddMenuEntry(Entry);
		}
		{
			FMenuEntryParams Entry;
			Entry.IconOverride = FSlateIcon("CoreStyle", "GenericCommands.Delete");
			Entry.LabelOverride = LOCTEXT("Remove", "Remove");
			Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnRemoveSlot, Parent->AsWeakSubobject(Parent), Slot.Type, Slot.Index);
			Menu.AddMenuEntry(Entry);
		}
	}

	// List factories
	static const FName NAME_Factories = TEXT("factories");
	Menu.BeginSection(NAME_Factories, LOCTEXT("Factories", "Factories"));
	ListFactoriesIfAppropriate(Menu, Item, NAME_None, INDEX_NONE);
	Menu.EndSection();

	return Menu.MakeWidget();
}

void SSketchOutliner::MakeNewSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> Widget, FName Type)
{
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Empty", "Empty");
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnMakeEmptySlot, Widget, Type);
		Menu.AddMenuEntry(Entry);
	}
	ListFactoriesIfAppropriate(Menu, Widget, Type, INDEX_NONE);
}

TSharedRef<SWidget> SSketchOutliner::MakeNewSlotMenu(TWeakPtr<SSketchWidget> Widget, FName Type)
{
	FMenuBuilder Menu(true, nullptr);
	MakeNewSlotMenu(Menu, Widget, Type);
	return Menu.MakeWidget();
}

void SSketchOutliner::ListFactoriesIfAppropriate(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]]
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("ERROR", "ERROR");
		Entry.DirectActions.CanExecuteAction.BindStatic([] { return false; });
		Menu.AddMenuEntry(Entry);
		return;
	}

	const bool bNewSlot = !SlotType.IsNone() && SlotIndex == INDEX_NONE;
	if (!Widget->GetContentFactory().IsValid() || bNewSlot)
	{
		ListFactories(Menu, WeakWidget, SlotType, SlotIndex);
	}
	else
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Replace", "Replace");
		Entry.bIsSubMenu = true;
		Entry.EntryBuilder.BindSP(this, &SSketchOutliner::ListFactories, WeakWidget, SlotType, SlotIndex);
		Menu.AddMenuEntry(Entry);
	}
}

void SSketchOutliner::ListFactories(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]]
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("ERROR", "ERROR");
		Entry.DirectActions.CanExecuteAction.BindStatic([] { return false; });
		Menu.AddMenuEntry(Entry);
		return;
	}

	const auto& Core = FSketchCore::Get();
	for (const auto& [Type, Factory] : Core.Factories)
	{
		Menu.AddSubMenu(
			FText::FromName(Type),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateSP(this, &SSketchOutliner::ListFactoriesOfType, WeakWidget, Type, SlotType, SlotIndex)
		);
	}
}

void SSketchOutliner::ListFactoriesOfType(
	FMenuBuilder& Menu,
	TWeakPtr<SSketchWidget> Widget,
	FName FactoriesType,
	FName SlotType,
	int SlotIndex
)
{
	const auto& Core = FSketchCore::Get();
	for (auto Factory = Core.Factories[FactoriesType].CreateConstIterator(); Factory; ++Factory)
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = FText::FromName(Factory->Name);
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnFactorySelected, FactoriesType, Factory.GetIndex(), Widget, SlotType, SlotIndex);
		Menu.AddMenuEntry(Entry);
	}
}

void SSketchOutliner::OnClearWidget(TWeakPtr<SSketchWidget> WeakWidget)
{
	if (TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin())
	{
		Widget->UnassignFactory(true);
		OnSketchUpdated();
	}
}

void SSketchOutliner::OnMakeEmptySlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType)
{
	check(!SlotType.IsNone());

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->AddDynamicSlot(SlotType, true);
	OnSketchUpdated();
}

void SSketchOutliner::OnClearExistingSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	check(!SlotType.IsNone());
	check(SlotIndex != INDEX_NONE);

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->ReleaseDynamicSlot(SlotType, SlotIndex, true);
	OnSketchUpdated();
}

void SSketchOutliner::OnRemoveSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	check(!SlotType.IsNone());
	check(SlotIndex != INDEX_NONE);

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->RemoveDynamicSlot(SlotType, SlotIndex, true);
	OnSketchUpdated();
}

FReply SSketchOutliner::OnRemoveSlotWithReply(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	OnRemoveSlot(WeakWidget, SlotType, SlotIndex);
	return FReply::Handled();
}

void SSketchOutliner::OnFactorySelected(
	FName FactoryType,
	int FactoryIndex,
	TWeakPtr<SSketchWidget> WeakWidget,
	FName SlotType,
	int SlotIndex
)
{
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	if (!SlotType.IsNone())
	{
		if (SlotIndex != INDEX_NONE)
		{
			const auto* TypedSlots = Widget->GetDynamicSlots().Find(SlotType);
			if (!TypedSlots || !TypedSlots->IsValidIndex(SlotIndex)) [[unlikely]] return;

			Widget->AssignDynamicSlot(SlotType, SlotIndex, FactoryType, FactoryIndex, true);
			OnSketchUpdated();
		}
		else
		{
			const int NewSlotIndex = Widget->AddDynamicSlot(SlotType, true);
			Widget->AssignDynamicSlot(SlotType, NewSlotIndex, FactoryType, FactoryIndex, true);
			OnSketchUpdated();
		}
	}
	else
	{
		Widget->AssignFactory(FactoryType, FactoryIndex, true);
		OnSketchUpdated();
	}
	Tree->SetItemExpansion(WeakWidget, true);
}

void SSketchOutliner::OnSketchUpdated()
{
	Tree->RebuildList();
	if (TSharedPtr<SSketchAttributeCollection> Collection = WeakCollection.Pin())
	{
		Collection->Update();
	}
}

#undef LOCTEXT_NAMESPACE
