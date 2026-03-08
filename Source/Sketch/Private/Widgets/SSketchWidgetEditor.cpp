#include "Widgets/SSketchWidgetEditor.h"

#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderRow.h"
#include "Widgets/SSketchOutliner.h"
#include "Widgets/SSketchWidget.h"

void SSketchWidgetEditor::Construct(const FArguments& InArgs)
{
	TSharedRef<SSketchWidget> Sketch = SNew(SSketchWidget).bRoot(true);
	TSharedRef<SSketchHeaderRow> HeaderRow = SNew(SSketchHeaderRow)
		.ShowNumUsers(false)
		.ShowLine(false);
	TSharedRef<SSketchAttributeCollection> PropertyEditor = SNew(SSketchAttributeCollection)
		.HeaderRow(HeaderRow);
	TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar);
	ChildSlot
		.Padding(InArgs._ContentPadding)
		[
			SNew(SBorder)
			.BorderImage(FSlateIcon("CoreStyle", "Brushes.Recessed").GetIcon())
			[
				SNew(SSplitter)

				+ SSplitter::Slot()
				.Value(0.15f)
				[
					SNew(SSketchOutliner, Sketch.Get())
					.AttributeCollection(&PropertyEditor.Get())
				]

				+ SSplitter::Slot()
				.Value(0.6f)
				[
					Sketch
				]

				+ SSplitter::Slot()
				.Value(0.25f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							HeaderRow
						]

						+ SVerticalBox::Slot()
						.FillHeight(1)
						[
							SNew(SScrollBox)
							.ExternalScrollbar(ScrollBar)

							+ SScrollBox::Slot()
							.FillSize(1)
							[
								PropertyEditor
							]
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						ScrollBar
					]
				]
			]
		];
}
