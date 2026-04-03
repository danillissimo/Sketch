#include "SketchModule.h"

#include "Editor.h"
#include "SketchCommands.h"
#include "SketchSandbox.h"
#include "SketchStyle.h"
#include "StatusBarSubsystem.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "HeaderTool/SourceCodeUtility.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderTool.h"
#include "Widgets/SSketchSanbox.h"
#include "Widgets/SSketchWidgetEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSketchModule"

static const FName WidgetEditorTabName("SketchWidgetEditor");
static const FName SandboxTabName("SketchSandbox");
static const FName HeaderToolTabName("SketchHeaderTool");



TAttribute<FSlateFontInfo> sketch::Private::DefaultFont()
{
	return Sketch("Font", FAppStyle::Get().GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}



void FSketchModule::StartupModule()
{
	using namespace sketch::SourceCode;
	enum { root, simple_text, complex_text, complex_text_a, complex_text_brackets, complex_text_b };
	auto ses = Match(
		TEXT("q"),
		0,
		&NoFilter,
		Matcher::String(TEXT("ab"), ST_Optional),
		Matcher::String(TEXT("q"))
	);
	// static_assert(ses.MatchResult == MR_Success);
	auto lal = sketch::MakeTuple(123, "a", 1.23);
	__nop();
	static constexpr auto quack = Match(
		TEXT("q"),
		0,
		&NoFilter,
		Matcher::String<0>(TEXT("a")),
		Matcher::Subsequence(
			ST_Optional,
			Matcher::String<1>(TEXT("q"))
			// Matcher::Subsequence(
			// 	ST_Optional,
			// 	Matcher::String<2>(TEXT("q")),
			// 	Matcher::Subsequence(
			// 		ST_Optional,
			// 		Matcher::String<3>(TEXT("q"))
			// 	)
			// )
		)
	);
	constexpr int qwe = []() constexpr
	{
		int a = 0, b = 0;
		auto tie = sketch::Tie(a, b);
		// auto tie = std::tie(a, b);
		return tie.Get<0>() + tie.Get<1>();
		// return 0;
		// return std::get<0>(tie) + std::get<1>(tie);
	}();
	constexpr auto aaq = quack.Get<1>()/*.View(TEXT("q"))*/;
	// static constexpr TMatcher m(
	// 	TEXT("q"),
	// 	Matcher::ClassDeclaration<{}>()
	// );
	// static_assert(m.Match.MatchResult == MR_Success);
	// constexpr auto& lal = ses.Get<root>();
	// ses.Components

	// Core
	FSketchCore::Initialize();

	// Style
	FSketchStyle::Initialize();
	FSketchStyle::ReloadTextures();

	// Tabs
	{
		SketchMenu = WorkspaceMenu::GetMenuStructure().GetToolsCategory()->AddGroup(
			TEXT("Sketch"),
			LOCTEXT("SketchMenuName", "Sketch"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.SlateBrushAsset")
		);
		const TSharedRef<FGlobalTabmanager>& GlobalTabManager = FGlobalTabmanager::Get();
		GlobalTabManager
			->RegisterNomadTabSpawner(
				SandboxTabName,
				FOnSpawnTab::CreateStatic([](const FSpawnTabArgs& Args)
					{
						return SNew(SDockTab)
							.TabRole(NomadTab)
							[
								SNew(SSketchSandbox)
								.ContentPadding(FMargin(4, 0, 4, 4))
							];
					}
				)
			)
			.SetDisplayName(LOCTEXT("SketchSandboxTabName", "Sketch Sandbox"))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.SlateBrushAsset"))
			.SetGroup(SketchMenu.ToSharedRef());
		GlobalTabManager
			->RegisterNomadTabSpawner(
				WidgetEditorTabName,
				FOnSpawnTab::CreateStatic([](const FSpawnTabArgs& Args)
				{
					return SNew(SDockTab)
						.TabRole(NomadTab)
						[
							SNew(SSketchWidgetEditor)
							.ContentPadding(FMargin(4, 0, 4, 4))
						];
				})
			)
			.SetDisplayName(LOCTEXT("SketchWidgetEditorTabName", "Sketch Widget Editor"))
			.SetIcon(FSlateIcon("HeaderViewStyle", "Icons.HeaderView"))
			.SetGroup(SketchMenu.ToSharedRef());
		GlobalTabManager
			->RegisterNomadTabSpawner(
				HeaderToolTabName,
				FOnSpawnTab::CreateStatic([](const FSpawnTabArgs& Args)
				{
					TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(NomadTab);
					TSharedRef<SVerticalBox> Content =
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SNew(SSketchHeaderTool)
							.ContentPadding(FMargin(4, 0))
						];
					Tab->SetContent(Content);
					Tab->RegisterActiveTimer(0, FWidgetActiveTimerDelegate::CreateLambda([C = &Content.Get(), T = &Tab.Get()](double, float)
					{
						// It can happen that Header Tool tab gets instanced before main editor window
						// When UStatusBarSubsystem is not instanced yet
						// This fixes it
						if (UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>())
						{
							C->AddSlot().AutoHeight()
							[
								StatusBarSubsystem->MakeStatusBarWidget(TEXT("Sketch.HeaderToolStatusBar"), StaticCastSharedRef<SDockTab>(T->AsShared()))
							];
							return EActiveTimerReturnType::Stop;
						}
						return EActiveTimerReturnType::Continue;
					}));
					return Tab;
				})
			)
			.SetDisplayName(LOCTEXT("SketchHeaderToolTabName", "Sketch Header Tool"))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"))
			.SetGroup(SketchMenu.ToSharedRef());
	}

	// Commands
	FSketchCommands::Register();
	Commands = MakeShared<FUICommandList>();
	{
		FSketchCommands& SketchCommands = FSketchCommands::Get();
		Commands->MapAction(
			SketchCommands.OpenWidgetEditor,
			FExecuteAction::CreateStatic([] { FGlobalTabmanager::Get()->TryInvokeTab(WidgetEditorTabName); })
		);
		Commands->MapAction(
			SketchCommands.OpenSandbox,
			FExecuteAction::CreateStatic([] { FGlobalTabmanager::Get()->TryInvokeTab(SandboxTabName); })
		);
		Commands->MapAction(
			SketchCommands.OpenHeaderTool,
			FExecuteAction::CreateStatic([] { FGlobalTabmanager::Get()->TryInvokeTab(HeaderToolTabName); })
		);
	}

	// Menus
	// UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSketchModule::RegisterMenus));
}

void FSketchModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WidgetEditorTabName);

	FSketchCommands::Unregister();

	FSketchStyle::Shutdown();

	FSketchCore::Shutdown();
}

void FSketchModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
	Section.AddSubMenu(
		TEXT("Sketch"),
		LOCTEXT("SketchMenuName", "Sketch"),
		FText::GetEmpty(),
		FNewToolMenuDelegate::CreateLambda([this](UToolMenu* Menu)
		{
			FToolMenuSection& Section = Menu->AddSection(TEXT("Sketch"));
			Section.AddMenuEntryWithCommandList(FSketchCommands::Get().OpenWidgetEditor, Commands);
			Section.AddMenuEntryWithCommandList(FSketchCommands::Get().OpenSandbox, Commands);
			Section.AddMenuEntryWithCommandList(FSketchCommands::Get().OpenHeaderTool, Commands);
		})
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSketchModule, Sketch)
