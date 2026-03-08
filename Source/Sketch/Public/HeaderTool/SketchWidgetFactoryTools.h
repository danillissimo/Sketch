#pragma once
#include "Sketch.h"
#include "Types/ISlateMetaData.h"

class SSketchWidget;

namespace sketch::HeaderTool
{
	class FUniqueSlotMeta : public ISlateMetaData
	{
	public:
		SLATE_METADATA_TYPE(FUniqueSlotMeta, ISlateMetaData)
		TMap<FName, TWeakPtr<SSketchWidget>> Slots;
		SKETCH_API TSharedRef<SSketchWidget> AddSlot(const SWidget* OldWidget, const FName& SlotType);

		SKETCH_API static FFactory::FUniqueSlots GetSlots(SWidget& Widget);
	};

	template <class T>
	void InitResizingMixinProperties(T& Slot, bool bSupportsAttributes)
	{
		using FMixin = TResizingWidgetSlotMixin<T>;
		FMixin& Mixin = static_cast<FMixin&>(Slot);
		class FAccessor : public FMixin
		{
		public:
			using FMixin::SizeRule;
			using FMixin::SizeValue;
			using FMixin::ShrinkSizeValue;
			using FMixin::MinSize;
			using FMixin::MaxSize;
		};
		FAccessor& Accessor = static_cast<FAccessor&>(Mixin);
		Accessor.SizeRule = Sketch(TEXT("SizeRule"), FSizeParam::SizeRule_Stretch);
		auto Bind = [&](decltype(FAccessor::SizeValue)& Attribute, auto&& Initializer)
		{
			bSupportsAttributes
				? Attribute.Assign(Slot, Initializer)
				: Attribute.Assign(Slot, static_cast<float>(Initializer));
		};
		Bind(Accessor.SizeValue, Sketch(TEXT("SizeValue"), 1.f));
		Bind(Accessor.ShrinkSizeValue, Sketch(TEXT("ShrinkSizeValue"), 1.f));
		Bind(Accessor.MinSize, Sketch(TEXT("MinSize"), 0.f));
		Bind(Accessor.MaxSize, Sketch(TEXT("MaxSize"), 0.f));
	}

	template <class T>
	void DestroyDynamicSlot(SWidget& InWidget, const FName& SlotType, int Index, FSlotBase& Slot)
	{
		// Multiple ways to remove a widget are prepared
		// But looks like it's always by a shared ref
		// So be it
		T& Widget = static_cast<T&>(InWidget);
		Widget.RemoveSlot(Slot.GetWidget());
	}
}
