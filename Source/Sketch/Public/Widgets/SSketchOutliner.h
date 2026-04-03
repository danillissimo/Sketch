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

	void Construct(const FArguments& InArgs);
	void SetRoot(SSketchWidget* Root);

private:
	void Rebuild();

	static void OnGetChildren(TWeakPtr<SSketchWidget> InItem, TArray<TWeakPtr<SSketchWidget>>& Children);
	static TSharedRef<ITableRow> OnGenerateRow(TWeakPtr<SSketchWidget> InItem, const TSharedRef<STableViewBase>& Owner);
	static FReply OnExportRow(TWeakPtr<SSketchWidget> InItem);
	void OnSelectionChanged(TWeakPtr<SSketchWidget> InItem, ESelectInfo::Type SelectionType);

	TSharedPtr<SWidget> OnMakeContextMenu();
	void MakeNewSlotMenu(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> Widget, FName Type);
	void ListFactoriesIfAppropriate(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void ListFactories(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void ListFactoriesOfType(FMenuBuilder& Menu, TWeakPtr<SSketchWidget> Widget, FName FactoriesType, FName SlotType, int SlotIndex);

	static void OnClearWidget(TWeakPtr<SSketchWidget> WeakWidget);
	static void OnMakeEmptySlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType);
	static void OnClearExistingSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	static void OnRemoveSlot(TWeakPtr<SSketchWidget> WeakWidget, FName SlotType, int SlotIndex);
	void OnFactorySelected(FName FactoryType, int FactoryIndex, TWeakPtr<SSketchWidget> Widget, FName SlotType, int SlotIndex);


	void OnSketchUpdated();

	TArray<TWeakPtr<SSketchWidget>> RootAsArray;
	FDelegateHandle RootListener;
	TSharedPtr<STreeView<TWeakPtr<SSketchWidget>>> Tree;

	TWeakPtr<SSketchAttributeCollection> WeakCollection;
	TWeakPtr<SSketchWidget> WeakSelectedWidget;
};
