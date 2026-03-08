#include "Widgets/SSketch.h"

#include "Widgets/SSketchAttributeCollection.h"
#include "Widgets/SSketchOutliner.h"
#include "Widgets/SSketchWidget.h"

// uint32 GetTypeHash(const sketch::FFactoryHandle& Path) { return HashCombineFast(GetTypeHash(Path.Type), GetTypeHash(Path.Name)); }
//
// template <>
// struct TIsValidListItem<sketch::FFactoryHandle>
// {
// 	enum
// 	{
// 		Value = true
// 	};
// };
//
// template <>
// struct TListTypeTraits<sketch::FFactoryHandle>
// {
// public:
// 	using NullableType = sketch::FFactoryHandle;
//
// 	using MapKeyFuncs = TDefaultMapHashableKeyFuncs<sketch::FFactoryHandle, TSharedRef<ITableRow>, false>;
// 	using MapKeyFuncsSparse = TDefaultMapHashableKeyFuncs<sketch::FFactoryHandle, FSparseItemInfo, false>;
// 	using SetKeyFuncs = DefaultKeyFuncs<sketch::FFactoryHandle>;
//
// 	template <typename U>
// 	static void AddReferencedObjects(
// 		FReferenceCollector& Collector,
// 		TArray<sketch::FFactoryHandle>& ItemsWithGeneratedWidgets,
// 		TSet<sketch::FFactoryHandle>& SelectedItems,
// 		TMap<const U*, sketch::FFactoryHandle>& WidgetToItemMap
// 	)
// 	{}
//
// 	static bool IsPtrValid(const sketch::FFactoryHandle& InValue)
// 	{
// 		return !(InValue.Type.IsNone() && InValue.Name.IsNone());
// 	}
//
// 	static void ResetPtr(sketch::FFactoryHandle& InValue)
// 	{
// 		InValue.Type = NAME_None;
// 		InValue.Name = NAME_None;
// 	}
//
// 	static sketch::FFactoryHandle MakeNullPtr()
// 	{
// 		return {};
// 	}
//
// 	static sketch::FFactoryHandle NullableItemTypeConvertToItemType(const sketch::FFactoryHandle& InValue)
// 	{
// 		return InValue;
// 	}
//
// 	static FString DebugDump(sketch::FFactoryHandle InValue)
// 	{
// 		return InValue.Type.ToString() + TEXT(".") + InValue.Name.ToString();
// 	}
//
// 	struct SerializerType
// 	{};
// };

namespace sketch
{
	using FWidgetPath = TArray<uint8, TInlineAllocator<sizeof(uint64) * 2 / sizeof(uint8)>>;
}

void SSketch::Construct(const FArguments& InArgs)
{
	// SVerticalBox::FArguments Factories;
	// const auto& Host = FSketchModule::Get();
	// for (const FSketchModule::FFactory& Factory : Host.Factories)
	// {
	// 	Factories + SVerticalBox::Slot()
	// 		.AutoHeight()
	// 		[
	// 			SNew(SBox)
	// 			[
	// 				SNew(STextBlock)
	// 				.Text(FText::FromName(Factory.Name))
	// 			]
	// 		];
	// }

	// names = { TEXT("a") };
	// STreeView<FName> s;

	TSharedRef<SSketchWidget> Sketch = SNew(SSketchWidget).bRoot(true);
	TSharedRef<SSketchAttributeCollection> PropertyEditor = SNew(SSketchAttributeCollection)
		.AllowCodePatching(false)
		.ShowInteractivity(true)
		.ShowLine(false)
		.ShowName(true)
		.ShowNumUsers(false);
	ChildSlot
	[
		SNew(SSplitter)

		+ SSplitter::Slot()
		[
			SNew(SSketchOutliner, Sketch.Get())
			.AttributeCollection(&PropertyEditor.Get())
		]

		+ SSplitter::Slot()
		[
			MoveTemp(Sketch)

		]

		+ SSplitter::Slot()
		[

			MoveTemp(PropertyEditor)
		]

		// + SHorizontalBox::Slot()
		// .AutoWidth()
		// [
		// ]
		//
		// + SHorizontalBox::Slot()
		// [
		// ]
		//
		// + SHorizontalBox::Slot().AutoWidth()
		// [
		// ]
		// SNew(SSplitter)
		// .Orientation(Orient_Vertical)
		//
		// + SSplitter::Slot()
		// [
		// 	SNullWidget::NullWidget
		// ]
		//
		// + SSplitter::Slot()
		// [
		// 	// SArgumentNew(Factories, SVerticalBox)
		// 	SNew(STreeView<FSketchModule::FFactoryHandle>)
		// 	.TreeItemsSource(&FSketchModule::Get().FactoryTypes)
		// 	.OnGetChildren_Lambda(
		// 		[](FSketchModule::FFactoryHandle Path, TArray<FSketchModule::FFactoryHandle>& Out)
		// 		{
		// 			if (Path.Name.IsNone())
		// 			{
		// 				for (const FSketchModule::FFactory& Factory : FSketchModule::Get().Factories[Path.Type])
		// 				{
		// 					Out.Emplace(Path.Type, Factory.Name);
		// 				}
		// 			}
		// 		}
		// 	)
		// 	.OnGenerateRow_Lambda(
		// 		[](FSketchModule::FFactoryHandle Path, const TSharedRef<STableViewBase>& Tree)
		// 		{
		// 			return SNew(STableRow<FSketchModule::FFactoryHandle>, Tree)
		// 				[
		// 					SNew(STextBlock).Text(FText::FromName(Path.Name.IsNone() ? Path.Type : Path.Name))
		// 				];
		// 		}
		// 	)
		// ]
	];
}

namespace sketch
{
	enum ESocketType
	{
		ST_Generic,
		ST_Named
	};

	struct FWidget;

	struct FSocket
	{
		TArray<FWidget> Slots;
		ESocketType Type;
	};

	struct FWidget
	{
		FFactoryHandle Factory;
		TArray<FAttribute> Attributes;
		TArray<FSocket, TInlineAllocator<1>> Sockets;
	};
}
