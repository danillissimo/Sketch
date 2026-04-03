#pragma once
#include "SketchTypes.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SSketchHeaderRow;

class SKETCH_API SSketchAttributeCollection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchAttributeCollection) {}
		SLATE_ARGUMENT(sketch::FConstAttributeCollection, SlotAttributes)
		SLATE_ARGUMENT(sketch::FConstAttributeCollection, Attributes)
		SLATE_ARGUMENT_DEFAULT(TWeakPtr<SSketchHeaderRow>, HeaderRow) = nullptr;
		/** Following settings prevail over HeaderRow state when provided, though doesn't change general column visibility */
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowLine);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowName);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowInteractivity);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, ShowNumUsers);
		SLATE_ARGUMENT_DEFAULT(TOptional<bool>, AllowCodePatching) = false;
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetSlotAttributes(const sketch::FConstAttributeCollection& Attributes);
	void SetAttributes(const sketch::FConstAttributeCollection& Attributes);
	void Update();

private:
	EVisibility GetAttributeGroupSpacerVisibility() const;
	void SetAttributes(
		TWeakPtr<const TArray<const TSharedPtr<sketch::FAttribute>>>& WeakAttributesToSet,
		const sketch::FConstAttributeCollection& NewAttributes,
		const TSharedPtr<SVerticalBox>& Box,
		const TOptional<bool>& bActuallyAllowCodePatching
	);

	TWeakPtr<const TArray<const TSharedPtr<sketch::FAttribute>>> SlotAttributes;
	TWeakPtr<const TArray<const TSharedPtr<sketch::FAttribute>>> Attributes;

	TSharedPtr<SVerticalBox> SlotAttributesBox;
	TSharedPtr<SVerticalBox> AttributesBox;

	TWeakPtr<SSketchHeaderRow> WeakHeader;
	TOptional<bool> bShowLine;
	TOptional<bool> bShowName;
	TOptional<bool> bShowInteractivity;
	TOptional<bool> bShowNumUsers;
	TOptional<bool> bAllowCodePatching;
};
