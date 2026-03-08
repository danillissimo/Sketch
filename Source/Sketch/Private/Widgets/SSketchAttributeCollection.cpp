#include "Widgets/SSketchAttributeCollection.h"

#include "Widgets/SSketchAttribute.h"
#include "Sketch.h"

void SSketchAttributeCollection::Construct(const FArguments& InArgs)
{
	bShowLine = InArgs._ShowLine;
	bShowName = InArgs._ShowName;
	bShowInteractivity = InArgs._ShowInteractivity;
	bShowNumUsers = InArgs._ShowNumUsers;
	bAllowCodePatching = InArgs._AllowCodePatching;

	ChildSlot
	[
		SNew(SScrollBox)

		+ SScrollBox::Slot()
		.FillSize(1)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
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

			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(AttributesBox, SVerticalBox).Clipping(EWidgetClipping::ClipToBounds)
			]
		]
	];
	if (InArgs._SlotAttributes)
		SetSlotAttributes(*InArgs._SlotAttributes);
	if (InArgs._Attributes)
		SetAttributes(*InArgs._Attributes);
}

void SSketchAttributeCollection::SetSlotAttributes(const sketch::FAttributeCollectionHandle& InAttributes)
{
	SetAttributes(SlotAttributes, InAttributes, SlotAttributesBox, false);
}

void SSketchAttributeCollection::SetAttributes(const sketch::FAttributeCollectionHandle& InAttributes)
{
	SetAttributes(Attributes, InAttributes, AttributesBox, bAllowCodePatching);
}

void SSketchAttributeCollection::Update()
{
	SetSlotAttributes(SlotAttributes);
	SetAttributes(Attributes);
}

EVisibility SSketchAttributeCollection::GetAttributeGroupSpacerVisibility() const
{
	return SlotAttributesBox->GetChildren()->Num() > 0 && AttributesBox->GetChildren()->Num() > 0
		       ? EVisibility::Visible
		       : EVisibility::Collapsed;
}

void SSketchAttributeCollection::SetAttributes(
	sketch::FAttributeCollectionHandle& AttributesToSet,
	const sketch::FAttributeCollectionHandle& NewAttributes,
	const TSharedPtr<SVerticalBox>& Box,
	bool bActuallyAllowCodePatching
)
{
	Box->ClearChildren();

	AttributesToSet = NewAttributes;
	if (!AttributesToSet.IsValid()) return;

	AttributesToSet << [&](sketch::FAttribute& Attribute, const auto& It)
	{
		Box->AddSlot().AutoHeight()
		[
			SNew(SSketchAttribute, sketch::FAttributeHandle{ .CollectionHandle = AttributesToSet, .Index = It.GetIndex() })
			.ShowLine(bShowLine)
			.ShowName(bShowName)
			.ShowInteractivity(bShowInteractivity)
			.ShowNumUsers(bShowNumUsers)
			.AllowCodePatching(bActuallyAllowCodePatching)
		];
	};
}
