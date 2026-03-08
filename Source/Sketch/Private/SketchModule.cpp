#include "SketchModule.h"
#include "Sketch.h"
#include "SketchStyle.h"
#include "SketchCommands.h"
#include "SketchHeaderTool.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "Widgets/SSketch.h"
#include "Widgets/SSketchController.h"
#include "Widgets/SSketchWindow.h"
#include "BuiltinFactories.h"
#include "SketchHeaderToolTypes.h"
using namespace sketch;

#define LOCTEXT_NAMESPACE "FSketchModule"

static const FName SketchTabName("Sketch");

// TTuple<const TSparseArray<FAttribute>*, const FAttribute*> FSketchModule::FindAttribute(const FAttributePath& Path) const
// {
// 	const TSharedPtr<TSparseArray<FAttribute>>* ContextPtr = Attributes.Find(Path.Source);
// 	TSparseArray<FAttribute>* Context = ContextPtr ? ContextPtr->Get() : nullptr;
// 	const FAttribute* Attribute = nullptr;
// 	if (Context)
// 	[[likely]]
// 	{
// 		if (Context->IsValidIndex(Path.Index))
// 		[[likely]]
// 		{
// 			Attribute = &(*Context)[Path.Index];
// 		}
// 	}
// 	return { Context, Attribute };
// }
//
// TTuple<const TSparseArray<FAttribute>*, FAttribute*> FSketchModule::FindAttribute(const FAttributePath& Path)
// {
// 	auto Result = const_cast<const FSketchModule*>(this)->FindAttribute(Path);
// 	return {
// 		Result.Get<0>(),
// 		const_cast<FAttribute*>(Result.Get<1>())
// 	};
// }

void FSketchModule::ClearStaleAttributes()
{
	bool bRemovedAnything = false;
	for (auto Context = Attributes.CreateIterator(); Context; ++Context)
	{
		for (auto Attribute = Context->Value->CreateIterator(); Attribute; ++Attribute)
		{
			if (Attribute->StaleCountdown == 0)
			{
				Attribute.RemoveCurrent();
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

void FSketchModule::RegisterFactory(const FName& Type, sketch::FFactory&& Factory)
{
	Factories.FindOrAdd(Type).Emplace(MoveTemp(Factory));
	FactoryTypes.AddUnique({ Type });
}

TAttribute<FSlateFontInfo> Private::DefaultFont()
{
	return Sketch("Font", FAppStyle::Get().GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

void RegisterBuiltinFactories(FSketchModule& Host)
{
	{
		FFactory Factory;
		Factory.Name = TEXT("SHorizontalBox");
		Factory.ConstructWidget = []
		{
			return SNew(SHorizontalBox)
				.SKETCH_WIDGET_FACTORY_BOILERPLATE();
		};
		Factory.EnumerateDynamicSlotTypes = [](const SWidget& Widget)-> FFactory::FDynamicSlotTypes
		{
			return { TEXT("Horizontal slot") };
		};
		Factory.ConstructDynamicSlot = [](SWidget& Widget, const FName& Name)-> FSlotBase&
		{
			check(Name == TEXT("Horizontal slot"));
			SHorizontalBox::FSlot& Slot = *static_cast<SHorizontalBox&>(Widget).AddSlot().FillWidth(Sketch("FillWidth", 1.f)).GetSlot();
			HeaderTool::InitResizingMixinProperties(Slot, true);
			return Slot;
		};
		Factory.DestroyDynamicSlot = [](SWidget& Widget, const FName& Name, int Index, FSlotBase& Slot)
		{
			check(Name == TEXT("Horizontal slot"))
			static_cast<SHorizontalBox&>(Widget).RemoveSlot(Slot.GetWidget());
		};
		Host.RegisterFactory(TEXT("Containers"), MoveTemp(Factory));
	}
	{
		auto TextFactory = []
		{
			return SNew(STextBlock)
			                       .Text(Sketch("Text", LOCTEXT("TextSample", "Text Sample")))
									// .TextStyle()
			                       .Font(Sketch("Font", FCoreStyle::GetDefaultFontStyle("Regular", 10)))
									// .StrikeBrush()
			                       .ColorAndOpacity(Sketch("ColorAndOpacity", FLinearColor::White))
									// .ShadowOffset()
			                       .ShadowColorAndOpacity(Sketch("ShadowColorAndOpacity", FLinearColor::Black))
			                       .HighlightColor(Sketch("HighlightColor", FLinearColor::Yellow))
									// .HighlightShape()	
			                       .HighlightText(Sketch("HighlightText", FText::GetEmpty()))
			                       .WrapTextAt(Sketch("WrapTextAt", 0.f))
			                       .AutoWrapText(Sketch("AutoWrapText", false))
			                       .WrappingPolicy(Sketch("WrappingPolicy", ETextWrappingPolicy::DefaultWrapping))
			                       .TransformPolicy(Sketch("TransformPolicy", ETextTransformPolicy::None))
			                       .Margin(Sketch("Margin", FMargin(0.f)))
			                       .LineHeightPercentage(Sketch("LineHeightPercentage", 1.f))
			                       .ApplyLineHeightToBottomLine(Sketch("ApplyLineHeightToBottomLine", true))
			                       .Justification(Sketch("Justification", ETextJustify::Left))
			                       .MinDesiredWidth(Sketch("MinDesiredWidth", 0.f))
									// .TextShapingMethod(Sketch("TextShapingMethod", TOptional<ETextShapingMethod>()))	
									// .TextFlowDirection(Sketch("TextFlowDirection", TOptional<ETextFlowDirection>()))
									// .LineBreakPolicy()
									// .OverflowPolicy(Sketch("OverflowPolicy", TOptional<ETextOverflowPolicy>()))
			                       .SimpleTextMode(Sketch("SimpleTextMode", false))
			                       .SKETCH_WIDGET_FACTORY_BOILERPLATE();
		};
		Host.RegisterFactory(TEXT("Text"), { TEXT("STextBlock"), MoveTemp(TextFactory) });
	}
}

void FSketchModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FSketchStyle::Initialize();
	FSketchStyle::ReloadTextures();

	FSketchCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSketchCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSketchModule::PluginButtonClicked),
		FCanExecuteAction()
	);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSketchModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SketchTabName, FOnSpawnTab::CreateRaw(this, &FSketchModule::OnSpawnPluginTab))
	                        .SetDisplayName(LOCTEXT("FSketchTabTitle", "Sketch"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);



	RegisterBuiltinFactories(*this);
	RegisterAll();
	auto& Slate = FSlateApplication::Get();
	Slate.OnPostTick().AddLambda(
		[this](float)
		{
			for (auto& [Source, Context] : Attributes)
			{
				for (FAttribute& Attribute : *Context)
				{
					Attribute.StaleCountdown -= Attribute.StaleCountdown == 0 ? 0 : 1;
				}
			}
		}
	);
}

void FSketchModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSketchStyle::Shutdown();

	FSketchCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SketchTabName);
}

TSharedRef<SDockTab> FSketchModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSketchWindow)
		];
}

void FSketchModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SketchTabName);
}

void FSketchModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FSketchCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FSketchCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSketchModule, Sketch)
