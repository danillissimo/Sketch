#include "SketchCore.h"

#include "Factories/BuiltinFactories.h"

FSketchCore& FSketchCore::Get()
{
	static FSketchCore Instance;
	return Instance;
}

void FSketchCore::Initialize()
{
	RegisterBuiltinFactories();

	// auto& Slate = FSlateApplication::Get();
	// Slate.OnPostTick().AddLambda(
	// 	[this](float)
	// 	{
	// 		for (auto& [Source, Context] : Attributes)
	// 		{
	// 			for (const TSharedPtr<FAttribute>& Attribute : Context)
	// 			{
	// 				Attribute->StaleCountdown -= Attribute->StaleCountdown == 0 ? 0 : 1;
	// 			}
	// 		}
	// 	}
	// );
}

void FSketchCore::Shutdown() {}

void FSketchCore::ClearStaleAttributes()
{
	bool bRemovedAnything = false;
	for (auto Context = Attributes.CreateIterator(); Context; ++Context)
	{
		for (auto Attribute = Context->Value->CreateIterator(); Attribute; ++Attribute)
		{
			if (Attribute->Get()->StaleCountdown == 0)
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
