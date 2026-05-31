// Copyright 2026 danillissimo

#include "Widgets/SSketchOutliner.h"

#include "Sketch.h"
#include "SketchStringLiteral.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchWidget.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Layout/WidgetPath.h"
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
			.OnKeyDownHandler(this, &SSketchOutliner::OnRowKeyDown)
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

		// Make "add/assign slot" button
		FOnGetContent OnListSlotOptions;
		FText Tooltip;
		{
			const sketch::FFactory* Factory = FactoryHandle.Resolve();
			if (!Factory)
			{
				OnListSlotOptions.BindSP(Owner, &SSketchOutliner::ListFactories, InItem);
				Tooltip = LOCTEXT("AssignSlot", "Assign slot");
			}
			else
			{
				if (Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
				{
					sketch::FFactory::FDynamicSlotTypes SlotTypes = Factory->EnumerateDynamicSlotTypes(Item->GetContent());
					if (!SlotTypes.IsEmpty())
					{
						OnListSlotOptions.BindSP(Owner, &SSketchOutliner::MakeNewSlotMenu, InItem, SlotTypes[0]);
						Tooltip = LOCTEXT("AddSlot", "Add slot");
					}
				}
			}
		}
		TSharedRef<SMenuAnchor> MenuAnchor =
			SNew(SMenuAnchor)
			.IsEnabled(OnListSlotOptions.IsBound())
			.OnGetMenuContent(MoveTemp(OnListSlotOptions))
			.OnMenuOpenChanged(this, &SSketchOutlinerRow::OnMenuOpenChanged);
		constexpr FLinearColor ThreeThirdsWhite{ 1, 1, 1, 0.75 };
		TSharedRef<SButton> AddSlotButton =
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.ToolTipText(MoveTemp(Tooltip))
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
			OwningSlot = Parent->FindDynamicSlotFor(Item.Get());
			OnRemoveSlot.BindSP(Owner, &SSketchOutliner::OnRemoveDynamicSlotWithReply, Parent->AsWeakSubobject(Parent), OwningSlot.Type, OwningSlot.Index);
		}

		// Make full widget
		ChildSlot
		[
			SAssignNew(Content, SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MoveTemp(Name)))
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ToolTipText(LOCTEXT("RemoveSlot", "Remove slot"))
					.OnClicked(MoveTemp(OnRemoveSlot))
					.IsEnabled(!!OwningSlot.Data)
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

	void MayBeHideControls()
	{
		if (bWantHide && !bMenuOpen)
		{
			Content->GetSlot(1).SetFillWidth(0);
		}
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

		bWantHide = false;
		Content->GetSlot(1).SetAutoWidth();
	}

	void OnMenuOpenChanged(bool bOpen)
	{
		bMenuOpen = bOpen;
		MayBeHideControls();
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		SCompoundWidget::OnMouseLeave(MouseEvent);

		bWantHide = true;
		MayBeHideControls();
	}

private:
	TSharedPtr<SHorizontalBox> Content;
	bool bMenuOpen = false;
	bool bWantHide = false;
};

TSharedRef<ITableRow> SSketchOutliner::OnGenerateRow(TWeakPtr<SSketchWidget> InItem, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TWeakPtr<SSketchWidget>>, Owner)
		.OnDragDetected_Static(&SSketchOutliner::OnItemDragDetected, InItem)
		.OnDragLeave_Static(&SSketchOutliner::OnDragLeaveItem)
		.OnCanAcceptDrop_Static(&SSketchOutliner::OnCanAcceptDrop)
		.OnAcceptDrop(this, &SSketchOutliner::OnAcceptDrop)
		[
			SNew(SSketchOutlinerRow, this, InItem)
		];
}

FReply SSketchOutliner::OnRowKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Sanitize
	TArray<TWeakPtr<SSketchWidget>> SelectedItems = Tree->GetSelectedItems();
	if (SelectedItems.Num() != 1) [[unlikely]] return FReply::Unhandled();
	TSharedPtr<SSketchWidget> Item = SelectedItems[0].Pin();
	if (!Item) [[unlikely]] return FReply::Unhandled();

	// Handle delete
	if (Event.GetKey() == EKeys::Delete)
	{
		SSketchWidget* Parent = Item->GetParent();
		if (Item->IsUniqueSlotContainer())
		{
			sketch::FFactory::FUniqueSlots UniqueSlots = Parent->CollectUniqueSlots();
			SSketchWidget** Record = UniqueSlots.FindByPredicate([&](const SSketchWidget* Widget) { return Widget == Item.Get(); });
			const int Index = Record - UniqueSlots.GetData();
			OnResetUniqueSlot(Parent, Index);
		}
		else if (!Item->IsRoot())
		{
			SSketchWidget::FSlotReference Slot = Parent->FindDynamicSlotFor(Item.Get());
			OnRemoveDynamicSlot(Parent->AsWeakSubobject(Parent), Slot.Type, Slot.Index);
		}
		else
		{
			OnResetRoot();
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
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
			SSketchWidget::FSlot* Slot = SlotContainer->FindDynamicSlotFor(Item.Get()).Data;
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

	// List slot creation commands
	FMenuBuilder Menu(true, nullptr);
	const sketch::FFactory* Factory = Item->GetContentFactory().Resolve();
	if (Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
	{
		// List new slot controls
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

		// List all existing slots controls
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

	// List replacement options
	if (Factory && (Factory->EnumerateDynamicSlotTypes.IsSet() || Factory->EnumerateUniqueSlots.IsSet()))
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("ReplaceWith", "Replace with");
		Entry.bIsSubMenu = true;
		Entry.EntryBuilder.BindSP(this, &SSketchOutliner::ListReplacementOptions, SelectedItems[0]);
		Menu.AddMenuEntry(Entry);
	}

	// List wrap options
	if (!Item->IsRoot())
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("WrapWith", "Wrap with");
		Entry.bIsSubMenu = true;
		Entry.EntryBuilder.BindSP(this, &SSketchOutliner::ListWrappers, SelectedItems[0]);
		Menu.AddMenuEntry(Entry);
	}

	// List destructive slot commands
	if (!Item->IsRoot())[[likely]]
	{
		// Consider dynamic slot
		SSketchWidget* Parent = Item->GetParent();
		SSketchWidget::FSlotReference Slot = Parent->FindDynamicSlotFor(Item.Get());
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
				Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnRemoveDynamicSlot, Parent->AsWeakSubobject(Parent), Slot.Type, Slot.Index);
				Menu.AddMenuEntry(Entry);
			}
		}
		else
		{
			// Detect owning unique slot
			const sketch::FFactory::FUniqueSlots UniqueSlots = Parent->CollectUniqueSlots();
			SSketchWidget* const* Record = UniqueSlots.FindByPredicate([&](const SSketchWidget* Widget) { return Widget == Item.Get(); });
			if (Record)[[likely]]
			{
				const int Index = Record - UniqueSlots.GetData();
				FMenuEntryParams Entry;
				Entry.IconOverride = FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.DiffersFromDefault");
				Entry.LabelOverride = LOCTEXT("Clear", "Clear");
				Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnResetUniqueSlot, Parent->AsWeakSubobject(Parent), Index);
				Menu.AddMenuEntry(Entry);
			}
		}
	}
	else
	{
		FMenuEntryParams Entry;
		Entry.IconOverride = FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.DiffersFromDefault");
		Entry.LabelOverride = LOCTEXT("Clear", "Clear");
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnResetRoot);
		Menu.AddMenuEntry(Entry);
	}

	// List factories
	ListFactoriesIfAppropriate(Menu, Item, NAME_None, INDEX_NONE);

	return Menu.MakeWidget();
}

void SSketchOutliner::MakeNewSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName Type)
{
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = LOCTEXT("Empty", "Empty");
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnMakeEmptySlot, WeakWidget, Type);
		Menu.AddMenuEntry(Entry);
	}
	ListFactoriesIfAppropriate(Menu, WeakWidget, Type, INDEX_NONE);
}

TSharedRef<SWidget> SSketchOutliner::MakeNewSlotMenu(TWeakPtr<SSketchWidget> WeakWidget, FName Type)
{
	FMenuBuilder Menu(true, nullptr);
	MakeNewSlotMenu(Menu, WeakWidget, Type);
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
		Entry.LabelOverride = LOCTEXT("New", "New");
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

TSharedRef<SWidget> SSketchOutliner::ListFactories(TWeakPtr<SSketchWidget> WeakWidget)
{
	FMenuBuilder Menu(true, nullptr);
	ListFactories(Menu, WeakWidget, NAME_None, INDEX_NONE);
	return Menu.MakeWidget();
}

void SSketchOutliner::ListFactoriesOfType(
	FMenuBuilder& Menu,
	TWeakPtr<SSketchWidget> WeakWidget,
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
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnFactorySelected, FactoriesType, Factory.GetIndex(), WeakWidget, SlotType, SlotIndex);
		Menu.AddMenuEntry(Entry);
	}
}

void SSketchOutliner::ListReplacementOptions(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget)
{
	// Sanitize
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	sketch::FFactory* CurrentWidgetFactory = Widget ? Widget->GetContentFactory().Resolve() : nullptr;
	if (!Widget) [[unlikely]] return;

	// Calc num slots of each type
	int NumDynamicSlots = 0, NumOccupiedDynamicSlots = 0, NumUniqueSlots = 0, NumOccupiedUniqueSlots = 0;
	for (const auto& [Type, Slots] : Widget->GetDynamicSlots())
	{
		for (const SSketchWidget::FSlot& Slot : Slots)
		{
			++NumDynamicSlots;
			if (Slot.Widget->GetContentFactory().IsValid())
			{
				++NumOccupiedDynamicSlots;
			}
		}
	}
	if (CurrentWidgetFactory->EnumerateUniqueSlots.IsSet())
	{
		sketch::FFactory::FUniqueSlots UniqueSlots = CurrentWidgetFactory->EnumerateUniqueSlots(Widget->GetContent());
		NumUniqueSlots = UniqueSlots.Num();
		for (SSketchWidget* UniqueSlot : UniqueSlots)
		{
			if (UniqueSlot->GetContentFactory().IsValid())
			{
				++NumOccupiedDynamicSlots;
			}
		}
	}

	// List children first
	if (NumOccupiedDynamicSlots > 0 || NumOccupiedUniqueSlots > 0)
	{
		static const FName NAME_Child = SL"Child";
		Menu.BeginSection(NAME_Child, LOCTEXT("Child", "Child"));
		ListChildrenToBeReplacedBy(Menu, Widget.Get(), *CurrentWidgetFactory);
		Menu.EndSection();
	}

	// List any suitable containers
	// New container must be able to accommodate all existing slots of replaced widget
	// Slots are allowed to change their type during transition
	static const FName NAME_Container = SL"Container";
	Menu.BeginSection(NAME_Container, LOCTEXT("Container", "Container"));
	const int TotalSlotsNeeded = NumDynamicSlots + NumUniqueSlots;
	FSketchCore& Core = FSketchCore::Get();
	for (const auto& [Category, Factories] : Core.Factories)
	{
		for (auto Factory = Factories.CreateConstIterator(); Factory; ++Factory)
		{
			if (!Factory->ConstructDynamicSlot.IsSet())
			{
				if (!Factory->EnumerateUniqueSlots.IsSet()) continue;

				sketch::FAttributeCollection TempAttributes(InPlace);
				Core.RedirectNewAttributesInto(TempAttributes);
				TSharedRef<SWidget> WidgetSample = Factory->ConstructWidget(nullptr);
				Core.StopRedirectingNewAttributes();
				sketch::FFactory::FUniqueSlots UniqueSlots = Factory->EnumerateUniqueSlots(*WidgetSample);
				if (UniqueSlots.Num() < TotalSlotsNeeded) continue;
			}

			FMenuEntryParams Entry;
			Entry.LabelOverride = FText::FromName(Factory->Name);
			Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::ReplaceContainer, WeakWidget, Category, Factory.GetIndex());
			Menu.AddMenuEntry(Entry);
		}
	}
	Menu.EndSection();
}

void SSketchOutliner::ListChildrenToBeReplacedBy(FMenuBuilder& Menu, SSketchWidget* Widget, sketch::FFactory& Factory)
{
	// List unique slots
	if (Factory.EnumerateUniqueSlots.IsSet())
	{
		sketch::FFactory::FUniqueSlots UniqueSlots = Factory.EnumerateUniqueSlots(Widget->GetContent());
		for (SSketchWidget* UniqueSlot : UniqueSlots)
		{
			if (UniqueSlot->GetContentFactory().IsValid())
			{
				FMenuEntryParams Entry;
				Entry.LabelOverride = FText::FromName(UniqueSlot->GetTag());
				Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnReplaceByUniqueSlotContent, Widget->AsWeakSubobject(Widget), UniqueSlot->GetTag());
				Menu.AddMenuEntry(Entry);
			}
		}
	};

	// List dynamic slots
	for (const auto& [Type, Slots] : Widget->GetDynamicSlots())
	{
		for (auto Slot = Slots.CreateConstIterator(); Slot; ++Slot)
		{
			if (sketch::FFactory* SlotFactory = Slot->Widget->GetContentFactory().Resolve())
			{
				FMenuEntryParams Entry;
				FString Label = Type.ToString();
				Label.Append(TEXT(" #"));
				Label.AppendInt(Slot.GetIndex());
				Label.Append(TEXT(" - "));
				SlotFactory->Name.AppendString(Label);
				Entry.LabelOverride = FText::FromString(MoveTemp(Label));
				Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::OnReplaceByDynamicSlotContent, Widget->AsWeakSubobject(Widget), Type, Slot.GetIndex());
				Menu.AddMenuEntry(Entry);
			}
		}
	}
}

void SSketchOutliner::ListWrappers(FMenuBuilder& MenuBuilder, TWeakPtr<SSketchWidget> WeakWidget)
{
	const FSketchCore& Core = FSketchCore::Get();
	for (const auto& [Category, Factories] : Core.Factories)
	{
		for (auto Factory = Factories.CreateConstIterator(); Factory; ++Factory)
		{
			if (Factory->ConstructDynamicSlot.IsSet() || Factory->EnumerateUniqueSlots.IsSet())
			{
				FMenuEntryParams Entry;
				Entry.LabelOverride = FText::FromName(Factory->Name);
				Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchOutliner::Wrap, WeakWidget, Category, Factory.GetIndex());
				MenuBuilder.AddMenuEntry(Entry);
			}
		}
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

void SSketchOutliner::OnRemoveDynamicSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	if (!Widget) [[unlikely]] return;
	OnRemoveDynamicSlot(Widget.Get(), SlotType, SlotIndex);
}

void SSketchOutliner::OnRemoveDynamicSlot(SSketchWidget* Widget, FName SlotType, int SlotIndex)
{
	check(!SlotType.IsNone());
	check(SlotIndex != INDEX_NONE);
	Widget->RemoveDynamicSlot(SlotType, SlotIndex, true);
	OnSketchUpdated();
}

FReply SSketchOutliner::OnRemoveDynamicSlotWithReply(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex)
{
	OnRemoveDynamicSlot(WeakWidget, SlotType, SlotIndex);
	return FReply::Handled();
}

void SSketchOutliner::OnResetUniqueSlot(TWeakPtr<SSketchWidget> WeakParent, int Index)
{
	if (TSharedPtr<SSketchWidget> Parent = WeakParent.Pin()) [[likely]]
	{
		OnResetUniqueSlot(Parent.Get(), Index);
	}
}

void SSketchOutliner::OnResetUniqueSlot(SSketchWidget* Parent, int Index)
{
	const sketch::FFactory::FUniqueSlots UniqueSlots = Parent->CollectUniqueSlots();
	if (UniqueSlots.IsValidIndex(Index)) [[likely]]
	{
		UniqueSlots[Index]->UnassignFactory(true);
		OnSketchUpdated();
	}
}

void SSketchOutliner::OnResetRoot()
{
	if (TSharedPtr<SSketchWidget> Root = RootAsArray[0].Pin()) [[likely]]
	{
		Root->UnassignFactory(true);
		OnSketchUpdated();
	}
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

void SSketchOutliner::OnReplaceByUniqueSlotContent(TWeakPtr<SSketchWidget> WeakWidget, FName SlotName)
{
	// Sanitize
	TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin();
	sketch::FFactory* Factory = Widget ? Widget->GetContentFactory().Resolve() : nullptr;
	if (!Widget || !Factory) [[unlikely]] return;

	// Locate child and do the job
	sketch::FFactory::FUniqueSlots UniqueSlots = Factory->EnumerateUniqueSlots(Widget->GetContent());
	for (SSketchWidget* UniqueSlot : UniqueSlots)
	{
		const FName UniqueSlotName = UniqueSlot->GetTag();
		if (UniqueSlotName == SlotName)
		{
			ReplaceWidget(Widget.Get(), UniqueSlot);
			OnSketchUpdated();
			break;
		}
	}
}

void SSketchOutliner::OnReplaceByDynamicSlotContent(TWeakPtr<SSketchWidget> WeakWidget, FName Type, int Index)
{
	if (TSharedPtr<SSketchWidget> Widget = WeakWidget.Pin())
	{
		const SSketchWidget::FSlot& ChildWidget = Widget->GetDynamicSlots()[Type][Index];
		ReplaceWidget(Widget.Get(), ChildWidget.Widget.Get());
		OnSketchUpdated();
	}
}

void SSketchOutliner::ReplaceWidget(SSketchWidget* Widget, SSketchWidget* Replacement)
{
	// Sanitize
	SSketchWidget* Parent = Widget ? Widget->GetParent() : nullptr;
	sketch::FFactory* ParentFactory = Parent ? Parent->GetContentFactory().Resolve() : nullptr;
	if (!Parent || !ParentFactory) [[unlikely]] return;

	// If we are the root - no looking for a parent slot
	if (Widget->IsRoot())
	{
		Widget->ReplaceBy(Replacement, true);
		return;
	}

	// Try to locate owning dynamic slot
	const SSketchWidget::FSlotReference DynamicSlot = Parent->FindDynamicSlotFor(Widget);
	if (DynamicSlot.Data)
	{
		Parent->ReassignDynamicSlot(DynamicSlot.Type, DynamicSlot.Index, Replacement, true);
		return;
	}

	// Ensure this is a unique parent slot
	if (ParentFactory->EnumerateUniqueSlots.IsSet())
	{
		sketch::FFactory::FUniqueSlots UniqueSlots = ParentFactory->EnumerateUniqueSlots(Parent->GetContent());
		for (SSketchWidget* UniqueSlot : UniqueSlots)
		{
			if (UniqueSlot == Widget)
			{
				Widget->ReplaceBy(Replacement, true);
				return;
			}
		}
	}
}

void SSketchOutliner::Wrap(TWeakPtr<SSketchWidget> WeakWidget, FName FactoryCategory, int FactoryIndex)
{
	// Sanitize
	TSharedPtr<SSketchWidget> OperatedWidgetContainer = WeakWidget.Pin();
	if (!OperatedWidgetContainer) [[unlikely]] return;

	// Move replaced widget into a buffer
	TSharedPtr<SSketchWidget> Buffer = SNew(SSketchWidget).IsAttachTarget(false);
	Buffer->ReplaceBy(OperatedWidgetContainer.Get(), true);

	// Put wrapper in place of the original widget
	OperatedWidgetContainer->AssignFactory(FactoryCategory, FactoryIndex, true);

	// Locate a slot within wrapper
	// Prefer dynamic slots if both dynamic and unique are present
	const FSketchCore& Core = FSketchCore::Get();
	const sketch::FFactory& Factory = Core.Factories[FactoryCategory][FactoryIndex];
	if (Factory.EnumerateDynamicSlotTypes.IsSet())
	{
		// Create dynamic slot
		sketch::FFactory::FDynamicSlotTypes DynamicSlotTypes = Factory.EnumerateDynamicSlotTypes(OperatedWidgetContainer->GetContent());
		OperatedWidgetContainer->AddDynamicSlot(DynamicSlotTypes[0], true);
		OperatedWidgetContainer->ReassignDynamicSlot(DynamicSlotTypes[0], 0, Buffer.Get(), true);
		OnSketchUpdated();
	}
	else
	{
		// Use unique slot
		check(Factory.EnumerateUniqueSlots.IsSet());
		sketch::FFactory::FUniqueSlots UniqueSlots = Factory.EnumerateUniqueSlots(OperatedWidgetContainer->GetContent());
		UniqueSlots[0]->ReplaceBy(Buffer.Get(), true);
		OnSketchUpdated();
	}

	// Reflect changes immediately
	Tree->SetItemExpansion(WeakWidget, true);
}

void SSketchOutliner::ReplaceContainer(TWeakPtr<SSketchWidget> WeakWidget, FName FactoryCategory, int FactoryIndex)
{
	// Sanitize
	TSharedPtr<SSketchWidget> OperatedWidgetContainer = WeakWidget.Pin();
	if (!OperatedWidgetContainer) [[unlikely]] return;

	// Move replaced container into a buffer
	TSharedPtr<SSketchWidget> Buffer = SNew(SSketchWidget).IsAttachTarget(false);
	Buffer->ReplaceBy(OperatedWidgetContainer.Get(), true);

	// Put new container in place of the original widget
	OperatedWidgetContainer->AssignFactory(FactoryCategory, FactoryIndex, true);

	// Prepare dynamic slot type
	FSketchCore& Core = FSketchCore::Get();
	FName DynamicSlotType;
	{
		const sketch::FFactory& Factory = Core.Factories[FactoryCategory][FactoryIndex];
		if (Factory.EnumerateDynamicSlotTypes.IsSet())
		{
			const sketch::FFactory::FDynamicSlotTypes UniqueSlotTypes = Factory.EnumerateDynamicSlotTypes(OperatedWidgetContainer->GetContent());
			DynamicSlotType = UniqueSlotTypes[0];
		}
	}

	// Move all unique slots
	int NumUniqueSlotsMoved = 0;
	sketch::FFactory::FUniqueSlots UniqueSlots = OperatedWidgetContainer->CollectUniqueSlots();
	{
		// Move all unique slots to available unique slots of the new container
		sketch::FFactory::FUniqueSlots UniqueSlotsToMove = Buffer->CollectUniqueSlots();
		for (; NumUniqueSlotsMoved < FMath::Min(UniqueSlots.Num(), UniqueSlotsToMove.Num()); ++NumUniqueSlotsMoved)
		{
			UniqueSlots[NumUniqueSlotsMoved]->ReplaceBy(UniqueSlotsToMove[NumUniqueSlotsMoved], true);
		}

		// Move the rest of unique slots to dynamic slots
		if (NumUniqueSlotsMoved < UniqueSlotsToMove.Num())
		{
			for (; NumUniqueSlotsMoved < UniqueSlotsToMove.Num(); ++NumUniqueSlotsMoved)
			{
				const int NewSlotIndex = OperatedWidgetContainer->AddDynamicSlot(DynamicSlotType, true);
				OperatedWidgetContainer->ReassignDynamicSlot(DynamicSlotType, NewSlotIndex, UniqueSlotsToMove[NumUniqueSlotsMoved], true);
			}
		}
	}

	// Move all dynamic slots
	// If new container supports dynamic slots - use them
	// If it doesn't - then it means unique slots can accomodate all dynamic slots
	if (!DynamicSlotType.IsNone())
	{
		for (const auto& [Type, Slots] : Buffer->GetDynamicSlots())
		{
			for (const SSketchWidget::FSlot& Slot : Slots)
			{
				const int NewSlotIndex = OperatedWidgetContainer->AddDynamicSlot(DynamicSlotType, true);
				OperatedWidgetContainer->ReassignDynamicSlot(DynamicSlotType, NewSlotIndex, Slot.Widget.Get(), true);
			}
		}
	}
	else
	{
		for (const auto& [Type, Slots] : Buffer->GetDynamicSlots())
		{
			for (const SSketchWidget::FSlot& Slot : Slots)
			{
				UniqueSlots[NumUniqueSlotsMoved]->ReplaceBy(Slot.Widget.Get(), true);
				++NumUniqueSlotsMoved;
			}
		}
	}

	// Refresh representation
	OnSketchUpdated();
}

class FSketchOutlinerDragDropOperation : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSketchOutlinerDragDropOperation, FDragDropOperation)

	static TSharedRef<FSketchOutlinerDragDropOperation> New(const TWeakPtr<SSketchWidget>& InWeakWidget)
	{
		TSharedRef<FSketchOutlinerDragDropOperation> Operation = MakeShared<FSketchOutlinerDragDropOperation>();
		Operation->WeakWidget = InWeakWidget;
		Operation->Construct();
		return Operation;
	}

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2)
				[
					SAssignNew(Icon, SImage)
					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Graph.ConnectorFeedback.Error").GetIcon())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2)
				[
					SAssignNew(Text, STextBlock)
				]
			];
	}

	void SetResponse(bool bAcceptable, FText&& Reason)
	{
		Icon->SetVisibility(EVisibility::Visible);
		Icon->SetImage(bAcceptable ? FSlateIcon(FAppStyle::GetAppStyleSetName(), "Graph.ConnectorFeedback.OK").GetIcon() : FSlateIcon(FAppStyle::GetAppStyleSetName(), "Graph.ConnectorFeedback.Error").GetIcon());
		Text->SetText(MoveTemp(Reason));
	}

	void SetNoResponse()
	{
		Icon->SetVisibility(EVisibility::Collapsed);
		Text->SetText(INVTEXT("..."));
	}

	TWeakPtr<SSketchWidget> WeakWidget;

private:
	mutable TSharedPtr<SImage> Icon;
	mutable TSharedPtr<STextBlock> Text;
};

FReply SSketchOutliner::OnItemDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, TWeakPtr<SSketchWidget> WeakWidget)
{
	return FReply::Handled().BeginDragDrop(FSketchOutlinerDragDropOperation::New(WeakWidget));
}

TOptional<EItemDropZone> SSketchOutliner::OnCanAcceptDrop(const FDragDropEvent& Event, EItemDropZone DropZone, TWeakPtr<SSketchWidget> WeakWidget)
{
	// Sanitize
	TSharedPtr<FSketchOutlinerDragDropOperation> Operation = Event.GetOperationAs<FSketchOutlinerDragDropOperation>();
	TSharedPtr<SSketchWidget> DraggedWidget = Operation ? Operation->WeakWidget.Pin() : nullptr;
	TSharedPtr<SSketchWidget> HoveredWidget = WeakWidget.Pin();
	if (!Operation || !DraggedWidget || !HoveredWidget) [[unlikely]] return {};

	// Ensure widget is not dragged over itself
	if (DraggedWidget == HoveredWidget) [[unlikely]]
	{
		Operation->SetResponse(false, {});
		return {};
	}

	// Ensure dragged widget is not already within hovered widget
	if (DraggedWidget->GetParent() == HoveredWidget.Get()) [[unlikely]]
	{
		Operation->SetResponse(false, LOCTEXT("WidgetIsAlreadyWithinThisContainer", "Widget is already withing this container"));
		return {};
	}

	// Ensure hovered widget is not a child of dragged widget
	for (SSketchWidget* Parent = HoveredWidget->GetParent(); ; Parent = Parent->GetParent())
	{
		if (Parent == DraggedWidget.Get()) [[unlikely]]
		{
			Operation->SetResponse(false, LOCTEXT("WidgetCanNotBeMovedIntoItsChild", "Widget can't be moved into its child"));
			return {};
		}
		if (Parent->IsRoot()) break;
	}

	// Direct drop is only allowed when target supports dynamic slots, or has no factory assigned
	if (DropZone == EItemDropZone::OntoItem)
	{
		const sketch::FFactory* Factory = HoveredWidget->GetContentFactory().Resolve();
		if (Factory && !Factory->ConstructDynamicSlot.IsSet())
		{
			Operation->SetResponse(false, LOCTEXT("TargetDoesNotSupportDynamicSlots", "Target doesn't support dynamic slots"));
			return {};
		}
	}

	// In-between drop is only allowed when a new dynamic slot can be added
	// And only if drop actually reorders anything
	EItemDropZone DesiredDropZone = DropZone;
	if (DropZone != EItemDropZone::OntoItem)
	{
		SSketchWidget* NewParent = HoveredWidget->GetParent();
		sketch::FFactory* Factory = NewParent->GetContentFactory().Resolve();
		if (!Factory->ConstructDynamicSlot.IsSet())
		{
			DesiredDropZone = EItemDropZone::OntoItem;
		}
		else
		{
			SSketchWidget* CurrentParent = DraggedWidget->GetParent();
			if (CurrentParent == NewParent)
			{
				const SSketchWidget::FSlotReference HoveredSlot = CurrentParent->FindDynamicSlotFor(HoveredWidget.Get());
				const SSketchWidget::FSlotReference DraggedSlot = CurrentParent->FindDynamicSlotFor(DraggedWidget.Get());
				const int NewExpectedPosition = HoveredSlot.Index + (DropZone == EItemDropZone::AboveItem ? 0 : 1);
				if (NewExpectedPosition == DraggedSlot.Index)[[unlikely]]
				{
					Operation->SetResponse(false, LOCTEXT("WidgetIsAlreadyAtThisPosition", "Widget is already at this position"));
					return {};
				}
			}
		}
	}

	// The rest is safe to do
	Operation->SetResponse(true, {});
	return DesiredDropZone;
}

void SSketchOutliner::OnDragLeaveItem(const FDragDropEvent& Event)
{
	if (TSharedPtr<FSketchOutlinerDragDropOperation> Operation = Event.GetOperationAs<FSketchOutlinerDragDropOperation>())[[likely]]
	{
		Operation->SetNoResponse();
	}
}

FReply SSketchOutliner::OnAcceptDrop(const FDragDropEvent& Event, EItemDropZone DropZone, TWeakPtr<SSketchWidget> WeakWidget)
{
	// Sanitize
	TSharedPtr<FSketchOutlinerDragDropOperation> Operation = Event.GetOperationAs<FSketchOutlinerDragDropOperation>();
	TSharedPtr<SSketchWidget> DraggedWidget = Operation ? Operation->WeakWidget.Pin() : nullptr;
	TSharedPtr<SSketchWidget> HoveredWidget = WeakWidget.Pin();
	if (!Operation || !DraggedWidget || !HoveredWidget) [[unlikely]] return FReply::Unhandled();

	// Remove dragged widget from wherever it was
	TSharedPtr<SSketchWidget> Buffer;
	{
		SSketchWidget* Parent = DraggedWidget->GetParent();
		if (SSketchWidget::FSlotReference Slot = Parent->FindDynamicSlotFor(DraggedWidget.Get()); Slot.Data)
		{
			Buffer = Slot.Data->Widget;
			Parent->RemoveDynamicSlot(Slot.Type, Slot.Index, true);
		}
		else
		{
			Buffer = SNew(SSketchWidget).IsAttachTarget(false);
			Buffer->ReplaceBy(DraggedWidget.Get(), true);
			DraggedWidget->UnassignFactory(true);
		}
	}

	// Handle drop
	if (DropZone == EItemDropZone::OntoItem)
	{
		// Handle direct drop
		const sketch::FFactory* Factory = HoveredWidget->GetContentFactory().Resolve();
		if (!Factory)
		{
			// Assign empty slot
			HoveredWidget->ReplaceBy(Buffer.Get(), true);
		}
		else
		{
			// Move to a new dynamic slot
			sketch::FFactory::FDynamicSlotTypes DynamicSlotTypes = Factory->EnumerateDynamicSlotTypes(HoveredWidget->GetContent());
			const int SlotIndex = HoveredWidget->AddDynamicSlot(DynamicSlotTypes[0], true);
			HoveredWidget->ReassignDynamicSlot(DynamicSlotTypes[0], SlotIndex, Buffer.Get(), true);
		}
	}
	else
	{
		// Determine index for the new slot
		SSketchWidget* Parent = HoveredWidget->GetParent();
		const sketch::FFactory* Factory = Parent->GetContentFactory().Resolve();
		const SSketchWidget::FSlotReference Slot = Parent->FindDynamicSlotFor(HoveredWidget.Get());
		check(Slot.Data);
		const int NewSlotPosition = Slot.Index + (DropZone == EItemDropZone::AboveItem ? 0 : 1);

		// Determine new slot type
		sketch::FFactory::FDynamicSlotTypes DynamicSlotTypes = Factory->EnumerateDynamicSlotTypes(HoveredWidget->GetContent());

		// Process in-between drop
		Parent->InsertDynamicSlot(DynamicSlotTypes[0], NewSlotPosition, true);
		Parent->ReassignDynamicSlot(DynamicSlotTypes[0], NewSlotPosition, Buffer.Get(), true);
	}

	// Notify whoever
	OnSketchUpdated();
	return FReply::Handled();
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
