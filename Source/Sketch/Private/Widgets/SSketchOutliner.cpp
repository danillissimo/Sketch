#include "Widgets/SSketchOutliner.h"

#include "SketchCore.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchWidget.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "SSketchOutliner"

void SSketchOutliner::Construct(const FArguments& InArgs, SSketchWidget& Root)
{
	RootAsArray.Reserve(1);
	RootAsArray.Emplace(StaticCastWeakPtr<SSketchWidget>(Root.AsWeak()));
	ChildSlot
	[
		SAssignNew(Tree, STreeView<TWeakPtr<SSketchWidget>>)
		.TreeItemsSource(&RootAsArray)
		.OnGetChildren_Static(&SSketchOutliner::OnGetChildren)
		.OnGenerateRow_Static(&SSketchOutliner::OnGenerateRow)
		.OnSelectionChanged(this, &SSketchOutliner::OnSelectionChanged)
		.OnContextMenuOpening(this, &SSketchOutliner::OnMakeContextMenu)
	];
	Root.OnModification.AddSP(this, &SSketchOutliner::OnSketchUpdated);
	if (InArgs._AttributeCollection)
		WeakCollection = StaticCastWeakPtr<SSketchAttributeCollection>(InArgs._AttributeCollection->AsWeak());
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

TSharedRef<ITableRow> SSketchOutliner::OnGenerateRow(TWeakPtr<SSketchWidget> InItem, const TSharedRef<STableViewBase>& Owner)
{
	TSharedPtr<SSketchWidget> Item = InItem.Pin();
	if (!Item) [[unlikely]] return SNew(STableRow<TWeakPtr<SSketchWidget>>, Owner)
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.Content()
			[
				SNew(STextBlock)
				.Text(INVTEXT("ERROR"))
			]
		];

	const sketch::FFactoryHandle& Factory = Item->GetContentFactory();
	FString Name;
	Name.Reserve(64);
	const FName Tag = Item->GetTag();
	if (!Tag.IsNone())
	{
		Tag.AppendString(Name);
		Name += TEXT(": ");
	}
	Factory.Name.AppendString(Name);
	return SNew(STableRow<TWeakPtr<SSketchWidget>>, Owner)
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
			SSketchWidget::FSlot* Slot = SlotContainer->FindSlotFor(Item.Get());
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
	TArray<TWeakPtr<SSketchWidget>> SelectedItems = Tree->GetSelectedItems();
	if (SelectedItems.Num() != 1) [[unlikely]] return nullptr;
	TSharedPtr<SSketchWidget> Item = SelectedItems[0].Pin();
	if (!Item) [[unlikely]] return nullptr;

	// List slot controls if content is present and supports slots
	FMenuBuilder Menu(true, nullptr);
	if (const sketch::FFactory* Factory = Item->GetContentFactory().Resolve(); Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
	{
		// First list new slot controls
		Menu.AddSeparator(TEXT("new slots"));
		const sketch::FFactory::FDynamicSlotTypes SlotTypes = Factory->EnumerateDynamicSlotTypes(Item->GetContent());
		for (const FName& SlotType : SlotTypes)
		{
			FMenuEntryParams Entry;
			FString Label = TEXT("New ");
			SlotType.AppendString(Label);
			Entry.LabelOverride = FText::FromString(Label);
			Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnMakeEmptySlot, SelectedItems[0], SlotType);
			Entry.EntryBuilder.BindSP(this, &SSketchOutliner::MakeNewSlotMenu, SelectedItems[0], SlotType);
			Entry.bIsSubMenu = true;

			Menu.AddMenuEntry(Entry);
		}

		// Second list all existing slots controls
		Menu.AddSeparator(TEXT("existing slots"));
		for (auto It = Item->GetDynamicSlots().CreateConstIterator(); It; ++It)
		{
			const FName& Type = It->Key;
			const auto& TypedSlots = It->Value;
			FString TypeString = Type.ToString();
			for (auto Slot = TypedSlots.CreateConstIterator(); Slot; ++Slot)
			{
				FMenuEntryParams Entry;
				FString Caption = TypeString + TEXT("#") + FString::FromInt(Slot.GetIndex());
				Entry.LabelOverride = FText::FromString(Caption);
				Entry.EntryBuilder.BindSP(this, &SSketchOutliner::MakeExistingSlotMenu, SelectedItems[0], Type, Slot.GetIndex());
				Entry.bIsSubMenu = true;
				Entry.bIsRecursivelySearchable = false;

				Menu.AddMenuEntry(Entry);
			}
		}

		Menu.AddSeparator(TEXT("factories"));
	}

	ListFactories(Menu, Item, NAME_None, INDEX_NONE);

	return Menu.MakeWidget();
}

void SSketchOutliner::MakeNewSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> Widget, FName Type)
{
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Empty", "Empty");
		Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnMakeEmptySlot, Widget, Type);
		Menu.AddMenuEntry(Entry);
	}
	ListFactories(Menu, Widget, Type, INDEX_NONE);
}

void SSketchOutliner::MakeExistingSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> Widget, FName Type, int Index)
{
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Clear", "Clear");
		Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnClearExistingSlot, Widget, Type, Index);
		Menu.AddMenuEntry(Entry);
	}
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Remove", "Remove");
		Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnRemoveSlot, Widget, Type, Index);
		Menu.AddMenuEntry(Entry);
	}
	ListFactories(Menu, Widget, Type, Index);
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

	{
		FMenuEntryParams Entry;
		Entry.IconOverride = FSlateIcon("CoreStyle", "Icons.Delete");
		Entry.LabelOverride = LOCTEXT("Clear", "Clear");
		Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnClearWidget, WeakWidget);
		Menu.AddMenuEntry(Entry);
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
		Entry.DirectActions.ExecuteAction.BindStatic(&SSketchOutliner::OnFactorySelected, FactoriesType, Factory.GetIndex(), Widget, SlotType, SlotIndex);
		Menu.AddMenuEntry(Entry);
	}
}

void SSketchOutliner::OnClearWidget(TWeakPtr<SSketchWidget> WeakWidget)
{
	if (TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin())
		Widget->UnassignFactory(false);
}

void SSketchOutliner::OnMakeEmptySlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType)
{
	check(!SlotType.IsNone());

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->AddDynamicSlot(SlotType, false);
}

void SSketchOutliner::OnClearExistingSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	check(!SlotType.IsNone());
	check(SlotIndex != INDEX_NONE);

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->ReleaseDynamicSlot(SlotType, SlotIndex, false);
}

void SSketchOutliner::OnRemoveSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	check(!SlotType.IsNone());
	check(SlotIndex != INDEX_NONE);

	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;

	Widget->RemoveDynamicSlot(SlotType, SlotIndex, false);
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

			Widget->AssignDynamicSlot(SlotType, SlotIndex, FactoryType, FactoryIndex, false);
		}
		else
		{
			const int NewSlotIndex = Widget->AddDynamicSlot(SlotType, true);
			Widget->AssignDynamicSlot(SlotType, NewSlotIndex, FactoryType, FactoryIndex, false);
		}
	}
	else
	{
		Widget->AssignFactory(FactoryType, FactoryIndex, false);
	}
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
