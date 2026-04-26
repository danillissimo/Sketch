#pragma once
#include "HeaderTool/SketchHeaderTool.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

namespace sketch::HeaderTool
{
	struct FLog;
}


namespace sketch::Private
{
	struct FLogItemId
	{
		bool IsValid() const { return FileId != INDEX_NONE || ClassId != INDEX_NONE || SlotId != INDEX_NONE || MessageId != INDEX_NONE; }

		int FileId = INDEX_NONE;
		int ClassId = INDEX_NONE;
		int SlotId = INDEX_NONE;
		int MessageId = INDEX_NONE;
		bool bShortcut = false;

		bool operator==(const FLogItemId& Other) const { return FileId == Other.FileId && ClassId == Other.ClassId && SlotId == Other.SlotId && MessageId == Other.MessageId; }

		friend uint32 GetTypeHash(const FLogItemId& InValue)
		{
			return HashCombineFast(
				GetTypeHash(InValue.FileId),
				GetTypeHash(InValue.ClassId),
				GetTypeHash(InValue.SlotId),
				GetTypeHash(InValue.MessageId)
			);
		}
	};
}

class SSketchLog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchLog) {}
		SLATE_ARGUMENT_DEFAULT(const sketch::HeaderTool::FLog*, Log) = nullptr;
	SLATE_END_ARGS()

	SKETCH_API void Construct(const FArguments& InArgs);

	/** 
	 * This widget is STreeView based, and STreeView doesn't have any means to update immediately.
	 * What makes synchronizing with an async log an unstraightforward task.
	 */
	SKETCH_API void Rebuild(const sketch::HeaderTool::FLog& Log);
	SKETCH_API void Reset();

private:
	void OnGetChildren(sketch::Private::FLogItemId ItemId, TArray<sketch::Private::FLogItemId>& Children);

	TSharedRef<SWidget> MakeFileTitleWidget(sketch::Private::FLogItemId ItemId);
	TSharedRef<SWidget> MakeClassTitleWidget(sketch::Private::FLogItemId ItemId);
	TSharedRef<SWidget> MakeSlotTitleWidget(sketch::Private::FLogItemId ItemId);
	TSharedRef<SWidget> MakeMessageVerbosityIcon(sketch::Private::FLogItemId ItemId);
	TSharedRef<SWidget> MakeMessageSourceButton(sketch::Private::FLogItemId ItemId);
	TSharedRef<SWidget> MakeMessageWidget(sketch::Private::FLogItemId ItemId);
	TSharedRef<ITableRow> OnGenerateRow(sketch::Private::FLogItemId ItemId, const TSharedRef<STableViewBase>& Table);

	FReply OnFileTitleDoubleClick(const FGeometry&, const FPointerEvent&, int FileId) const;
	FReply GoToMessageSource(int MessageId) const;

	const sketch::HeaderTool::FLog* Log = nullptr;
	TArray<sketch::Private::FLogItemId> LogRoot;

	TSharedPtr<STreeView<sketch::Private::FLogItemId>> Tree;
};
