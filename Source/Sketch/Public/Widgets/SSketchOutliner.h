#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class SSketchAttributeCollection;
class SSketchWidget;

class SSketchOutliner : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchOutliner) {}
		SLATE_ARGUMENT_DEFAULT(SSketchWidget*, Root) = nullptr;
		SLATE_ARGUMENT_DEFAULT(SSketchAttributeCollection*, AttributeCollection) = nullptr;
	SLATE_END_ARGS()

	SKETCH_API void Construct(const FArguments& InArgs);
	SKETCH_API void SetRoot(SSketchWidget* Root);

private:
	friend class SSketchOutlinerRow;

	void Rebuild();

	static void OnGetChildren(TWeakPtr<SSketchWidget> InItem, TArray<TWeakPtr<SSketchWidget>>& Children);
	TSharedRef<ITableRow> OnGenerateRow(TWeakPtr<SSketchWidget> InItem, const TSharedRef<STableViewBase>& Owner);
	static FReply OnExportRow(TWeakPtr<SSketchWidget> InItem);
	void OnSelectionChanged(TWeakPtr<SSketchWidget> InItem, ESelectInfo::Type SelectionType);

	TSharedPtr<SWidget> OnMakeContextMenu();
	void MakeNewSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName Type);
	TSharedRef<SWidget> MakeNewSlotMenu(TWeakPtr<SSketchWidget> WeakWidget, FName Type);
	void ListFactoriesIfAppropriate(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void ListFactories(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	TSharedRef<SWidget> ListFactories(TWeakPtr<SSketchWidget> WeakWidget);
	void ListFactoriesOfType(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName FactoriesType, FName SlotType, int SlotIndex);
	void ListChildrenToBeReplacedBy(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget);

	void OnClearWidget(TWeakPtr<SSketchWidget> WeakWidget);
	void OnMakeEmptySlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType);
	void OnClearExistingSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void OnRemoveDynamicSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	FReply OnRemoveDynamicSlotWithReply(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void OnResetUniqueSlot(TWeakPtr<SSketchWidget> WeakParent, int Index);
	void OnFactorySelected(FName FactoryType, int FactoryIndex, TWeakPtr<SSketchWidget> Widget, FName SlotType, int SlotIndex);
	void OnReplaceByUniqueSlotContent(TWeakPtr<SSketchWidget> WeakWidget, FName SlotName);
	void OnReplaceByDynamicSlotContent(TWeakPtr<SSketchWidget> WeakWidget, FName Type, int Index);
	void OnReplaceByChild(SSketchWidget* Widget, SSketchWidget* Child);

	void OnSketchUpdated();

	TArray<TWeakPtr<SSketchWidget>> RootAsArray;
	FDelegateHandle RootListener;
	TSharedPtr<SWidget> Hint;
	TSharedPtr<STreeView<TWeakPtr<SSketchWidget>>> Tree;

	TWeakPtr<SSketchAttributeCollection> WeakCollection;
	TWeakPtr<SSketchWidget> WeakSelectedWidget;
};
