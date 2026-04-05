#pragma once
#include "Sketch.h"
#include "Components/VerticalBox.h"
#include "Types/ISlateMetaData.h"

class SSketchWidget;


/**
 * Every widget factory should end with calling this macro - it sets up attributes inherited by each widget from SWidget
 */
#define SKETCH_WIDGET_FACTORY_BOILERPLATE()\
	 ToolTipText(Sketch("ToolTipText",                         FText                            {}))\
	/*.ToolTip(Sketch("ToolTip",                                 TSharedPtr<IToolTip>             {}))*/\
	.Cursor(Sketch("Cursor",                                   TOptional<EMouseCursor::Type>    {}))\
	.IsEnabled(Sketch("IsEnabled",                             bool                             { true }))\
	.Visibility(Sketch("Visibility",                           EVisibility                      { EVisibility::Visible }))\
	.ForceVolatile(Sketch("ForceVolatile",                     bool                             { false }))\
	.Clipping(Sketch("Clipping",                               EWidgetClipping                  { EWidgetClipping::Inherit }))\
	.PixelSnappingMethod(Sketch("PixelSnappingMethod",         EWidgetPixelSnapping             { EWidgetPixelSnapping::Inherit }))\
	.FlowDirectionPreference(Sketch("FlowDirectionPreference", EFlowDirectionPreference         { EFlowDirectionPreference::Inherit }))\
	.RenderOpacity(Sketch("RenderOpacity",                     float                            { 1.f }))\
	.RenderTransform(Sketch("RenderTransform",                 TOptional<FSlateRenderTransform> {}))\
	.RenderTransformPivot(Sketch("RenderTransformPivot",       FVector2D                        {}))\
	/*.Tag(Sketch("Tag",                                         FName                            {}))*/\
	/*.AccessibleParams(Sketch("AccessibleParams",               TOptional<FAccessibleWidgetData> {}))*/\
	/*.AccessibleText(Sketch("AccessibleText",                   FText                            {}))*/


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
		// Introduce HAX
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

		// Introduce code generator
		static FName ResizingMixinInitializer = TEXT("ResizingMixinInitializer");
		auto CodeGenerator = [](const FSlotBase* SlotPtr)-> FString
		{
			const T* Slot = (T*)SlotPtr;
			const FMixin* Mixin = static_cast<const FMixin*>(Slot);
			const FAccessor* Accessor = static_cast<const FAccessor*>(Mixin);
			FString Result;
			Result.Reserve(128);
			Result += TEXT(".");
			const FSizeParam::ESizeRule& SizeRule = Accessor->SizeRule;
			constexpr bool bVerticalBox = std::is_same_v<SVerticalBox::FSlot, T>;
			constexpr bool bHorizontalBox = std::is_same_v<SHorizontalBox::FSlot, T>;
			constexpr FStringView ParamName = bVerticalBox ? TEXT("Height") : bHorizontalBox ? TEXT("Width") : TEXT("Size");
			switch (SizeRule)
			{
			case FSizeParam::SizeRule_Auto:
				Result += TEXT("Auto");
				Result += ParamName;
				Result += TEXT("()");
				break;
			case FSizeParam::SizeRule_Stretch:
				Result += TEXT("Fill");
				Result += ParamName;
				Result += TEXT("(");
				Result.Append(FString::SanitizeFloat(Accessor->SizeValue.Get()));
				Result += TEXT(")");
				break;
			case FSizeParam::SizeRule_StretchContent:
				Result += TEXT("FillContent");
				Result += ParamName;
				Result += TEXT("(");
				Result.Append(FString::SanitizeFloat(Accessor->SizeValue.Get()));
				Result += TEXT(", ");
				Result.Append(FString::SanitizeFloat(Accessor->ShrinkSizeValue.Get()));
				Result += TEXT(")");
				break;
			default:
				unimplemented();
			}
			return Result;
		};

		// Bind all the hidden stuff
		FAccessor& Accessor = static_cast<FAccessor&>(Mixin);
		Accessor.SizeRule = Sketch(TEXT("SizeRule"), FSizeParam::SizeRule_Stretch).AddMeta(ResizingMixinInitializer, CodeGenerator);
		auto Bind = [&](decltype(FAccessor::SizeValue)& Attribute, auto&& Initializer)
		{
			bSupportsAttributes
				? Attribute.Assign(Slot, Initializer)
				: Attribute.Assign(Slot, static_cast<float>(Initializer));
		};
		Bind(Accessor.SizeValue, Sketch(TEXT("SizeValue"), 1.f).AddMeta(ResizingMixinInitializer, CodeGenerator));
		Bind(Accessor.ShrinkSizeValue, Sketch(TEXT("ShrinkSizeValue"), 1.f).AddMeta(ResizingMixinInitializer, CodeGenerator));
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
