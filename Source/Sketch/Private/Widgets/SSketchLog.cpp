#include "Widgets/SSketchLog.h"

#include "Sketch.h"
#include "SourceCodeNavigation.h"
#include "HeaderTool/StringLiteral.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SSketchLog"

template <>
struct TIsValidListItem<sketch::Private::FLogItemId>
{
	enum { Value = true };
};

template <>
struct TListTypeTraits<sketch::Private::FLogItemId>
{
public:
	typedef sketch::Private::FLogItemId NullableType;

	using MapKeyFuncs = TDefaultMapHashableKeyFuncs<sketch::Private::FLogItemId, TSharedRef<ITableRow>, false>;
	using MapKeyFuncsSparse = TDefaultMapHashableKeyFuncs<sketch::Private::FLogItemId, FSparseItemInfo, false>;
	using SetKeyFuncs = DefaultKeyFuncs<sketch::Private::FLogItemId>;

	template <typename U>
	static void AddReferencedObjects(
		FReferenceCollector& Collector,
		TArray<sketch::Private::FLogItemId>& ItemsWithGeneratedWidgets,
		TSet<sketch::Private::FLogItemId>& SelectedItems,
		TMap<const U*, sketch::Private::FLogItemId>& WidgetToItemMap)
	{
	}

	static bool IsPtrValid(const sketch::Private::FLogItemId& InValue)
	{
		return InValue.IsValid();
	}

	static void ResetPtr(sketch::Private::FLogItemId& InValue)
	{
		InValue = sketch::Private::FLogItemId();
	}

	static sketch::Private::FLogItemId MakeNullPtr()
	{
		return sketch::Private::FLogItemId();
	}

	static sketch::Private::FLogItemId NullableItemTypeConvertToItemType(const sketch::Private::FLogItemId& InValue)
	{
		return InValue;
	}

	static FString DebugDump(sketch::Private::FLogItemId InValue)
	{
		return {};
	}

	class SerializerType {};
};

void SSketchLog::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(Tree, STreeView<sketch::Private::FLogItemId>)
		.TreeItemsSource(&LogRoot)
		.OnGetChildren(this, &SSketchLog::OnGetChildren)
		.OnGenerateRow(this, &SSketchLog::OnGenerateRow)
	];

	if (InArgs._Log)
	{
		Rebuild(*InArgs._Log);
	}
}

void SSketchLog::Rebuild(const sketch::HeaderTool::FLog& InLog)
{
	Log = &InLog;
	LogRoot.Reset(Log->GetFiles().Num());

	// Add all general messages first
	// Look for shortcuts by the way
	// A shortcut is when some level of log has only one item, so it can be presented as a single item
	struct FShortcut
	{
		int NumMessages = 0;
		int LastMessageId = 0;
	};
	TArray<FShortcut> NumFileChildren;
	// TArray<FShortcut> NumClassChildren;
	// TArray<FShortcut> NumSlotChildren;
	NumFileChildren.Reserve(Log->GetFiles().Num());
	// NumClassChildren.Reserve(Log->GetFiles().Num());
	// NumSlotChildren.Reserve(Log->GetFiles().Num());
	NumFileChildren.SetNumZeroed(Log->GetFiles().Num());
	// NumClassChildren.SetNumZeroed(Log->GetFiles().Num());
	// NumSlotChildren.SetNumZeroed(Log->GetFiles().Num());
	for (auto Message = Log->GetMessages().CreateConstIterator(); Message; ++Message)
	{
		if (Message->FileIndex == INDEX_NONE)
		{
			LogRoot.Add({ .MessageId = Message.GetIndex() });
		}
		else
		{
			++NumFileChildren[Message->FileIndex].NumMessages;
			NumFileChildren[Message->FileIndex].LastMessageId = Message.GetIndex();
			// if (Message->ClassIndex != INDEX_NONE)
			// {
			// 	++NumClassChildren[Message->ClassIndex].NumMessages;
			// 	NumClassChildren[Message->ClassIndex].LastMessageId = Message.GetIndex();
			// }
			// if (Message->SlotIndex != INDEX_NONE)
			// {
			// 	++NumSlotChildren[Message->SlotIndex].NumMessages;
			// 	NumSlotChildren[Message->SlotIndex].LastMessageId = Message.GetIndex();
			// }
		}
	}

	// Add all files second
	for (int i = 0; i < Log->GetFiles().Num(); ++i)
	{
		const FShortcut& Shortcut = NumFileChildren[i];
		if (Shortcut.NumMessages == 1)
		{
			LogRoot.Add({
				.FileId = Log->GetMessages()[Shortcut.LastMessageId].FileIndex,
				.ClassId = Log->GetMessages()[Shortcut.LastMessageId].ClassIndex,
				.SlotId = Log->GetMessages()[Shortcut.LastMessageId].SlotIndex,
				.MessageId = Shortcut.LastMessageId,
				.bShortcut = true
			});
		}
		else
		{
			LogRoot.Add({ .FileId = i });
		}
	}

	Tree->RequestTreeRefresh();
}

void SSketchLog::Reset()
{
	LogRoot.Empty();
	Tree->RequestTreeRefresh();
}

void SSketchLog::OnGetChildren(sketch::Private::FLogItemId ItemId, TArray<sketch::Private::FLogItemId>& Children)
{
	// Message entries has no children
	if (ItemId.MessageId != INDEX_NONE)
	{
		return;
	}

	// List all file-general messages and classes
	if (ItemId.ClassId == INDEX_NONE)
	{
		for (auto Message = Log->GetMessages().CreateConstIterator(); Message; ++Message)
		{
			if (Message->FileIndex == ItemId.FileId)
			{
				Children.Add({
					.FileId = Message->FileIndex,
					.ClassId = Message->ClassIndex,
					.MessageId = Message->ClassIndex == INDEX_NONE ? Message.GetIndex() : INDEX_NONE
				});
			}
		}
		return;
	}

	// List all class-general messages and slots
	if (ItemId.SlotId == INDEX_NONE)
	{
		for (auto Message = Log->GetMessages().CreateConstIterator(); Message; ++Message)
		{
			if (Message->FileIndex == ItemId.FileId && Message->ClassIndex == ItemId.ClassId)
			{
				Children.Add({
					.FileId = Message->FileIndex,
					.ClassId = Message->ClassIndex,
					.SlotId = Message->SlotIndex,
					.MessageId = Message->SlotIndex == INDEX_NONE ? Message.GetIndex() : INDEX_NONE
				});
			}
		}
		return;
	}

	// List all slot messages
	for (auto Message = Log->GetMessages().CreateConstIterator(); Message; ++Message)
	{
		if (Message->FileIndex == ItemId.FileId && Message->ClassIndex == ItemId.ClassId && Message->SlotIndex == ItemId.SlotId)
		{
			Children.Add({ .FileId = Message->FileIndex, .ClassId = Message->ClassIndex, .SlotId = Message->SlotIndex, .MessageId = Message.GetIndex() });
		}
	}
}

TSharedRef<SWidget> SSketchLog::MakeFileTitleWidget(sketch::Private::FLogItemId ItemId)
{
	FString FileName = Log->GetFiles()[ItemId.FileId];
	int SlashIndex = INDEX_NONE;
	if (FileName.FindLastChar(TCHAR('/'), SlashIndex))
	{
		FileName.RightChopInline(SlashIndex + 1);
	}
	TSharedRef<STextBlock> Text = SNew(STextBlock)
		.Text(FText::FromString(MoveTemp(FileName)))
		.ToolTipText(LOCTEXT("FileActions", "Copy full file path - double click\nBrowse file - ctrl + double-click"));
	Text->SetOnMouseDoubleClick(FPointerEventHandler::CreateSP(this, &SSketchLog::OnFileTitleDoubleClick, ItemId.FileId));
	return MoveTemp(Text);
}

TSharedRef<SWidget> SSketchLog::MakeClassTitleWidget(sketch::Private::FLogItemId ItemId)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Log->GetClasses()[ItemId.ClassId]));
}

TSharedRef<SWidget> SSketchLog::MakeSlotTitleWidget(sketch::Private::FLogItemId ItemId)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Log->GetSlots()[ItemId.SlotId]));
}

TSharedRef<SWidget> SSketchLog::MakeMessageVerbosityIcon(sketch::Private::FLogItemId ItemId)
{
	const sketch::HeaderTool::FLog::FMessage& Message = Log->GetMessages()[ItemId.MessageId];
	const FSlateBrush* Icon =
		Message.Verbosity ==
		ELogVerbosity::Error
			? FSlateIcon("EditorStyle", "Icons.ErrorWithColor").GetIcon()
			: Message.Verbosity == ELogVerbosity::Warning
			? FSlateIcon("CoreStyle", "Icons.WarningWithColor").GetIcon()
			: FSlateIcon(FAppStyle::GetAppStyleSetName(), "CurveEd.CurveKeySelected").GetIcon();
	return
			SNew(SBox)
			.MinDesiredWidth(16.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(Icon)
			];
}

TSharedRef<SWidget> SSketchLog::MakeMessageSourceButton(sketch::Private::FLogItemId ItemId)
{
	const sketch::HeaderTool::FLog::FMessage& Message = Log->GetMessages()[ItemId.MessageId];
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.OnClicked(this, &SSketchLog::GoToMessageSource, ItemId.MessageId)
		.ToolTipText(LOCTEXT("GoToSourceCode", "Go to source code in IDE"))
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::FromInt(Message.Source.Line) + TEXT("::") + FString::FromInt(Message.Source.Column)))
			.Font(sketch::Private::DefaultFont())
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> SSketchLog::MakeMessageWidget(sketch::Private::FLogItemId ItemId)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Log->GetMessages()[ItemId.MessageId].Message));
}

TSharedRef<ITableRow> SSketchLog::OnGenerateRow(
	sketch::Private::FLogItemId ItemId,
	const TSharedRef<STableViewBase>& Table
)
{
	// Shortcuts
	if (ItemId.bShortcut)
	{
		SHorizontalBox::FArguments HorizontalBox;
		HorizontalBox
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .VAlign(VAlign_Center)
			[
				MakeMessageVerbosityIcon(ItemId)
			]

			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .MinWidth(75.f)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Center)
			[
				MakeMessageSourceButton(ItemId)
			]

			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .MinWidth(160.0f)
			  .VAlign(VAlign_Center)
			[
				MakeFileTitleWidget(ItemId)
			]

			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(INVTEXT(" - "))
			];

		const sketch::HeaderTool::FLog::FMessage& Message = Log->GetMessages()[ItemId.MessageId];
		if (Message.ClassIndex != INDEX_NONE)
		{
			HorizontalBox
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .MinWidth(160.0f)
				  .VAlign(VAlign_Center)
				[
					MakeClassTitleWidget(ItemId)
				]

				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(INVTEXT(" - "))
				];
		}
		if (Message.SlotIndex != INDEX_NONE)
		{
			HorizontalBox
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				[
					MakeSlotTitleWidget(ItemId)
				]

				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(INVTEXT(" - "))
				];
		}
		HorizontalBox
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .VAlign(VAlign_Center)
			[
				MakeMessageWidget(ItemId)
			];
		return SNew(STableRow<sketch::Private::FLogItemId>, Table)
			[
				SArgumentNew(HorizontalBox, SHorizontalBox)
			];
	}

	// File headers
	if (ItemId.FileId != INDEX_NONE && ItemId.ClassId == INDEX_NONE && ItemId.SlotId == INDEX_NONE && ItemId.MessageId == INDEX_NONE)
	{
		return SNew(STableRow<sketch::Private::FLogItemId>, Table)
			[
				MakeFileTitleWidget(ItemId)
			];
	}

	// Class headers
	if (ItemId.FileId != INDEX_NONE && ItemId.ClassId != INDEX_NONE && ItemId.SlotId == INDEX_NONE && ItemId.MessageId == INDEX_NONE)
	{
		return SNew(STableRow<sketch::Private::FLogItemId>, Table)
			[
				MakeClassTitleWidget(ItemId)
			];
	}

	// Slot headers
	if (ItemId.FileId != INDEX_NONE && ItemId.ClassId != INDEX_NONE && ItemId.SlotId != INDEX_NONE && ItemId.MessageId == INDEX_NONE)
	{
		return SNew(STableRow<sketch::Private::FLogItemId>, Table)
			[
				MakeSlotTitleWidget(ItemId)
			];
	}

	// Messages
	if (ItemId.MessageId != INDEX_NONE) [[likely]]
	{
		return SNew(STableRow<sketch::Private::FLogItemId>, Table)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.MinWidth(16.f)
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					MakeMessageVerbosityIcon(ItemId)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					MakeMessageSourceButton(ItemId)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.VAlign(VAlign_Center)
				[
					MakeMessageWidget(ItemId)
				]
			];
	}

	// Only bad items get here
	// Clarify this fact
	return SNew(STableRow<sketch::Private::FLogItemId>, Table)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(SL"Invalid item: file - %i, class - %i, slot - %i, message - %i", ItemId.FileId, ItemId.ClassId, ItemId.SlotId, ItemId.MessageId)))
		];
}

FReply SSketchLog::OnFileTitleDoubleClick(const FGeometry&, const FPointerEvent& Event, int FileId) const
{
	if (Log->GetFiles().IsValidIndex(FileId)) [[likely]]
	{
		if (!Event.IsControlDown())
		{
			FPlatformApplicationMisc::ClipboardCopy(*Log->GetFiles()[FileId]);
		}
		else
		{
			FPlatformProcess::ExploreFolder(*Log->GetFiles()[FileId]);
		}
	}
	return FReply::Handled();
}

FReply SSketchLog::GoToMessageSource(int MessageId) const
{
	if (Log && Log->GetMessages().IsValidIndex(MessageId)) [[likely]]
	{
		const sketch::HeaderTool::FLog::FMessage& Message = Log->GetMessages()[MessageId];
		FSourceCodeNavigation::OpenSourceFile(
			Log->GetFiles()[Message.FileIndex],
			Message.Source.Line,
			Message.Source.Column
		);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
