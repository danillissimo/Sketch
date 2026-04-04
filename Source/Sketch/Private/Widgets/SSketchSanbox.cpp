#include "Widgets/SSketchSanbox.h"

#include "SketchCore.h"
#include "SketchSandbox.h"
#include "Components/VerticalBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/StyleColors.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderRow.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/LaYout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "SSketchSanbox"

static const FCheckBoxStyle TabHeaderStyle = []
{
	const FCheckBoxStyle& DefaultStyle = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox");
	FCheckBoxStyle Result = DefaultStyle;
	Result.UncheckedImage = *FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Section.Background_Header").GetIcon();
	Result.UncheckedImage.TintColor = EStyleColor::Recessed;
	Result.UncheckedHoveredImage = Result.UncheckedImage;
	Result.UncheckedHoveredImage.TintColor = EStyleColor::Header;
	Result.UncheckedPressedImage = Result.UncheckedImage;
	Result.UncheckedPressedImage.TintColor = EStyleColor::Header;
	Result.CheckedImage = Result.UncheckedImage;
	Result.CheckedImage.TintColor = EStyleColor::Panel;
	Result.CheckedHoveredImage = Result.CheckedImage;
	Result.CheckedHoveredImage.TintColor = EStyleColor::Header;
	Result.CheckedPressedImage = Result.CheckedImage;
	Result.CheckedPressedImage.TintColor = EStyleColor::Header;
	Result.UndeterminedImage = Result.UncheckedImage;
	Result.UndeterminedImage.TintColor = EStyleColor::Recessed;
	Result.UndeterminedHoveredImage = Result.UndeterminedImage;
	Result.UndeterminedHoveredImage.TintColor = EStyleColor::Header;
	Result.UndeterminedPressedImage = Result.UndeterminedImage;
	Result.UndeterminedPressedImage.TintColor = EStyleColor::Header;
	return Result;
}();

static const FText SandboxDocumentation = LOCTEXT(
	"Documentation",
	"Welcome to Sketch attributes control panel.\n"
	"To explain what it does, the main purpose of the whole plugin must be explained first.\n"
	"For short - Sketch is a tool for un-hardcoding things.\n"
	"Not that much things gets actually hardcoded in a good program, but there are exceptions.\n"
	"A common exception is development time, when things gets coded so they just work somehow, but here and now.\n"
	"Another common Unreal-specific exception is Slate, where most widget attributes are hardcoded - and that's normal.\n"
	"Sketch is a tool to help working with such things.\n"
	"In particular, Sketch allows You to replace all Your hardcoded values with invocation of \"Sketch\" function.\n"
	"And then collects all of this invocations right in this window - above this text.\n"
	"It might be empty right now, so here's a quick start for You:\n"
	"- Replace some hardcoded thing with something like \"Sketch(\"AttributeName\", %default_value%)\"\n"
	"- Invoke Live Coding, probably ? Ctrl + alt + F11. Just saying.\n"
	"- Make Your Sketched code work\n"
	"- Perceive Your \"AttributeName\" appearing in this window\n"
	"- Try editing it\n"
	"- Watch it actually getting changed without full Unreal reboot and code recompilation\n"
	"That's it!\n"
	"\n"
	"Sketch provides following attribute details:\n"
	"- Source location - line and column. It's actually a button.\n"
	"  Hit it to make Sketch bring You right into Your IDE right to the source invocation of Sketch.\n"
	"- Attribute name. You define it on Sketch invocation.\n"
	"- Interactivity. Reflects if changes to this attribute are expected to be applied immediately.\n"
	"- Number of active attribute users - number of live readers of this attribute.\n"
	"- \"Patch code\" button. Earlier You could notice Sketch can bring You to the source invocation of Sketch.\n"
	"  Coincidentally, the same data can be used to update Your source code with new values Your chose here.\n"
	"  So that's exactly what it does.\n"
	"  Note that there are 2 modes of doing this:\n"
	"  - Simple click - simply updates the default value - the second argument of Sketch invocation.\n"
	"  - Ctrl + click - not only updates default value, but completely replaces Sketch invocation with it.\n"
	"    Can obviously be done only once.\n"
	"  Be cautious when using it though - it overwrites the file on disk, so You may loose unsaved changes!\n"
	"- \"Copy code\" button. Copies to clipboard the value that would be inserted in source code on \"Patch code\" button click.\n"
	"- Attribute editor. 'Nough said I guess.\n"
	"\n"
	"Extending Sketch.\n"
	"Sketch comes packed with most common types support, but doesn't support all of them.\n"
	"Note that Sketch doesn't deny any invocations even when Sketch can't handle them.\n"
	"But in case You ask for something it doesn't currently support - You will get a message about this instead of an attribute editor.\n"
	"To add a custom data type editor You'll have to take following steps:\n"
	"- Create a derivative of IAttributeImplementation for Your type.\n"
	"  In most cases You'll want to derive from TCommonAttributeImplementation instead, as it provides a common base for simple cases.\n"
	"- Create a specialization of TAttributeTraits for Your type.\n"
	"  It can be implemented as a TCommonAttributeTraits derivative in common cases.\n"
	"- Make sure both are visible to any related Sketch invocations\n"
	"See existing types implementations for an example.\n"
	"\n"
	"Sandbox.\n"
	"Some data types like colors or fonts may be more convenient to hardcode without linking to Sketch first.\n"
	"It would be more convenient to have a pallet for them.\n"
	"Sandbox provides such a pallet.\n"
	"Sketch holds some unused attributes of such types so You can play with them any moment, and copy chosen values once You're done.\n"
	"To add another type to Sandbox create a global variable of type 'sketch::Sandbox::TItemInitializer'.\n"
	"Its only attribute is displayed name."
);

void SSketchSandbox::Construct(const FArguments& InArgs)
{
	// Prepare components
	TSharedRef<SScrollBar> Scrollbar = SNew(SScrollBar);
	HeaderRow = SNew(SSketchHeaderRow)
		.ShowLine(true)
		.ShowName(true)
		.ShowInteractivity(true)
		.ShowNumUsers(true)
		.AllowCodePatching(true);
	ScrollBox = SNew(SScrollBox)
		.ExternalScrollbar(Scrollbar);
	TSharedPtr<SSplitter> Splitter;
	FMenuBarBuilder TitleMenu(nullptr);
	TitleMenu.AddMenuEntry(
		LOCTEXT("Clear", "Clear"),
		LOCTEXT("ClearTooltip", "Clear stale attributes"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic([]() { FSketchCore::Get().ClearStaleAttributes(); }))
	);

	// Make content
	ChildSlot
		.Padding(InArgs._ContentPadding)
		[
			SNew(SBorder)
			.BorderImage(FSlateIcon("CoreStyle", "Brushes.Recessed").GetIcon())
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					TitleMenu.MakeWidget()
				]

				+ SVerticalBox::Slot()
				.FillHeight(1)
				[
					SAssignNew(Splitter, SSplitter)
					.Orientation(Orient_Vertical)

					+ SSplitter::Slot()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								HeaderRow.ToSharedRef()
							]

							+ SVerticalBox::Slot()
							.FillHeight(1)
							[
								ScrollBox.ToSharedRef()
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							Scrollbar
						]
					]

					+ SSplitter::Slot()
					.SizeRule(SSplitter::SizeToContent)
					.Resizable(true)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(4, 0, 0, 0)
							[
								SNew(SCheckBox)
								.Style(&TabHeaderStyle)
								.IsChecked(this, &SSketchSandbox::IsTabActive, 0)
								.OnCheckStateChanged(this, &SSketchSandbox::OnTabClicked, 0)
								.Padding(FMargin{ 16, 4 })
								[
									SNew(SBox)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("Sandbox", "Sandbox"))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor(1, 1, 1, 0.75f))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(1, 0, 0, 0)
							[
								SNew(SCheckBox)
								.Style(&TabHeaderStyle)
								.IsChecked(this, &SSketchSandbox::IsTabActive, 1)
								.OnCheckStateChanged(this, &SSketchSandbox::OnTabClicked, 1)
								.Padding(FMargin{ 16, 4 })
								[
									SNew(SBox)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("Help", "Help"))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor(1, 1, 1, 0.75f))
									]
								]
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SAssignNew(TabSwitcher, SWidgetSwitcher)

							+ SWidgetSwitcher::Slot()
							[
								SNew(SSketchAttributeCollection)
								.Attributes(sketch::Sandbox::Get())
								.HeaderRow(HeaderRow)
								.ShowInteractivity(false)
								.ShowNumUsers(false)
								.AllowCodePatching(false)
							]

							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.BorderImage(FSlateIcon("CoreStyle", "Brushes.White").GetIcon())
								.BorderBackgroundColor(EStyleColor::Panel)
								[
									SNew(SBox)
									.HeightOverride(83)
									[
										SNew(SScrollBox)
										+ SScrollBox::Slot()
										.Padding(8)
										[
											SNew(SEditableText)
											.Text(SandboxDocumentation)
											.IsReadOnly(true)
										]
									]
								]
							]
						]
					]
				]
			]
		];
	Splitter->SlotAt(1).OnSlotResized().BindSPLambda(
		Splitter.ToSharedRef(),
		[S = Splitter.Get()](float Value)
		{
			SSplitter::FSlot& Slot = S->SlotAt(1);
			Slot.SetSizingRule(SSplitter::FractionOfParent);
			Slot.OnSlotResized().Unbind();
			Slot.SetSizeValue(Value);
		}
	);

	// Make initial attributes
	UpdateAttributes();

	// Listen core
	FSketchCore& SketchCore = FSketchCore::Get();
	SketchCore.OnAttributesChanged.AddSP(this, &SSketchSandbox::ScheduleUpdate);
}

void SSketchSandbox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	UpdateAttributes();
	SetCanTick(false);
}

void SSketchSandbox::ScheduleUpdate()
{
	SetCanTick(true);
}

void SSketchSandbox::UpdateAttributes()
{
	ScrollBox->ClearChildren();

	FSketchCore& SketchCore = FSketchCore::Get();
	const FSketchCore::FNomadAttributes& NomadAttributes = SketchCore.GetNomadAttributes();
	for (auto& [Source, Attributes] : NomadAttributes)
	{
		ScrollBox->AddSlot()
		         .AutoSize()
		[
			SNew(SExpandableArea)
			.InitiallyCollapsed(false)
			.AreaTitle(FText::FromString(Source.FileName.ToString()))
			.BodyContent()
			[
				SNew(SSketchAttributeCollection)
				.Attributes(Attributes)
				.HeaderRow(HeaderRow)
				.AllowCodePatching({})
			]
		];
	}
}

ECheckBoxState SSketchSandbox::IsTabActive(int Tab) const
{
	return TabSwitcher->GetActiveWidgetIndex() == Tab ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchSandbox::OnTabClicked(ECheckBoxState NewTabState, int Tab)
{
	if (NewTabState == ECheckBoxState::Checked)
	{
		TabSwitcher->SetActiveWidgetIndex(Tab);
	}
}

#undef LOCTEXT_NAMESPACE
