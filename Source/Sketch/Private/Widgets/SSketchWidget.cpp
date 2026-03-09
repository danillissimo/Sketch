#include "Widgets/SSketchWidget.h"

#include "SketchModule.h"
#include "Sketch.h"

SLATE_IMPLEMENT_WIDGET(SSketchWidget)

void SSketchWidget::PrivateRegisterAttributes(FSlateAttributeDescriptor::FInitializer&)
{}

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
	// Unassign current factory if any
	if (ContentFactory.IsValid())
		UnassignFactory();

	// Reset content
	Overlay->ClearChildren();

	// Construct new widget
	auto& Core = FSketchCore::Get();
	const sketch::FFactory& Factory = Core.Factories[FactoryType][FactoryIndex];
	Core.RedirectNewAttributesInto(AsSharedSubobject(&Attributes));
	TSharedRef<SWidget> Widget = Factory.ConstructWidget(nullptr);
	Core.StopRedirectingNewAttributes();

	// Start listening non-dynamic attributes
	for (TSharedPtr<sketch::FAttribute>& Attribute : Attributes)
	{
		if (!Attribute->IsDynamic()) [[unlikely]]
		{
			Attribute->GetValue()->OnValueChanged.AddSP(this, &SSketchWidget::RebuildWidget);
		}
	}

	// Cache current factory
	ContentFactory.Type = FactoryType;
	ContentFactory.Name = Factory.Name;

	// Compose new content
	Overlay->AddSlot()[MoveTemp(Widget)];
	Overlay->AddSlot()[Border.ToSharedRef()];

	// Notify whoever
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

void SSketchWidget::RebuildWidget()
{
	// Some non-dynamic widget attribute was changed
	// Old widget has to be destroyed, but all of its attributes and slots must be preserved
	// Do not detach from current attributes cause they will be reused

	// Preserve old content so we can retrieve unique slots from it
	TSharedRef<SWidget> OldContent = GetContent().AsShared();

	// Reset content
	Overlay->ClearChildren();

	// Reconstruct widget
	auto& Core = FSketchCore::Get();
	const sketch::FFactory& Factory = *Core.Factories[ContentFactory.Type].FindByPredicate([&](const sketch::FFactory& F) { return F.Name == ContentFactory.Name; });
	Core.RedirectNewAttributesInto(AsSharedSubobject(&Attributes));
	TSharedRef<SWidget> Widget = Factory.ConstructWidget(&*OldContent);
	Core.StopRedirectingNewAttributes();

	// Restore dynamic slots
	for (auto& [Type, TypedSlots] : Slots)
	{
		for (FSlot& Slot : TypedSlots)
		{
			Core.RedirectNewAttributesInto(Slot.Attributes);
			Slot.Slot = &Factory.ConstructDynamicSlot(*Widget, Type);
			Core.StopRedirectingNewAttributes();
			Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());
		}
	}

	// Compose new content
	Overlay->AddSlot()[MoveTemp(Widget)];
	Overlay->AddSlot()[Border.ToSharedRef()];
}

void SSketchWidget::OnSlotNonDynamicAttributeChanged(FName Type, int Index)
{
	// We don't currently have any means to remake a single slot
	// And it's unlikely they will ever appear
	// 'Cause different widgets' slot order is configured differently
	// And it's very hard to determine correct way to recreate a slot procedurally
	// But slots are usually stored in a kind of internal stack
	// So a single slot can be recreated by destroying it and constructing it again at the same depth of the stack
	// But to access this exact position - the entire city must be purged
	// Or at least slots occupying indices larger than the one to be remade
	// So kill all slots starting from the requested one, and then remake them all back
	sketch::FFactory* Factory = ContentFactory.Resolve();
	auto& TypedSlots = Slots[Type];
	SWidget& Content = *Overlay->GetChildren()->GetChildAt(0);
	for (int i = TypedSlots.Num() - 1; i >= Index; --i)
	{
		FSlot& Slot = TypedSlots[i];
		Factory->DestroyDynamicSlot(Content, Type, i, *Slot.Slot);
	}
	auto& Core = FSketchCore::Get();
	for (int i = Index; i < TypedSlots.Num(); ++i)
	{
		FSlot& Slot = TypedSlots[i];
		Core.RedirectNewAttributesInto(Slot.Attributes);
		Slot.Slot = &Factory->ConstructDynamicSlot(Content, Type);
		Core.StopRedirectingNewAttributes();
		Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());
	}
}

void SSketchWidget::UnassignFactory(bool bSuppressModificationEvent)
{
	UnassignFactory();

	Overlay->ClearChildren();
	Overlay->AddSlot();
	Overlay->AddSlot()[Border.ToSharedRef()];

	BroadcastModification(bSuppressModificationEvent);
}

int SSketchWidget::AddDynamicSlot(const FName& Type, bool bSuppressModificationEvent)
{
	// Make slot container
	auto& TypedSlots = Slots.FindOrAdd(Type);
	FSlot& Slot = TypedSlots.Emplace_GetRef();
	Slot.Attributes = MakeShared<TArray<TSharedPtr<sketch::FAttribute>>>();
	Slot.Widget = SNew(SSketchWidget);

	// Make slot
	auto& Core = FSketchCore::Get();
	sketch::FFactory* Factory = ContentFactory.Resolve();
	Core.RedirectNewAttributesInto(Slot.Attributes);
	Slot.Slot = &Factory->ConstructDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Type);
	Core.StopRedirectingNewAttributes();
	Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());

	// Start listening non-dynamic attributes
	for (TSharedPtr<sketch::FAttribute>& Attribute : *Slot.Attributes)
	{
		if (!Attribute->IsDynamic()) [[unlikely]]
		{
			Attribute->GetValue()->OnValueChanged.AddSP(this, &SSketchWidget::OnSlotNonDynamicAttributeChanged, Type, TypedSlots.Num() - 1);
		}
	}

	// Notify whoever
	BroadcastModification(bSuppressModificationEvent);

	// Return new slot index
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

FString SSketchWidget::GenerateCode() const
{
	// Make sure there's anything to do
	sketch::FFactory* Factory = ContentFactory.Resolve();
	if (!Factory) return {};

	// Init resulting string
	FString Result;
	Result.Reserve(1024);

	// Make widget constructor
	Result += TEXT("SNew(S");
	ContentFactory.Name.AppendString(Result);
	Result += TEXT(")");

	// Add all modified attributes
	for (const TSharedPtr<sketch::FAttribute>& Attribute : Attributes)
	{
		if (Attribute->IsSetToDefault()) continue;

		Result += TEXT("\r\n.");
		Attribute->GetName().AppendString(Result);
		Result += TEXT("(");
		Result += Attribute->GetValue()->GenerateCode();
		Result += TEXT(")");
	}

	// Add all non-empty unique slots
	if (Factory->EnumerateUniqueSlots)
	{
		sketch::FFactory::FUniqueSlots UniqueSlots = Factory->EnumerateUniqueSlots(GetContent());
		for (SSketchWidget* UniqueSlot : UniqueSlots)
		{
			Result += TEXT("\r\n.");
			UniqueSlot->GetTag().AppendString(Result);
			Result += TEXT("()\r\n[\r\n\t");
			Result += UniqueSlot->GenerateCode();
			Result += TEXT("\r\n]");
		}
	}

	// Add all dynamic slots
	for (const auto& [Type, TypedSlots] : Slots)
	{
		for (const FSlot& Slot : TypedSlots)
		{
			Result += TEXT("\r\n\r\n+ S");
			ContentFactory.Name.AppendString(Result);
			Result += TEXT("::");
			FString TypeString = Type.ToString();
			Result += FStringView(TypeString).RightChop(1);
			Result += TEXT("()");

			// Add all slot properties
			// Here's a little nuance
			// Skip attributes with special code generators, but don't completely discard them
			// Collect their generators and run them later
			TSet<sketch::FAttribute::FCodeGenerator> SpecialCases;
			auto TryAddGenerator = [&SpecialCases](const TSharedPtr<sketch::FAttribute>& Attribute)-> bool
			{
				for (const auto& [_,MayBeGenerator] : Attribute->Meta)
				{
					const sketch::FAttribute::FCodeGenerator* Generator = MayBeGenerator.TryGet<sketch::FAttribute::FCodeGenerator>();
					if (Generator && *Generator)
					{
						SpecialCases.Add(*Generator);
						return true;
					}
				}
				return false;
			};
			for (const TSharedPtr<sketch::FAttribute>& Attribute : *Slot.Attributes)
			{
				if (Attribute->IsSetToDefault()) continue;
				if (TryAddGenerator(Attribute)) continue;

				Result += TEXT("\r\n.");
				Attribute->GetName().AppendString(Result);
				Result += TEXT("(");
				Result += Attribute->GetValue()->GenerateCode();
				Result += TEXT(")");
			}

			// Run all generators
			for (sketch::FAttribute::FCodeGenerator Generator : SpecialCases)
			{
				FString CustomCode = Generator(Slot.Slot);
				Result += MoveTemp(CustomCode);
			}

			// Add slot content
			constexpr FStringView Tab(TEXT("\t"));
			FString SlotContent = Tab + Slot.Widget->GenerateCode();
			if (SlotContent != Tab)
			{
				SlotContent.ReplaceInline(TEXT("\r\n"), TEXT("\r\n\t"), ESearchCase::CaseSensitive);
				SlotContent.RemoveFromEnd(Tab, ESearchCase::CaseSensitive);
				Result += TEXT("\r\n[\r\n");
				Result += SlotContent;
				Result += TEXT("\r\n]");
			}
		}
	}

	return Result;
}

SSketchWidget::FArguments::WidgetArgsType& SSketchWidget::FArguments::SetupAsUniqueSlotContainer(const FName& SlotName)
{
	_bRoot = false;
	Tag(SlotName);
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

sketch::FFactory::FUniqueSlots SSketchWidget::CollectUniqueSlots(SWidget& Content) const
{
	if (!ContentFactory.IsValid())
		return {};

	const sketch::FFactory* Factory = ContentFactory.Resolve();
	if (!Factory) [[unlikely]]
		return {};

	return Factory->EnumerateUniqueSlots.IsSet()
		       ? Factory->EnumerateUniqueSlots(Content)
		       : sketch::FFactory::FUniqueSlots{};
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
	Slot.Attributes = MakeShared<TArray<TSharedPtr<sketch::FAttribute>>>();
	Slot.Widget = SNew(SSketchWidget);
	auto& Core = FSketchCore::Get();
	Core.RedirectNewAttributesInto(Slot.Attributes);
	Slot.Slot = &Factory->ConstructDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Name);
	Core.StopRedirectingNewAttributes();
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
	const auto& Core = FSketchCore::Get();
	for (auto Factory = Core.Factories[Type].CreateConstIterator(); Factory; ++Factory)
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = FText::FromName(Factory->Name);
		Entry.DirectActions.ExecuteAction.BindSP(this, &SSketchWidget::OnFactorySelected, Type, Factory.GetIndex());
		SubMenu.AddMenuEntry(Entry);
	}
}

void SSketchWidget::OnFactorySelected(FName Type, int Index)
{
	auto& Core = FSketchCore::Get();
	Overlay->ClearChildren();

	Core.RedirectNewAttributesInto(AsSharedSubobject(&Attributes));
	const sketch::FFactory& Factory = Core.Factories[Type][Index];
	ContentFactory.Type = Type;
	ContentFactory.Name = Factory.Name;
	TSharedRef<SWidget> Widget = Factory.ConstructWidget(nullptr);
	Core.StopRedirectingNewAttributes();
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
