#include "Widgets/SSketchWidgetEditor.h"

#include "SketchCore.h"
#include "AttributesTraites/BooleanTraits.h"
#include "AttributesTraites/ColorTraits.h"
#include "AttributesTraites/MarginTraits.h"
#include "AttributesTraites/TextTraits.h"
#include "HeaderTool/StringLiteral.h"
#include "Textures/SlateIcon.h"
#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchHeaderRow.h"
#include "Widgets/SSketchOutliner.h"
#include "Widgets/SSketchWidget.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "SSketchWidgetEditor"

static const FText WidgetEditorDocumentation = LOCTEXT(
	"Documentation",
	"======================================Welcome to Sketch widget editor!======================================\n"
	"This - is a WYSIWYG editor for Slate - a tool to design Slate interfaces without touching code until actually needed.\n"
	"Editing is done trough hierarchy Outliner to the left of this text.\n"
	"Properties of a particular widget are listed to the right when you select one in the Outliner.\n"
	"Once you done - you can copy your design by clicking the \"Copy\" button, present on each widget in the outliner.\n"
	"Note that the widget you click \"Copy\" on will be the root of copied design - any widgets above it will be ignored.\n"
	"Not only Sketch widget editor allows you building interfaces interactively - it allows you doing so wherever is convenient for you.\n"
	"To do so:\n"
	"- Create an instance of SSketchWidget in your layout\n"
	"- Bring it onto your screen\n"
	"- Ctrl + shift + click it\n"
	"And now widget editor is attached to this widget.\n"
	"\n"
	"==============================================Limitations========================================\n"
	"Some things may be represented incorrectly in this editor since Slate was never designed to support a dedicated editor.\n"
	"E.g.most colors that are white by default are replaced by magenta.\n"
	"Or default checkerboard brush may unexpectedly appear here and there.\n"
	"Some of these distortions are produced by Unreal being unaware of what's going on.\n"
	"Others are introduced intentionally to ensure Sketch stability.\n"
	"Potentially nothing can be done to fix them, so you'll need some experience to identify them.\n"
	"Another Sketch widget editor limitation is stability.\n"
	"Most Sketch factories are generated rather than hand-written, and Slate was never prepared for such usage.\n"
	"So some widgets available here are available by mistake, and attempts to use them will lead to crashes.\n"
	"As well as some attributes may have some limitations that Sketch is unable to reflect.\n"
	"And breaking which will lead to crashes as well.\n"
	"Must be said that things are not that bad, and Sketch widget editor is generally stable.\n"
	"But you must be warned before advancing further, because it's not  a b s o l u t e l y  stable.\n"
	"\n"
	"==========================Extending widget editor==========================\n"
	"Extension of widget editor is done primarily via Sketch header tool.\n"
	"Just feed it needed file, and it will generate all the codes you need.\n"
	"In case Sketch header tool is not an option for some reason - you can do its job manually.\n"
	"Create an instance of 'sketch::FFactory' and put it into FSketchCore::Factories.\n"
	"For examples of live factories you can check out 'Sketch/Private/Factories' folder.\n"
	"There are both generated and hand-written factories."
);

template <class T>
T& FindAttribute(SSketchWidget& SketchWidget, const FName& Name)
{
	return static_cast<T&>(
		*SketchWidget
		 .GetAttributes()
		 .FindByPredicate([&](const TSharedPtr<sketch::FAttribute>& Attribute) { return Attribute->GetName() == Name; })
		 ->Get()
		 ->GetValue()
		 .Get()
	);
}

void SSketchWidgetEditor::Construct(const FArguments& InArgs)
{
	WidgetPreview = SNew(SSketchWidget).bRoot(true).bAttachTarget(false);
	auto& Core = FSketchCore::Get();
	[&]
	{
		static const FName EditableTextClassName(SL"EditableText");
		static const FName BoxClassName(SL"Box");

		FName BoxFactoryCategory;
		int BoxFactoryIndex = INDEX_NONE;
		FName EditableTextFactoryCategory;
		int EditableTextFactoryIndex = INDEX_NONE;
		for (const auto& [Category, Factories] : Core.Factories)
		{
			for (auto Factory = Factories.CreateConstIterator(); Factory; ++Factory)
			{
				if (Factory->Name == EditableTextClassName || Factory->Name == BoxClassName)
				{
					if (Factory->Name == EditableTextClassName)
					{
						EditableTextFactoryCategory = Category;
						EditableTextFactoryIndex = Factory.GetIndex();
					}
					else
					{
						BoxFactoryCategory = Category;
						BoxFactoryIndex = Factory.GetIndex();
					}
					if (BoxFactoryIndex != INDEX_NONE && EditableTextFactoryIndex != INDEX_NONE)
					{
						WidgetPreview->AssignFactory(BoxFactoryCategory, BoxFactoryIndex, true);
						FindAttribute<sketch::FMarginAttribute>(*WidgetPreview, SL"Padding").SetValue(FMargin{ 8.f });
						auto UniqueSlots = WidgetPreview->CollectUniqueSlots();
						UniqueSlots[0]->AssignFactory(EditableTextFactoryCategory, EditableTextFactoryIndex, true);
						FindAttribute<sketch::FTextAttribute>(*UniqueSlots[0], SL"Text").SetValue(WidgetEditorDocumentation);
						FindAttribute<sketch::FBooleanAttribute>(*UniqueSlots[0], SL"IsReadOnly").SetValue(true);
						FindAttribute<sketch::TColorAttribute<FSlateColor>>(*UniqueSlots[0], SL"ColorAndOpacity").SetValue(FLinearColor(1, 1, 1, .75f));
						return;
					}
				}
			}
		}
	}();
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
