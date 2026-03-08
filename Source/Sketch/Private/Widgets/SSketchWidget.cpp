#include "Widgets/SSketchWidget.h"

#include "SketchModule.h"
#include "Sketch.h"

SLATE_IMPLEMENT_WIDGET(SSketchWidget)

void SSketchWidget::PrivateRegisterAttributes(FSlateAttributeDescriptor::FInitializer&)
{}

sketch::FFactory::FUniqueSlots SSketchWidget::CollectUniqueSlots() const
{
	if (!ContentFactory.IsValid())
		return {};

	const sketch::FFactory* Factory = ContentFactory.Resolve();
	if (!Factory) [[unlikely]]
		return {};

	return Factory->EnumerateUniqueSlots.IsSet()
			? Factory->EnumerateUniqueSlots(GetContent())
			: sketch::FFactory::FUniqueSlots{};
}

SSketchWidget::FSlot* SSketchWidget::FindSlotFor(SSketchWidget* Widget)
{
	for (auto& [Type,TypedSlots] : Slots)
	{
		for (FSlot& Slot : TypedSlots)
		{
			if (Slot.Widget.Get() == Widget)
			{
				return &Slot;
			}
		}
	}
	return nullptr;
}

SSketchWidget* SSketchWidget::GetParent() const
{
	if (bRoot) [[unlikely]] return const_cast<SSketchWidget*>(this);

	const FSlateWidgetClassData& SketchWidgetClass = GetWidgetClass();
	for (TSharedPtr<SWidget> Parent = GetParentWidget(); Parent; Parent = Parent->GetParentWidget())
	{
		const FSlateWidgetClassData& Class = Parent->GetWidgetClass();
		if (Class.GetWidgetType() == SketchWidgetClass.GetWidgetType())
		{
			auto* ParentSketchWidget = static_cast<SSketchWidget*>(Parent.Get());
			return ParentSketchWidget;
		}
	}
	return nullptr;
}

SSketchWidget* SSketchWidget::FindRoot() const
{
	if (bRoot) [[unlikely]]
	{
		return const_cast<SSketchWidget*>(this);
	}

	const FSlateWidgetClassData& SketchWidgetClass = GetWidgetClass();
	for (TSharedPtr<SWidget> Parent = GetParentWidget(); Parent; Parent = Parent->GetParentWidget())
	{
		const FSlateWidgetClassData& Class = Parent->GetWidgetClass();
		if (Class.GetWidgetType() == SketchWidgetClass.GetWidgetType())
		{
			auto* ParentSketchWidget = static_cast<SSketchWidget*>(Parent.Get());
			if (ParentSketchWidget->bRoot)
			{
				return ParentSketchWidget;
			}
		}
	}
	return nullptr;
}

void SSketchWidget::AssignFactory(const FName& FactoryType, int FactoryIndex, bool bSuppressModificationEvent)
{
	if (ContentFactory.IsValid())
		UnassignFactory();

	Overlay->ClearChildren();

	auto& Host = FSketchModule::Get();
	const sketch::FFactory& Factory = Host.Factories[FactoryType][FactoryIndex];
	Host.RedirectNewAttributesInto(*this, Attributes);
	TSharedRef<SWidget> Widget = Factory.ConstructWidget();
	Host.StopRedirectingNewAttributes();

	ContentFactory.Type = FactoryType;
	ContentFactory.Name = Factory.Name;

	Overlay->AddSlot()[MoveTemp(Widget)];
	Overlay->AddSlot()[Border.ToSharedRef()];

	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::UnassignFactory()
{
	for (auto& [Type, TypedSlots] : Slots)
		for (int Index = TypedSlots.Num() - 1; Index >= 0; Index--)
			RemoveDynamicSlot(Type, Index, true);

	Attributes.Empty();

	ContentFactory.Type = NAME_None;
	ContentFactory.Name = NAME_None;
}

void SSketchWidget::UnassignFactory(bool bSuppressModificationEvent)
{
	Overlay->ClearChildren();

	UnassignFactory();

	Overlay->AddSlot();
	Overlay->AddSlot()[Border.ToSharedRef()];

	BroadcastModification(bSuppressModificationEvent);
}

int SSketchWidget::AddDynamicSlot(const FName& Type, bool bSuppressModificationEvent)
{
	auto& TypedSlots = Slots.FindOrAdd(Type);
	FSlot& Slot = TypedSlots.Emplace_GetRef();
	Slot.Attributes = MakeShared<TArray<sketch::FAttribute>>();
	Slot.Widget = SNew(SSketchWidget);

	auto& Host = FSketchModule::Get();
	sketch::FFactory* Factory = ContentFactory.Resolve();
	Host.RedirectNewAttributesInto(Slot.Attributes);
	Slot.Slot = &Factory->ConstructDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Type);
	Host.StopRedirectingNewAttributes();
	Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());

	BroadcastModification(bSuppressModificationEvent);
	return TypedSlots.Num() - 1;
}

void SSketchWidget::AssignDynamicSlot(
	const FName& Type,
	int Index,
	const FName& FactoryType,
	int FactoryIndex,
	bool bSuppressModificationEvent
)
{
	FSlot& Slot = Slots[Type][Index];
	if (Slot.Widget->ContentFactory.IsValid())
	{
		ReleaseDynamicSlot(Type, Index, bSuppressModificationEvent);
	}
	Slot.Widget->AssignFactory(FactoryType, FactoryIndex, bSuppressModificationEvent);
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::ReleaseDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent)
{
	auto& TypedSlots = Slots[Type];
	FSlot& Slot = TypedSlots[Index];
	Slot.Widget->UnassignFactory(bSuppressModificationEvent);
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::RemoveDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent)
{
	sketch::FFactory* Factory = ContentFactory.Resolve();
	auto& TypedSlots = Slots[Type];
	FSlot& Slot = TypedSlots[Index];
	Factory->DestroyDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Type, Index, *Slot.Slot);
	TypedSlots.RemoveAt(Index);
	BroadcastModification(bSuppressModificationEvent);
}

SSketchWidget::FArguments::WidgetArgsType& SSketchWidget::FArguments::SetupAsUniqueSlotContainer(const FStringView& SlotName)
{
	_bRoot = false;
	Tag(FName(SlotName));
	return *this;
}

void SSketchWidget::Construct(const FArguments& InArgs)
{
	bRoot = InArgs._bRoot;
	ChildSlot
	[
		SAssignNew(Overlay, SOverlay)

		+ SOverlay::Slot()
		+ SOverlay::Slot()
		[
			SAssignNew(Border, SBorder)
			.BorderImage(FSlateIcon("CoreStyle", "PlainBorder").GetIcon())
			.BorderBackgroundColor(this, &SSketchWidget::GetBorderColor)
			.Visibility(EVisibility::HitTestInvisible)
		]
	];
}

FReply SSketchWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// // When content is present - do not handle left-clicks
	// TSharedRef<SWidget> Content = Overlay->GetChildren()->GetChildAt(0);
	// const bool bHasWidget = Content != SNullWidget::NullWidget;
	// if (bHasWidget && MouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
	// 	return FReply::Unhandled();
	//
	// // List slot controls if content is present and supports slots
	// FMenuBuilder Menu(true, nullptr);
	// if (const sketch::FFactory* Factory = ContentFactory.Resolve(); Factory && Factory->EnumerateDynamicSlotTypes.IsSet())
	// {
	// 	// First list slot constructors
	// 	Menu.AddSeparator(TEXT("make slots"));
	// 	const TArray<sketch::FFactory::FSlotDescriptor> SlotDescriptors = Factory->EnumerateDynamicSlotTypes(Content.Get());
	// 	for (const sketch::FFactory::FSlotDescriptor& Slot : SlotDescriptors)
	// 	{
	// 		FMenuEntryParams Entry;
	// 		Entry.LabelOverride = FText::FromName(Slot.Name);
	// 		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchWidget::OnConstructSlot, Slot.Name);
	//
	// 		// Remember slots can be unique and already occupied
	// 		if (Slot.Type == sketch::ST_Unique)
	// 		{
	// 			const auto* SlotContent = Slots.Find(Slot.Name);
	// 			if (SlotContent && !SlotContent->IsEmpty())
	// 			{
	// 				Entry.DirectActions.CanExecuteAction.BindStatic([] { return false; });
	// 			}
	// 		}
	//
	// 		Menu.AddMenuEntry(Entry);
	// 	}
	//
	// 	// Second list all existing slots destructors
	// 	Menu.AddSeparator(TEXT("remove slots"));
	// 	for (auto It = Slots.CreateIterator(); It; ++It)
	// 	{
	// 		const FName& Type = It->Key;
	// 		const auto& TypedSlots = It->Value;
	// 		FString TypeString = Type.ToString();
	// 		for (auto Slot = TypedSlots.CreateConstIterator(); Slot; ++Slot)
	// 		{
	// 			FMenuEntryParams Entry;
	// 			FString Caption = TypeString + TEXT("#") + FString::FromInt(Slot.GetIndex());
	// 			Entry.LabelOverride = FText::FromString(Caption);
	// 			Entry.DirectActions.ExecuteAction.
	// 			      BindSP(this, &SSketchWidget::OnDestroySlot, Type, Slot.GetIndex());
	// 		}
	// 	}
	//
	// 	Menu.AddSeparator(TEXT("factories"));
	// }
	//
	// // List factories
	// const auto& Host = FSketchModule::Get();
	// for (const auto& [Type, Factory] : Host.Factories)
	// {
	// 	Menu.AddSubMenu(
	// 		FText::FromName(Type),
	// 		FText::GetEmpty(),
	// 		FNewMenuDelegate::CreateSP(this, &SSketchWidget::OnListFactoriesOfType, Type)
	// 	);
	// }
	// FSlateApplication::Get().PushMenu(
	// 	AsShared(),
	// 	FWidgetPath(),
	// 	Menu.MakeWidget(),
	// 	MouseEvent.GetScreenSpacePosition(),
	// 	FPopupTransitionEffect::TypeInPopup
	// );
	return FReply::Handled();
}

void SSketchWidget::BroadcastModification(bool bSuppress)
{
	if (bSuppress) return;

	if (SSketchWidget* Root = FindRoot()) [[likely]]
	{
		Root->OnModification.Broadcast();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SSketchWidget couldn't find a root to broadcast modification on"));
	}
}

void SSketchWidget::OnConstructSlot(FName Name)
{
	sketch::FFactory* Factory = ContentFactory.Resolve();
	FSlot& Slot = Slots.FindOrAdd(Name).Emplace_GetRef();
	Slot.Attributes = MakeShared<TArray<sketch::FAttribute>>();
	Slot.Widget = SNew(SSketchWidget);
	auto& Host = FSketchModule::Get();
	Host.RedirectNewAttributesInto(Slot.Attributes);
	Slot.Slot = &Factory->ConstructDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Name);
	Host.StopRedirectingNewAttributes();
	Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());
	BroadcastModification(false);
}

void SSketchWidget::OnDestroySlot(FName Type, int Index)
{
	sketch::FFactory* Factory = ContentFactory.Resolve();
	auto& TypedSlots = Slots[Type];
	FSlot& Slot = TypedSlots[Index];
	Factory->DestroyDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Type, Index, *Slot.Slot);
	TypedSlots.RemoveAt(Index);
	BroadcastModification(false);
}

void SSketchWidget::OnListFactoriesOfType(FMenuBuilder& SubMenu, FName Type)
{
	const auto& Host = FSketchModule::Get();
	for (auto Factory = Host.Factories[Type].CreateConstIterator(); Factory; ++Factory)
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = FText::FromName(Factory->Name);
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchWidget::OnFactorySelected, Type, Factory.GetIndex());
		SubMenu.AddMenuEntry(Entry);
	}
}

void SSketchWidget::OnFactorySelected(FName Type, int Index)
{
	auto& Host = FSketchModule::Get();
	Overlay->ClearChildren();

	Host.RedirectNewAttributesInto(*this, Attributes);
	const sketch::FFactory& Factory = Host.Factories[Type][Index];
	ContentFactory.Type = Type;
	ContentFactory.Name = Factory.Name;
	TSharedRef<SWidget> Widget = Factory.ConstructWidget();
	Host.StopRedirectingNewAttributes();
	Overlay->AddSlot()[MoveTemp(Widget)];

	Overlay->AddSlot()[Border.ToSharedRef()];

	BroadcastModification(false);
}

FSlateColor SSketchWidget::GetBorderColor() const
{
	constexpr FLinearColor HoveredColor(0, 1, 0);
	constexpr FLinearColor HighlightedColor(0, 1, 1);
	constexpr float Shift = 1.f / 3.f;
	constexpr FLinearColor CombinedColor(
		FMath::Lerp(HoveredColor.R, HighlightedColor.R, Shift),
		FMath::Lerp(HoveredColor.G, HighlightedColor.G, Shift),
		FMath::Lerp(HoveredColor.B, HighlightedColor.B, Shift)
	);

	const bool bHovered = Border->IsDirectlyHovered();
	if (!bHovered && !bHighlighted)
		return FLinearColor::Transparent;
	if (bHovered && bHighlighted)
		return CombinedColor;
	if (bHovered)
		return HoveredColor;
	return HighlightedColor;
}
