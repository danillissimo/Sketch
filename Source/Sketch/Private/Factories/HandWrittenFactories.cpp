#include "Sketch.h"
#include "Sketch/Public/Widgets/SSketchWidget.h"
#include "SketchHeaderToolTypes.h"

template <class T>
void RegisterBox(const TCHAR* Name)
{
	using namespace sketch;
	FFactory Factory;
	Factory.Name = Name;
	Factory.ConstructWidget = [](SWidget* WidgetToTakeUniqueSlotsFrom)
	{
		constexpr bool bStackBox = std::is_same_v<SStackBox, T>;

		typename T::FArguments Arguments;
		TAttributeInitializer<EOrientation> Orientation;
		if constexpr (bStackBox)
		{
			Orientation = Sketch("Orientation", Orient_Horizontal);
			EOrientation EnforceAttributeConstruction = Orientation;
		}
		Arguments.SKETCH_WIDGET_FACTORY_BOILERPLATE();
		TSharedRef<T> Result = SArgumentNew(Arguments, T);
		if constexpr (bStackBox)
		{
			Result->SetOrientation(Orientation); // They forgot to cache it in constructor...
		}
		return MoveTemp(Result);
	};
	Factory.EnumerateDynamicSlotTypes = [](const SWidget& Widget)-> FFactory::FDynamicSlotTypes
	{
		return { TEXT("FSlot") };
	};
	Factory.ConstructDynamicSlot = [](SWidget& Widget, const FName& Name)-> FSlotBase&
	{
		check(Name == TEXT("FSlot"));
		typename T::FSlot& Slot = *static_cast<T&>(Widget)
		                           .AddSlot()
		                           .HAlign(Sketch("HAlign", HAlign_Fill))
		                           .VAlign(Sketch("VAlign", VAlign_Fill))
		                           .Padding(Sketch("Padding", FMargin(0.0f)))
		                           .GetSlot();
		HeaderTool::InitResizingMixinProperties(Slot, true);
		return Slot;
	};
	Factory.DestroyDynamicSlot = [](SWidget& Widget, const FName& Name, int Index, FSlotBase& Slot)
	{
		check(Name == TEXT("FSlot"))
		static_cast<T&>(Widget).RemoveSlot(Slot.GetWidget());
	};
	FSketchCore::Get().RegisterFactory(TEXT("Widgets"), MoveTemp(Factory));
}

void RegisterHandWrittenFactories()
{
	RegisterBox<SHorizontalBox>(TEXT("HorizontalBox"));
	RegisterBox<SVerticalBox>(TEXT("VerticalBox"));
	RegisterBox<SStackBox>(TEXT("StackBox"));
}
