// #pragma once
// #include "Widgets/SCompoundWidget.h"
//
// namespace sketch
// {
// 	struct FAttributePath;
// }
//
// class FSketchModule;
//
// class SSketchControllerOLD : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SSketchControllerOLD) {}
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
//
// private:
// 	TSharedRef<ITableRow> OnGenerateRow(
// 		sketch::FAttributePath Item,
// 		const TSharedRef<STableViewBase>& InTree
// 	);
//
// 	void OnGetChildren(sketch::FAttributePath Item, TArray<sketch::FAttributePath>& Children);
// 	static FText GetNumUsers(sketch::FAttributePath Item);
//
// 	void CaptureState();
// 	void OnAttributesChanged();
//
// 	FSketchModule* Host = nullptr;
// 	TArray<sketch::FAttributePath> CapturedState;
// 	TSharedPtr<STreeView<sketch::FAttributePath>> Tree;
// };
