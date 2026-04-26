#include "SketchCore.h"

#include "Factories/BuiltinFactories.h"
#include "Widgets/SSketchWidget.h"

FSketchCore& FSketchCore::Get()
{
	static FSketchCore Instance;
	return Instance;
}

void FSketchCore::Initialize()
{
	RegisterBuiltinFactories();
}

void FSketchCore::Shutdown() {}

TAttribute<FSlateFontInfo> FSketchCore::GetDefaultFont()
{
	// Sketch("Font", FAppStyle::Get().GetFontStyle(TEXT("PropertyWindow.NormalFont")));
	return FCoreStyle::GetDefaultFontStyle("Regular", 8);
}

void FSketchCore::ClearStaleAttributes()
{
	bool bRemovedAnything = false;
	for (auto Context = Attributes.CreateIterator(); Context; ++Context)
	{
		for (auto Attribute = Context->Value->CreateIterator(); Attribute; ++Attribute)
		{
			if (Attribute->Get()->NumUsers == 0)
			{
				Attribute.RemoveCurrentSwap();
				bRemovedAnything = true;
			}
		}
		if (Context->Value->Num() == 0)
		{
			Context.RemoveCurrent();
		}
	}
	if (bRemovedAnything)
		OnAttributesChanged.Broadcast();
}

void FSketchCore::RegisterFactory(const FName& Type, sketch::FFactory&& Factory)
{
	TArray<sketch::FFactory>& CategorizedFactories = Factories.FindOrAdd(Type);
	sketch::FFactory* ExistingFactory = CategorizedFactories.FindByPredicate([&](const sketch::FFactory& SomeFactory) { return SomeFactory.Name == Factory.Name; });
	if (ExistingFactory)
	{
		*ExistingFactory = MoveTemp(Factory);
	}
	else
	{
		CategorizedFactories.Emplace(MoveTemp(Factory));
	}
}

void FSketchCore::SetWidgetEditorTarget(SSketchWidget& NewTarget)
{
	WidgetEditorTarget = StaticCastWeakPtr<SSketchWidget>(NewTarget.AsWeak());
	OnWidgetEditorTargetChanged.Broadcast(&NewTarget);
}

void FSketchCore::ResetWidgetEditorTarget()
{
	if (auto Widget = WidgetEditorTarget.Pin())
	{
		Widget->NotifyEditorDetached();
	}
	WidgetEditorTarget.Reset();
	OnWidgetEditorTargetChanged.Broadcast(nullptr);
}
