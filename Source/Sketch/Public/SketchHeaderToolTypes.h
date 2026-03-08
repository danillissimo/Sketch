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
}
