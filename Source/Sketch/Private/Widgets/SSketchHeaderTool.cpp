#include "Widgets/SSketchHeaderTool.h"

#include "SketchHeaderTool.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SlateEditableTextLayout.h"

#define LOCTEXT_NAMESPACE "SSketchHeaderTool"

void SSketchHeaderTool::Construct(const FArguments& InArgs)
{
	GeneratedCode = SNew(SMultiLineEditableTextBox)
		.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 8))
		.IsReadOnly(true);
	SearchBox = SNew(SSearchBox).OnSearch(this, &SSketchHeaderTool::OnSearch);
	GeneratedCode->SetSearchText(TAttribute<FText>::CreateSP(SearchBox.Get(), &SSearchBox::GetText));

	ChildSlot
		.Padding(InArgs._ContentPadding)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SearchBox.ToSharedRef()
			]

			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				SAssignNew(ScrollBox, SScrollBox)
				+ SScrollBox::Slot()
				.FillSize(1)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8., 4., 8., 0)
					[
						SAssignNew(Lines, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 8))
						.Justification(ETextJustify::Center)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						GeneratedCode.ToSharedRef()
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(PathEdit, SEditableTextBox)
						.HintText(LOCTEXT("PathHint", "Path to scan"))
					]

					+ SVerticalBox::Slot()
					[
						SAssignNew(InclusionRootEdit, SEditableTextBox)
						.HintText(LOCTEXT("InclusionRootHint", "Part of the path to omit in '#include'-s"))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SSketchHeaderTool::ListCommonPaths)
					// .MenuPlacement(MenuPlacement_MenuLeft)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("Run", "Run"))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(this, &SSketchHeaderTool::GenerateCode)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("ButtonText"))
							.Text(LOCTEXT("Run", "Run"))
							.Visibility(this, &SSketchHeaderTool::GetButtonTextVisibility)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SThrobber)
							.Animate(SThrobber::VerticalAndOpacity)
							.Visibility(this, &SSketchHeaderTool::GetButtonThrobberVisibility)
						]
					]
				]
			]
		];
}

FReply SSketchHeaderTool::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsControlDown())
	{
		if (InKeyEvent.GetKey() == EKeys::Home)
		{
			ScrollBox->SetScrollOffset(0.f);
			return FReply::Handled();
		}
		if (InKeyEvent.GetKey() == EKeys::End)
		{
			ScrollBox->ScrollToEnd();
			return FReply::Handled();
		}
		constexpr float LineSize = 13.13f;
		const bool bScrollDown = InKeyEvent.GetKey() == EKeys::Down;
		const bool bScrollUp = InKeyEvent.GetKey() == EKeys::Up;
		if (bScrollDown || bScrollUp)
		{
			const float ScrollSize = ScrollBox->GetScrollOffsetOfEnd();
			// const float VisibleFraction = ScrollBox->GetViewFraction();
			// const float VisibleSize = ScrollSize * VisibleFraction;
			const float CurrentScroll = ScrollBox->GetScrollOffset();
			const float NewScroll = FMath::Clamp(CurrentScroll + (bScrollDown ? LineSize : -LineSize), 0, ScrollSize);
			ScrollBox->SetScrollOffset(NewScroll);
			return FReply::Handled();
		}
	}
	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

FSlateEditableTextLayout* SSketchHeaderTool::GetTextLayout()
{
	// We have no legal means to access detailed text state
	// Here are illegal ones
	class SMultiLineEditableTextBoxContents : public SMultiLineEditableTextBox
	{
	public:
		using SMultiLineEditableTextBox::EditableText;
	};
	class SMultiLineEditableTextContents : public SMultiLineEditableText
	{
	public:
		using SMultiLineEditableText::EditableTextLayout;
	};

	// Make sure it succeeded
	FSlateEditableTextLayout* Layout =
		static_cast<SMultiLineEditableTextContents*>(
			static_cast<SMultiLineEditableTextBoxContents*>(GeneratedCode.Get())->EditableText.Get()
		)
		->EditableTextLayout.Get();
	return Layout;
}

void SSketchHeaderTool::OnSearch(SSearchBox::SearchDirection Direction)
{
	// Advance search
	GeneratedCode->AdvanceSearch(Direction == SSearchBox::Previous);

	// Make sure it succeeded
	FSlateEditableTextLayout* Layout = GetTextLayout();
	const FTextSelection Selection = Layout->GetSelection();
	if (!Selection.LocationA.IsValid())
		return;

	// Synthesize and apply new scroll offset
	const float TotalLines = Layout->GetTextLineCount();
	const float VisibleFraction = ScrollBox->GetViewFraction();
	const float BasicDesiredOffset = float(Selection.LocationA.GetLineIndex()) / TotalLines;
	const float RawCompensatedOffset = BasicDesiredOffset - VisibleFraction * 0.5f + VisibleFraction * BasicDesiredOffset;
	const float CompensatedOffset = FMath::Clamp(RawCompensatedOffset, 0.f, 1.f);
	const float NewOffset = CompensatedOffset * ScrollBox->GetScrollOffsetOfEnd();
	ScrollBox->SetScrollOffset(NewOffset);
}

TSharedRef<SWidget> SSketchHeaderTool::ListCommonPaths()
{
	FMenuBuilder Menu(true, nullptr);
	Menu.BeginSection(NAME_None, LOCTEXT("Engine", "Engine"));
	Menu.AddMenuEntry(
		INVTEXT("SlateCore"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SSketchHeaderTool::SelectCommonPath, FStringView(TEXT("Runtime/SlateCore/Public"))))
	);
	Menu.AddMenuEntry(
		INVTEXT("Slate"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SSketchHeaderTool::SelectCommonPath, FStringView(TEXT("Runtime/Slate/Public"))))
	);
	Menu.AddMenuEntry(
		INVTEXT("AdvancedWidgets"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SSketchHeaderTool::SelectCommonPath, FStringView(TEXT("Runtime/AdvancedWidgets/Public"))))
	);
	Menu.EndSection();
	Menu.BeginSection(NAME_None, LOCTEXT("Project", "Project"));
	Menu.AddMenuEntry(
		LOCTEXT("Source", "Source"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SSketchHeaderTool::SelectProjectPath, FStringView(TEXT("Source"))))
	);
	Menu.AddMenuEntry(
		LOCTEXT("Plugins", "Plugins"),
		FText(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SSketchHeaderTool::SelectProjectPath, FStringView(TEXT("Plugins"))))
	);
	Menu.EndSection();
	return Menu.MakeWidget();
}

void SSketchHeaderTool::SelectCommonPath(const FStringView Path)
{
	// Locate engine
	// Do not use FPaths since it returns ".."-based nonsense
	FString EngineDir = FPlatformProcess::BaseDir();
	constexpr FStringView Engine(TEXT("/Engine/"));
	const int EngineIndex = EngineDir.Find(Engine, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (EngineIndex == INDEX_NONE) return;
	const int Point = EngineIndex + Engine.Len();
	EngineDir.RemoveAt(Point, EngineDir.Len() - Point, EAllowShrinking::No);
	EngineDir.Append(FStringView(TEXT("Source/")));
	EngineDir.Append(Path);

	InclusionRootEdit->SetText(FText::FromString(EngineDir));
	PathEdit->SetText(FText::FromString(MoveTemp(EngineDir)));
}

void SSketchHeaderTool::SelectProjectPath(const FStringView Path)
{
	FString ProjectDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::ProjectDir());
	ProjectDir.Append(Path);
	InclusionRootEdit->SetText(FText::FromString(ProjectDir));
	PathEdit->SetText(FText::FromString(MoveTemp(ProjectDir)));
}

FReply SSketchHeaderTool::GenerateCode()
{
	if (bWorking)
		return FReply::Handled();

	bWorking = true;
	GeneratedCode->SetText(FText());
	Lines->SetText(FText());
	auto Job = [
			WeakThis = SharedThis(this).ToWeakPtr(),
			PathValue = PathEdit->GetText().ToString(),
			InclusionRootValue = InclusionRootEdit->GetText().ToString()
		]() mutable
	{
		using namespace sketch::HeaderTool;

		TSharedPtr<SSketchHeaderTool> This = WeakThis.Pin();
		if (!This) [[unlikely]]
			return;

		// Parse files
		TArray<FFile> Files;
		{
			const FString Extension = FPaths::GetExtension(PathValue);
			if (Extension.IsEmpty())
			{
				FPaths::NormalizeDirectoryName(PathValue);
				Files = FHeaderTool::Scan(PathValue, true);
			}
			else
			{
				FPaths::NormalizeFilename(PathValue);
				Files.Emplace(FHeaderTool::Scan(PathValue));
			}
		}

		// Generate code
		FString Prologue = FHeaderTool::GenerateReflectionPrologue();
		FString Code;
		FString Epilogue = FHeaderTool::GenerateReflectionEpilogue(InclusionRootValue);
		for (const FFile& File : Files)
		{
			for (const FClass& Class : File.Classes)
			{
				FHeaderTool::GenerateReflection(File.Path, InclusionRootValue, Class, Prologue, Code, Epilogue);
			}
		}
		FString Reflection = FHeaderTool::CombineReflection(Prologue, Code, Epilogue);

		// Generate line index
		int NumLines = 0;
		for (int i = 0; i < Reflection.Len(); ++i)
		{
			NumLines += Reflection[i] == TCHAR('\n');
		}
		FString LineIndices;
		LineIndices.Reserve(NumLines * 4);
		for (int i = 1; i < NumLines + 2; ++i)
		{
			LineIndices.AppendInt(i);
			LineIndices.Append(TEXT("\n"));
		}
		LineIndices.RemoveAt(LineIndices.Len() - 1);

		// Apply everything back in game thread
		auto Callback = [WeakThis, Reflection = FText::FromString(MoveTemp(Reflection)), Indices = FText::FromString(MoveTemp(LineIndices))]() mutable
		{
			TSharedPtr<SSketchHeaderTool> This = WeakThis.Pin();
			if (!This) [[unlikely]]
				return;

			This->Lines->SetText(MoveTemp(Indices));
			This->GeneratedCode->SetText(MoveTemp(Reflection));
			This->bWorking = false;
		};
		AsyncTask(ENamedThreads::Type::GameThread, MoveTemp(Callback));
	};
	AsyncTask(ENamedThreads::Type::AnyNormalThreadNormalTask, MoveTemp(Job));
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
