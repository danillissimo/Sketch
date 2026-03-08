#include "Widgets/SSketchAttributeCollection.h"

#include "Widgets/SSketchAttribute.h"
#include "Sketch.h"

#define LOCTEXT_NAMESPACE "SSketchAttributeCollection"

void SSketchAttributeCollection::Construct(const FArguments& InArgs)
{
	WeakHeader = InArgs._HeaderRow;
	bShowLine = InArgs._ShowLine;
	bShowName = InArgs._ShowName;
	bShowInteractivity = InArgs._ShowInteractivity;
	bShowNumUsers = InArgs._ShowNumUsers;
	bAllowCodePatching = InArgs._AllowCodePatching;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SlotAttributesBox, SVerticalBox)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSpacer)
			.Size(8)
			.Visibility(this, &SSketchAttributeCollection::GetAttributeGroupSpacerVisibility)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(AttributesBox, SVerticalBox)
		]
	];
	if (InArgs._SlotAttributes)
		SetSlotAttributes(InArgs._SlotAttributes);
	if (InArgs._Attributes)
		SetAttributes(InArgs._Attributes);
}

void SSketchAttributeCollection::SetSlotAttributes(const sketch::FConstAttributeCollection& InAttributes)
{
	SetAttributes(SlotAttributes, InAttributes, SlotAttributesBox, false);
}

void SSketchAttributeCollection::SetAttributes(const sketch::FConstAttributeCollection& InAttributes)
{
	SetAttributes(Attributes, InAttributes, AttributesBox, bAllowCodePatching);
}

void SSketchAttributeCollection::Update()
{
	SetSlotAttributes({ SlotAttributes.Pin() });
	SetAttributes({ Attributes.Pin() });
}

EVisibility SSketchAttributeCollection::GetAttributeGroupSpacerVisibility() const
{
	return SlotAttributesBox->GetChildren()->Num() > 0 && AttributesBox->GetChildren()->Num() > 0
		       ? EVisibility::Visible
		       : EVisibility::Collapsed;
}

void SSketchAttributeCollection::SetAttributes(
	TWeakPtr<const TArray<const TSharedPtr<sketch::FAttribute>>>& WeakAttributesToSet,
	const sketch::FConstAttributeCollection& NewAttributes,
	const TSharedPtr<SVerticalBox>& Box,
	const TOptional<bool>& bActuallyAllowCodePatching
)
{
	Box->ClearChildren();

	WeakAttributesToSet = NewAttributes.Attributes;
	if (!WeakAttributesToSet.IsValid()) return;

	for (const TSharedPtr<sketch::FAttribute>& Attribute : *NewAttributes)
	{
		Box->AddSlot()
		   .AutoHeight()
		   .Padding(0, 0, 0, 1)
		[
			SNew(SSketchAttribute, *Attribute)
			.HeaderRow(WeakHeader)
			.ShowLine(bShowLine)
			.ShowName(bShowName)
			.ShowInteractivity(bShowInteractivity)
			.ShowNumUsers(bShowNumUsers)
			.AllowCodePatching(bActuallyAllowCodePatching)
		];
	}
}

#undef LOCTEXT_NAMESPACE
