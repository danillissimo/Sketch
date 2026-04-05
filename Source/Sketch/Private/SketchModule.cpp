#include "SketchModule.h"

#include "Editor.h"
#include "SketchCommands.h"
#include "SketchSandbox.h"
#include "SketchStyle.h"
#include "StatusBarSubsystem.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "HeaderTool/SourceCodeUtility.h"
#include "Interfaces/IMainFrameModule.h"
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
	// Core
	FSketchCore::Initialize();

	if (IsRunningCommandlet()) return;

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
			.SetDisplayName(LOCTEXT("SketchAttributesTabName", "Sketch Attributes"))
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
	IMainFrameModule::Get().GetMainFrameCommandBindings()->Append(Commands.ToSharedRef());
}

void FSketchModule::ShutdownModule()
{
	if (!IsRunningCommandlet())
	{
		// Commands
		if (auto* MainFrame = FModuleManager::LoadModulePtr<IMainFrameModule>(TEXT("MainFrame")))
		{
			FSketchCommands& SketchCommands = FSketchCommands::Get();
			MainFrame->GetMainFrameCommandBindings()->UnmapAction(SketchCommands.OpenWidgetEditor);
			MainFrame->GetMainFrameCommandBindings()->UnmapAction(SketchCommands.OpenSandbox);
			MainFrame->GetMainFrameCommandBindings()->UnmapAction(SketchCommands.OpenHeaderTool);
		}
		FSketchCommands::Unregister();

		// Tabs
		const TSharedRef<FGlobalTabmanager>& GlobalTabManager = FGlobalTabmanager::Get();
		GlobalTabManager->UnregisterNomadTabSpawner(WidgetEditorTabName);
		GlobalTabManager->UnregisterNomadTabSpawner(SandboxTabName);
		GlobalTabManager->UnregisterNomadTabSpawner(HeaderToolTabName);

		FSketchStyle::Shutdown();
	}

	// Core
	FSketchCore::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSketchModule, Sketch)
