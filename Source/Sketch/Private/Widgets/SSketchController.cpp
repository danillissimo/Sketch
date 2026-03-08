#include "Widgets/SSketchController.h"

#include "SketchModule.h"
#include "Widgets/SSketchAttribute.h"

using namespace sketch;

void SSketchController::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	auto& Host = FSketchModule::Get();
	Host.OnAttributesChanged.AddSP(this, &SSketchController::OnAttributesChanged);
	Update();
}

void SSketchController::Update()
{
	auto& Host = FSketchModule::Get();

restart:
	{
		SVerticalBox::FArguments Files;
		for (auto& [Source, Attributes] : Host.Attributes)
		{
			FAttributeHandle Handle;
			Handle.CollectionHandle.Owner = StaticCastSharedPtr<const void>(Attributes);
			Handle.CollectionHandle.Container.Set<TSparseArray<FAttribute>*>(Attributes.Get());

			SVerticalBox::FArguments Widgets;
			for (int j = 0; j < Attributes->Num(); ++j)
			{
				Handle.Index = j;
				Widgets + SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSketchAttribute, Handle)
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
