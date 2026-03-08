#include "Widgets/SSketchController.h"

#include "SketchCore.h"
#include "Widgets/SSketchAttribute.h"

using namespace sketch;

void SSketchController::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	auto& Core = FSketchCore::Get();
	Core.OnAttributesChanged.AddSP(this, &SSketchController::OnAttributesChanged);
	Update();
}

void SSketchController::Update()
{
	auto& Core = FSketchCore::Get();

restart:
	{
		SVerticalBox::FArguments Files;
		for (auto& [Source, Attributes] : Core.GetNomadAttributes())
		{
			SVerticalBox::FArguments Widgets;
			for (const TSharedPtr<FAttribute>& Attribute : *Attributes)
			{
				Widgets + SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSketchAttribute, *Attribute.Get())
					];
				if (GetCanTick())
				{
					SetCanTick(false);
					goto restart;
				}
			}

			Files + SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SExpandableArea)
					.AllowAnimatedTransition(false)
					.HeaderContent()
					[
						SNew(STextBlock).Text(FText::FromName(Source.FileName))
					]
					.BodyContent()
					[
						SArgumentNew(Widgets, SVerticalBox)
					]
				];
		}

		ChildSlot
		[
			SArgumentNew(Files, SVerticalBox)
		];
	}
}

void SSketchController::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Update();
	SetCanTick(false);
}

void SSketchController::OnAttributesChanged()
{
	SetCanTick(true);
}
