#include "Widgets/SSketchWidget.h"

#include "Sketch.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

SLATE_IMPLEMENT_WIDGET(SSketchWidget)

#define LOCTEXT_NAMESPACE "SSketchWidget"

void SSketchWidget::PrivateRegisterAttributes(FSlateAttributeDescriptor::FInitializer&)
{}

SSketchWidget::FSlotReference SSketchWidget::FindDynamicSlotFor(SSketchWidget* Widget)
{
	for (auto& [Type,TypedSlots] : DynamicSlots)
	{
		for (auto SlotIt = TypedSlots.CreateIterator(); SlotIt; ++SlotIt)
		{
			if (SlotIt->Widget.Get() == Widget)
			{
				return { Type, SlotIt.GetIndex(), &*SlotIt };
			}
		}
	}
	return {};
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

	// Cache current factory
	auto& Core = FSketchCore::Get();
	const sketch::FFactory& Factory = Core.Factories[FactoryType][FactoryIndex];
	ContentFactory.Type = FactoryType;
	ContentFactory.Name = Factory.Name;

	// Construct new widget
	Core.RedirectNewAttributesInto(AsSharedSubobject(&Attributes));
	TSharedRef<SWidget> Widget = Factory.ConstructWidget(nullptr);
	Core.StopRedirectingNewAttributes();
	StartListeningNonDynamicAttributes();

	// Compose new content
	Overlay->AddSlot()[MoveTemp(Widget)];
	FinalizeOverlayRebuild();

	// Notify whoever
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::ReplaceBy(SSketchWidget* Widget, bool bSuppressModificationEvent)
{
	// Sanitize
	check(Widget->ContentFactory.IsValid());

	// Make sure widget doesn't get destroyed while we're working
	TSharedRef<SSketchWidget> StrongWidget = SharedThis(Widget);

	// Drop current identity
	UnassignFactory();
	Overlay->ClearChildren();

	// Move given widget fields into this
	ContentFactory = MoveTemp(Widget->ContentFactory);
	Attributes = MoveTemp(Widget->Attributes);
	DynamicSlots = MoveTemp(Widget->DynamicSlots);

	// Move given widget content into this
	TSharedRef<SWidget> Content = Widget->GetContent().AsShared();
	Widget->Overlay->ClearChildren();
	Overlay->AddSlot()[MoveTemp(Content)];
	FinalizeOverlayRebuild();

	// Start listening all non-dynamic attributes
	StartListeningNonDynamicAttributes();
	for (auto& [Type, TypedSlots] : DynamicSlots)
	{
		for (auto Slot = TypedSlots.CreateConstIterator(); Slot; ++Slot)
		{
			for (const TSharedPtr<sketch::FAttribute>& Attribute : *Slot->Attributes)
			{
				if (!Attribute->IsDynamic()) [[unlikely]]
				{
					Attribute->GetValue()->OnValueChanged.AddSP(this, &SSketchWidget::OnSlotNonDynamicAttributeChanged, Type, Slot.GetIndex());
				}
			}
		}
	}

	// Notify whoever
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::UnassignFactory(bool bSuppressModificationEvent)
{
	UnassignFactory();

	Overlay->ClearChildren();
	Overlay->AddSlot();
	FinalizeOverlayRebuild();

	BroadcastModification(bSuppressModificationEvent);
}

int SSketchWidget::AddDynamicSlot(const FName& Type, bool bSuppressModificationEvent)
{
	// Make slot container
	auto& TypedSlots = DynamicSlots.FindOrAdd(Type);
	FSlot& Slot = TypedSlots.Emplace_GetRef();
	Slot.Attributes = MakeShared<TArray<TSharedPtr<sketch::FAttribute>>>();
	Slot.Widget = SNew(SSketchWidget).IsAttachTarget(false);

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

void SSketchWidget::InsertDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent)
{
	// Sanitize
	auto& TypedSlots = DynamicSlots[Type];
	check(Index <= TypedSlots.Num())

	// 
	if (Index < TypedSlots.Num())[[likely]]
	{
		HandleSlotUpdate(Type, Index, [&]
		{
			FSlot& Slot = TypedSlots.InsertDefaulted_GetRef(Index);
			Slot.Attributes = MakeShared<TArray<TSharedPtr<sketch::FAttribute>>>();
			Slot.Widget = SNew(SSketchWidget).IsAttachTarget(false);
		});
		for (TSharedPtr<sketch::FAttribute>& Attribute : *TypedSlots[Index].Attributes)
		{
			if (!Attribute->IsDynamic()) [[unlikely]]
			{
				Attribute->GetValue()->OnValueChanged.AddSP(this, &SSketchWidget::OnSlotNonDynamicAttributeChanged, Type, TypedSlots.Num() - 1);
			}
		}
	}
	else
	{
		AddDynamicSlot(Type, true);
	}

	// Notify whoever
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::AssignDynamicSlot(
	const FName& Type,
	int Index,
	const FName& FactoryType,
	int FactoryIndex,
	bool bSuppressModificationEvent
)
{
	FSlot& Slot = DynamicSlots[Type][Index];
	if (Slot.Widget->ContentFactory.IsValid())
	{
		ReleaseDynamicSlot(Type, Index, bSuppressModificationEvent);
	}
	Slot.Widget->AssignFactory(FactoryType, FactoryIndex, bSuppressModificationEvent);
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::ReassignDynamicSlot(const FName& Type, int Index, SSketchWidget* NewWidget, bool bSuppressModificationEvent)
{
	auto& TypedSlots = DynamicSlots[Type];
	FSlot& Slot = TypedSlots[Index];
	Slot.Slot->DetachWidget();
	Slot.Widget = SharedThis(NewWidget);
	Slot.Widget->SetTag(NAME_None);
	Slot.Slot->AttachWidget(Slot.Widget.ToSharedRef());
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::ReleaseDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent)
{
	auto& TypedSlots = DynamicSlots[Type];
	FSlot& Slot = TypedSlots[Index];
	Slot.Widget->UnassignFactory(bSuppressModificationEvent);
	BroadcastModification(bSuppressModificationEvent);
}

void SSketchWidget::RemoveDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent)
{
	sketch::FFactory* Factory = ContentFactory.Resolve();
	auto& TypedSlots = DynamicSlots[Type];
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
			if (UniqueSlot->GetContentFactory().IsValid())
			{
				Result += TEXT("\r\n.");
				UniqueSlot->GetTag().AppendString(Result);
				Result += TEXT("()\r\n[\r\n\t");
				FString SlotContent = UniqueSlot->GenerateCode();
				SlotContent.ReplaceInline(TEXT("\r\n"), TEXT("\r\n\t"), ESearchCase::CaseSensitive);
				Result += SlotContent;
				Result += TEXT("\r\n]");
			}
		}
	}

	// Add all dynamic slots
	for (const auto& [Type, TypedSlots] : DynamicSlots)
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
			TSet<sketch::FAttribute::FCustomSloteCodeGenerator> SpecialCases;
			auto TryAddGenerator = [&SpecialCases](const TSharedPtr<sketch::FAttribute>& Attribute)-> bool
			{
				for (const auto& [_, MayBeGenerator] : Attribute->Meta)
				{
					const sketch::FAttribute::FCustomSloteCodeGenerator* Generator = MayBeGenerator.TryGet<sketch::FAttribute::FCustomSloteCodeGenerator>();
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
			for (sketch::FAttribute::FCustomSloteCodeGenerator Generator : SpecialCases)
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

void SSketchWidget::NotifyEditorDetached()
{
	check(AttachTargetHint);
	bRoot = false;
	AttachTargetHint->SetVisibility(EVisibility::Visible);
}

SSketchWidget::FArguments::WidgetArgsType& SSketchWidget::FArguments::SetupAsUniqueSlotContainer(const FName& SlotName)
{
	_IsRoot = false;
	_IsAttachTarget = false;
	Tag(SlotName);
	return *this;
}

void SSketchWidget::Construct(const FArguments& InArgs)
{
	bRoot = InArgs._IsRoot;
	if (InArgs._IsAttachTarget)
	{
		AttachTargetHint =
			SNew(SBox)
			.Visibility(EVisibility::Visible)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(2)
			[
				SNew(SBorder)
				.Padding(0.f)
				.BorderImage(FSlateIcon("CoreStyle", "Brushes.Black").GetIcon())
				.BorderBackgroundColor(FLinearColor{ 1, 1, 1, 0.5 })
				[
					SNew(SImage)
					.Image(FSlateIcon("CoreStyle", "Icons.Info.Small").GetIcon())
					.DesiredSizeOverride(FVector2d(10.0, 10.0))
					.ToolTipText(LOCTEXT("AttachTooltip", "Ctrl+shift+click anywhere on this widget to un/attach to Sketch Widget Editor"))
				]
			];
	}
	SAssignNew(Border, SBorder)
	.BorderImage(FSlateIcon("CoreStyle", "PlainBorder").GetIcon())
	.BorderBackgroundColor(this, &SSketchWidget::GetBorderColor)
	.Visibility(EVisibility::HitTestInvisible);

	ChildSlot
	[
		SAssignNew(Overlay, SOverlay)

		+ SOverlay::Slot()
	];
	FinalizeOverlayRebuild();
}

FReply SSketchWidget::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (AttachTargetHint && MouseEvent.IsControlDown() && MouseEvent.IsShiftDown())
	{
		auto& Core = FSketchCore::Get();
		if (!bRoot)
		{
			AttachTargetHint->SetVisibility(EVisibility::Collapsed);
			bRoot = true;
			Core.SetWidgetEditorTarget(*this);
		}
		else
		{
			Core.ResetWidgetEditorTarget();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
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

void SSketchWidget::FinalizeOverlayRebuild()
{
	Overlay->AddSlot()[Border.ToSharedRef()];
	if (AttachTargetHint)
		Overlay->AddSlot()[AttachTargetHint.ToSharedRef()];
}

void SSketchWidget::StartListeningNonDynamicAttributes()
{
	for (TSharedPtr<sketch::FAttribute>& Attribute : Attributes)
	{
		if (!Attribute->IsDynamic()) [[unlikely]]
		{
			Attribute->GetValue()->OnValueChanged.AddSP(this, &SSketchWidget::RebuildWidget);
		}
	}
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
	for (auto& [Type, TypedSlots] : DynamicSlots)
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
	FinalizeOverlayRebuild();
}

void SSketchWidget::UnassignFactory()
{
	for (auto& [Type, TypedSlots] : DynamicSlots)
		for (int Index = TypedSlots.Num() - 1; Index >= 0; Index--)
			RemoveDynamicSlot(Type, Index, true);

	Attributes.Empty();

	ContentFactory.Type = NAME_None;
	ContentFactory.Name = NAME_None;
}

void SSketchWidget::HandleSlotUpdate(FName Type, int Index, auto&& Actions)
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
	auto& TypedSlots = DynamicSlots[Type];
	SWidget& Content = *Overlay->GetChildren()->GetChildAt(0);
	for (int i = TypedSlots.Num() - 1; i >= Index; --i)
	{
		FSlot& Slot = TypedSlots[i];
		Factory->DestroyDynamicSlot(Content, Type, i, *Slot.Slot);
	}

	Actions();

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

void SSketchWidget::OnSlotNonDynamicAttributeChanged(FName Type, int Index)
{
	HandleSlotUpdate(Type, Index, [] {});
}

void SSketchWidget::OnConstructSlot(FName Name)
{
	sketch::FFactory* Factory = ContentFactory.Resolve();
	FSlot& Slot = DynamicSlots.FindOrAdd(Name).Emplace_GetRef();
	Slot.Attributes = MakeShared<TArray<TSharedPtr<sketch::FAttribute>>>();
	Slot.Widget = SNew(SSketchWidget).IsAttachTarget(false);
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
	auto& TypedSlots = DynamicSlots[Type];
	FSlot& Slot = TypedSlots[Index];
	Factory->DestroyDynamicSlot(*Overlay->GetChildren()->GetChildAt(0), Type, Index, *Slot.Slot);
	TypedSlots.RemoveAt(Index);
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

#undef LOCTEXT_NAMESPACE
