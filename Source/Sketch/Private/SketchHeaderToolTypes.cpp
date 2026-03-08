#include "SketchHeaderToolTypes.h"

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
