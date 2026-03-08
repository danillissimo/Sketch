#include "Widgets/SSketchSanbox.h"

#include "SketchCore.h"
#include "SketchSandbox.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderRow.h"

#define LOCTEXT_NAMESPACE "SSketchSanbox"

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
	TitleMenu.AddPullDownMenu(
		LOCTEXT("Clear", "Clear"),
		LOCTEXT("ClearTooltip", "Clear stale attributes"),
		FNewMenuDelegate::CreateStatic([](FMenuBuilder&) { FSketchCore::Get().ClearStaleAttributes(); })
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
							SNew(SBorder)
							.BorderImage(FSlateIcon("CoreStyle", "Brushes.Background").GetIcon())
							.Padding(4, 0, 0, 0)
							.HAlign(HAlign_Left)
							[
								SNew(SBorder)
								.BorderImage(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Section.Background_Header").GetIcon())
								.BorderBackgroundColor(FSlateColor(EStyleColor::Panel))
								.Padding(16, 4)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("Sandbox", "Sandbox"))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								]
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SNew(SSketchAttributeCollection)
							.Attributes(sketch::Sandbox::Get())
							.HeaderRow(HeaderRow)
							.ShowInteractivity(false)
							.ShowNumUsers(false)
							.AllowCodePatching(false)
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

#undef LOCTEXT_NAMESPACE
