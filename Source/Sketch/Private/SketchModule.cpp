#include "SketchModule.h"
#include "SketchCommands.h"
#include "SketchSandbox.h"
#include "SketchStyle.h"
#include "StatusBarSubsystem.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderTool.h"
#include "Widgets/SSketchSanbox.h"
#include "Widgets/SSketchWidgetEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSketchModule"

static const FName WidgetEditorTabName("SketchWidgetEditor");
static const FName SandboxTabName("SketchSandbox");
static const FName HeaderToolTabName("SketchHeaderTool");



#include "SketchHeaderToolTypes.h"
using namespace sketch;

TAttribute<FSlateFontInfo> Private::DefaultFont()
{
	return Sketch("Font", FAppStyle::Get().GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}



void FSketchModule::StartupModule()
{
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
					TSharedRef<SWidget> Content =
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SNew(SSketchHeaderTool)
							.ContentPadding(FMargin(4, 0))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->MakeStatusBarWidget(TEXT("Sketch.HeaderToolStatusBar"), Tab)
						];
					Tab->SetContent(Content);
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
