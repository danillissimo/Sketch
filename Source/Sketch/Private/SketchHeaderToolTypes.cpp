#include "SketchHeaderToolTypes.h"

#include "Widgets/SSketchWidget.h"

TSharedRef<SSketchWidget> sketch::HeaderTool::FUniqueSlotMeta::AddSlot(const SWidget* OldWidget, const FName& SlotType)
{
	TSharedPtr<SSketchWidget> Result;
	if (OldWidget)
	{
		TSharedPtr<FUniqueSlotMeta> Meta = OldWidget->GetMetaData<FUniqueSlotMeta>();
		TWeakPtr<SSketchWidget>& WeakSlot = Meta->Slots[SlotType];
		Result = WeakSlot.Pin();
	}
	else
	{
		Result = SNew(SSketchWidget).SetupAsUniqueSlotContainer(SlotType);
	}
	Slots.Add(SlotType, Result);
	return Result.ToSharedRef();
}

sketch::FFactory::FUniqueSlots sketch::HeaderTool::FUniqueSlotMeta::GetSlots(SWidget& Widget)
{
	FFactory::FUniqueSlots Result;
	if (TSharedPtr<FUniqueSlotMeta> Meta = Widget.GetMetaData<FUniqueSlotMeta>()) [[likely]]
	{
		Result.Reserve(Meta->Slots.Num());
		for (const auto& [SlotName, WeakWidget] : Meta->Slots)
		{
			if (TSharedPtr<SSketchWidget> SlotContainer = WeakWidget.Pin()) [[likely]]
			{
				Result.Add(SlotContainer.Get());
			}
		}
	}
	return Result;
}
