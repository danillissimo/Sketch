#include "Widgets/SSketchWindow.h"

#include "Sketch.h"
#include "Widgets/SSketch.h"
#include "Widgets/SSketchController.h"
#include "Widgets/SSketchHeaderTool.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "SSketchWindow"

void SSketchWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Playground", "Playground"))
				.OnClicked(this, &SSketchWindow::Switch, 0)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Editor", "Editor"))
				.OnClicked(this, &SSketchWindow::Switch, 1)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("HeaderTool", "Header Tool"))
				.OnClicked(this, &SSketchWindow::Switch, 2)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(Switcher, SWidgetSwitcher)

			+ SWidgetSwitcher::Slot()
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				+ SSplitter::Slot()
				[
					SNew(SSketchController)
				]

				+ SSplitter::Slot()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.FillHeight(1.)
					[
						SNew(SSketch)
					]

					+ SVerticalBox::Slot()
					[
						SNew(SImage)
						.Image(Sketch("IconTest", FCoreStyle::Get().GetDefaultBrush()))
					]
				]
			]

			+ SWidgetSwitcher::Slot()
			[
				SNew(STextBlock).Text(INVTEXT("TBD"))
			]

			+ SWidgetSwitcher::Slot()
			[
				SNew(SSketchHeaderTool)
			]
		]
	];
}

FReply SSketchWindow::Switch(int Index)
{
	Switcher->SetActiveWidgetIndex(Index);
	return FReply::Handled();
}
