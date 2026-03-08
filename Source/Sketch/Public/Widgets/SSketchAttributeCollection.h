#pragma once
#include "SketchTypes.h"
#include "Widgets/SCompoundWidget.h"

class SKETCH_API SSketchAttributeCollection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchAttributeCollection) {}
		SLATE_ARGUMENT_DEFAULT(sketch::FAttributeCollectionHandle*, SlotAttributes) = nullptr;
		SLATE_ARGUMENT_DEFAULT(sketch::FAttributeCollectionHandle*, Attributes) = nullptr;
		SLATE_ARGUMENT_DEFAULT(bool, ShowLine) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowName) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowInteractivity) = true;
		SLATE_ARGUMENT_DEFAULT(bool, ShowNumUsers) = true;
		SLATE_ARGUMENT_DEFAULT(bool, AllowCodePatching) = false;
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetSlotAttributes(const sketch::FAttributeCollectionHandle& Attributes);
	void SetAttributes(const sketch::FAttributeCollectionHandle& Attributes);
	void Update();

private:
	EVisibility GetAttributeGroupSpacerVisibility() const;
	void SetAttributes(
		sketch::FAttributeCollectionHandle& AttributesToSet,
		const sketch::FAttributeCollectionHandle& NewAttributes,
		const TSharedPtr<SVerticalBox>& Box,
		bool bActuallyAllowCodePatching
	);

	sketch::FAttributeCollectionHandle SlotAttributes;
	sketch::FAttributeCollectionHandle Attributes;

	TSharedPtr<SVerticalBox> SlotAttributesBox;
	TSharedPtr<SVerticalBox> AttributesBox;

	uint8 bShowLine : 1 = false;
	uint8 bShowName : 1 = false;
	uint8 bShowInteractivity : 1 = false;
	uint8 bShowNumUsers : 1 = false;
	uint8 bAllowCodePatching : 1 = false;
};
