#include "Widgets/SSketchWidgetEditor.h"

#include "SketchCore.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderRow.h"
#include "Widgets/SSketchOutliner.h"
#include "Widgets/SSketchWidget.h"
#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SSketchWidgetEditor"

void SSketchWidgetEditor::Construct(const FArguments& InArgs)
{
	WidgetPreview = SNew(SSketchWidget).bRoot(true).bAttachTarget(false);
	TSharedRef<SSketchHeaderRow> HeaderRow = SNew(SSketchHeaderRow)
		.ShowNumUsers(false)
		.ShowLine(false);
	TSharedRef<SSketchAttributeCollection> PropertyEditor = SNew(SSketchAttributeCollection)
		.HeaderRow(HeaderRow);
	TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar);
	TSharedPtr<SBackgroundBlur> PreviewOverlay;

	SSketchWidget* Target = WidgetPreview.Get();
	EVisibility PreviewOverlayVisibility = EVisibility::Collapsed;
	{
		auto& Core = FSketchCore::Get();
		if (TSharedPtr<SSketchWidget> CurrentTarget = Core.GetWidgetEditorTarget().Pin())
		{
			Target = CurrentTarget.Get();
			PreviewOverlayVisibility = EVisibility::Visible;
		}
		Core.OnWidgetEditorTargetChanged.AddSP(this, &SSketchWidgetEditor::OnTargetChanged);
	}

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
					SAssignNew(Outliner, SSketchOutliner)
					.Root(Target)
					.AttributeCollection(&PropertyEditor.Get())
				]

				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SAssignNew(WidgetPreviewOverlay, SOverlay)

					+ SOverlay::Slot()
					[
						WidgetPreview.ToSharedRef()
					]

					+ SOverlay::Slot()
					[
						SAssignNew(PreviewOverlay, SBackgroundBlur)
						.Visibility(PreviewOverlayVisibility)
						.BlurStrength(1.5f)
						.Padding(0.f)
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(FSlateIcon("CoreStyle", "Brushes.Black").GetIcon())
							.BorderBackgroundColor(FLinearColor{ 1, 1, 1, 0.5 })
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.ColorAndOpacity(FLinearColor::White)
								.Text(LOCTEXT("EditorAttachedElsewhere", "Editor is attached elsewhere. Double click to attach back."))
							]
						]
					]
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

						+ SVerticalBox::Slot()
						.MinHeight(50)
						[
							SNew(SSketchWidget)
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
	PreviewOverlay->SetOnMouseDoubleClick(FPointerEventHandler::CreateStatic(&SSketchWidgetEditor::OnPreviewOverlayDoubleClick));
}

void SSketchWidgetEditor::OnTargetChanged(SSketchWidget* SketchWidget)
{
	Outliner->SetRoot(SketchWidget ? SketchWidget : WidgetPreview.Get());
	WidgetPreviewOverlay->GetChildren()->GetChildAt(1)->SetVisibility(SketchWidget ? EVisibility::Visible : EVisibility::Collapsed);
}

FReply SSketchWidgetEditor::OnPreviewOverlayDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	auto& Core = FSketchCore::Get();
	Core.ResetWidgetEditorTarget();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
