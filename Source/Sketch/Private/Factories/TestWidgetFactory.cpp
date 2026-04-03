#include "Sketch.h"
#include "Sketch/Public/Widgets/SSketchWidget.h"

class SSketchTestWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchTestWidget) {}
		SLATE_ATTRIBUTE(FAnchors, Anchors)
		SLATE_ATTRIBUTE(bool, Boolean)
		SLATE_ATTRIBUTE(const FSlateBrush*, Brush)
		SLATE_ATTRIBUTE(FSlateColor, SlateColor)
		SLATE_ATTRIBUTE(FLinearColor, LinearColor)
		SLATE_ATTRIBUTE(EHorizontalAlignment, Enumeration)
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
		SLATE_ATTRIBUTE(float, Float)
		SLATE_ATTRIBUTE(double, Double)
		SLATE_ATTRIBUTE(uint32, UnsignedInteger)
		SLATE_ATTRIBUTE(int32, SignedInteger)
		SLATE_ATTRIBUTE(FMargin, Margin)
		SLATE_ATTRIBUTE(FName, Name)
		SLATE_ATTRIBUTE(FOptionalSize, OptionalSize)
		SLATE_ATTRIBUTE(TOptional<int32>, Optional)
		SLATE_ATTRIBUTE(FSlateRenderTransform, RenderTransform)
		SLATE_ATTRIBUTE(FString, String)
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FVector, Vector)
		SLATE_ATTRIBUTE(void*, Unsupported)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}
};

void RegisterTestWidgetFactory()
{
	using namespace sketch;
	FFactory Factory;
	Factory.Name = TEXT("SketchTestWidget";)
	Factory.ConstructWidget = [](SWidget* WidgetToTakeUniqueSlotsFrom)
	{
		SSketchTestWidget::FArguments Arguments;
		Arguments
			.Anchors(Sketch(TEXT("Anchors"), FAnchors()))
			.Boolean(Sketch(TEXT("Boolean"), bool()))
			.Brush(Sketch(TEXT("Brush"), (const FSlateBrush*)nullptr))
			.SlateColor(Sketch(TEXT("SlateColor"), FSlateColor()))
			.LinearColor(Sketch(TEXT("LinearColor"), FLinearColor()))
			.Enumeration(Sketch(TEXT("Enumeration"), EHorizontalAlignment()))
			.Font(Sketch(TEXT("Font"), FSlateFontInfo()))
			.Float(Sketch(TEXT("Float"), float()))
			.Double(Sketch(TEXT("Double"), double()))
			.UnsignedInteger(Sketch(TEXT("UnsignedInteger"), uint32()))
			.SignedInteger(Sketch(TEXT("SignedInteger"), int32()))
			.Margin(Sketch(TEXT("Margin"), FMargin()))
			.Name(Sketch(TEXT("Name"), FName()))
			.OptionalSize(Sketch(TEXT("OptionalSize"), FOptionalSize()))
			.Optional(Sketch(TEXT("Optional"), TOptional<int32>()))
			.RenderTransform(Sketch(TEXT("RenderTransform"), FSlateRenderTransform()))
			.String(Sketch(TEXT("String"), FString()))
			.Text(Sketch(TEXT("Text"), FText()))
			.Vector(Sketch(TEXT("Vector"), FVector()))
			.Unsupported(Sketch(TEXT("Unsupported"), nullptr));
		TSharedRef<SSketchTestWidget> Widget = SArgumentNew(Arguments, SSketchTestWidget);
		return MoveTemp(Widget);
	};
	FSketchCore& Core = FSketchCore::Get();
	Core.RegisterFactory(TEXT("SketchTest"), MoveTemp(Factory));
}
