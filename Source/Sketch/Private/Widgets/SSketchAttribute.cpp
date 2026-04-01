#include "Widgets/SSketchAttribute.h"

#include "PropertyCustomizationHelpers.h"
#include "Sketch.h"
#include "SketchTypes.h"
#include "SourceCodeNavigation.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HeaderTool/SourceCodeUtility.h"
#include "Widgets/SSketchHeaderRow.h"

#define LOCTEXT_NAMESPACE "SSketchAttribute"

void SSketchAttribute::Construct(
	const FArguments& InArgs,
	sketch::FAttribute& Attribute
)
{
	// Cache stuff
	WeakAttribute = Attribute.AsWeak();
	WeakHeader = InArgs._HeaderRow;

	// Prepare helpers
	TSharedPtr<SSketchHeaderRow> Header = WeakHeader.Pin();
	std::array<SSketchHeaderRow::FColumnProperties, 7> Columns = SSketchHeaderRow::GetColumnsProperties();
	auto MakeSlot = [&](int Index, EHorizontalAlignment HorizontalAlignment = HAlign_Fill)-> SHorizontalBox::FSlot::FSlotArguments
	{
		switch (Columns[Index].SizeMode)
		{
		default:
			{
				TAttribute<float> Width = Columns[Index].Size;
				if (Header && Header->GetColumns().IsValidIndex(Index) && Header->GetColumns()[Index].ColumnId == Columns[Index].Id)
				{
					Width = TAttribute<float>::CreateSPLambda(Header.Get(), [H = Header.Get(), Index, DefaultSize = Columns[Index].Size]() -> float
					{
						if (H->GetColumns().IsValidIndex(Index)) return H->GetColumns()[Index].Width.Get();
						return DefaultSize;
					});
				}
				return MoveTemp(
					SHorizontalBox::Slot()
					.HAlign(HorizontalAlignment)
					.VAlign(VAlign_Center)
					.FillWidth(MoveTemp(Width))
				);
			}
		case EColumnSizeMode::Fixed:
			return MoveTemp(
				SHorizontalBox::Slot()
				.HAlign(HorizontalAlignment)
				.VAlign(VAlign_Center)
				.AutoWidth() // This excludes slot from free space distribution algorithm
				.MinWidth(Columns[Index].Size)
				.MaxWidth(Columns[Index].Size)
			);
		}
	};
	auto GetSlotVisibility = [&](int Index, const TOptional<bool>& Controller) -> TAttribute<EVisibility>
	{
		if (!Header || !Header->GetColumns().IsValidIndex(Index))
			return Controller.Get(true) ? EVisibility::Visible : EVisibility::Collapsed;
		return !Header->GetColumns()[Index].bIsVisible ? EVisibility::Collapsed : Controller.Get(true) ? EVisibility::Visible : EVisibility::Hidden;
	};

	// Create reset button
	TSharedRef<SWidget> ResetButton = PropertyCustomizationHelpers::MakeResetButton(
		FSimpleDelegate::CreateSP(this, &SSketchAttribute::Reset)
	);
	ResetButton->SetVisibility(TAttribute<EVisibility>::CreateSP(this, &SSketchAttribute::GetResetButtonVisibility));

	// Make content
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FSlateIcon("CoreStyle", "WhiteBrush").GetIcon())
		.BorderBackgroundColor(this, &SSketchAttribute::GetBackgroundColor)
		.Padding(0, 3)
		[
			SNew(SHorizontalBox)

			+ MakeSlot(0, HAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked(this, &SSketchAttribute::OnBrowseSourceCode)
				.ToolTipText(LOCTEXT("ViewSourceCodeInIDE", "View source code in IDE"))
				.Visibility(GetSlotVisibility(0, InArgs._ShowLine))
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::FromInt(Attribute.GetLine()) + TEXT("::") + FString::FromInt(Attribute.GetColumn())))
					.Font(sketch::Private::DefaultFont())
					.Justification(ETextJustify::Center)
					.OverflowPolicy(ETextOverflowPolicy::Clip)
				]
			]

			+ MakeSlot(1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().MinSize(8)
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(STextBlock)
					.Text(FText::FromName(Attribute.GetName()))
					.Font(sketch::Private::DefaultFont())
					.Visibility(GetSlotVisibility(1, InArgs._ShowName))
				]

			]

			+ MakeSlot(2, HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(16)
				.HeightOverride(16)
				[
					SNew(SImage)
					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Profiler.EventGraph.ExpandHotPath16").GetIcon())
					.Visibility(GetSlotVisibility(2, InArgs._ShowInteractivity))
				]
			]

			+ MakeSlot(3)
			[
				SNew(STextBlock)
				.Text(this, &SSketchAttribute::GetNumUsers)
				.Font(sketch::Private::DefaultFont())
				.Justification(ETextJustify::Center)
				.OverflowPolicy(ETextOverflowPolicy::Clip)
				.Visibility(GetSlotVisibility(3, InArgs._ShowNumUsers))
			]

			+ MakeSlot(4, HAlign_Center)
			[
				SNew(SButton)
				.Visibility(GetSlotVisibility(4, InArgs._AllowCodePatching))
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.ToolTipText(LOCTEXT("PatchCode", "Patch code.\nHold \"ctrl\" to remove \"Sketch\" invocation as well - can only be done once."))
				.OnClicked(this, &SSketchAttribute::PatchCode)
				.HAlign(HAlign_Center)
				[
					SAssignNew(PatchCodeButtonIcon, SImage)
					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Import").GetIcon())
				]
			]

			+ MakeSlot(5, HAlign_Center)
			[
				SNew(SButton)
				.Visibility(GetSlotVisibility(5, true))
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.ToolTipText(LOCTEXT("CopyToClipboard", "Copy to clipboard"))
				.OnClicked(this, &SSketchAttribute::CopyCode)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Export").GetIcon())
				]
			]

			+ MakeSlot(6)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(EditorContainer, SBox)
					[
						Attribute.GetValue()->MakeEditor()
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					ResetButton
				]
			]
		]
	];
}

FSlateColor SSketchAttribute::GetBackgroundColor() const
{
	return IsHovered() ? FSlateColor(EStyleColor::Header) : FSlateColor(EStyleColor::Panel);
}

FText SSketchAttribute::GetNumUsers() const
{
	const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin();
	if (!Attribute) [[unlikely]] return LOCTEXT("Err", "Err");

	if (NumDisplayedUsers != Attribute->NumUsers)
	[[unlikely]]
	{
		NumDisplayedUsers = Attribute->NumUsers;
		FString Result;
		Result.Reserve(8);
		Result += TEXT("x");
		Result.AppendInt(Attribute->NumUsers);
		NumDisplayedUsersText = FText::FromString(Result);
	}
	return NumDisplayedUsersText;
}

bool SSketchAttribute::TryPatchCode(bool bRemoveSketchInvocation)
{
	// Sanitize
	const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin();
	if (!Attribute) [[unlikely]] return false;

	// Load file
	TArray<FString> File;
	const FString FilePath = Attribute->GetSourceLocation().FileName.ToString();
	if (!FFileHelper::LoadFileToStringArray(File, *FilePath)) [[unlikely]] return false;
	const int LineIndex = Attribute->GetLine();
	if (!File.IsValidIndex(LineIndex)) [[unlikely]] return false;
	FString& Line = File[LineIndex - 1];

	// Find attribute in given line. Do not rely on column info as at can possibly be outdated.
	{
		using namespace sketch::SourceCode;
		enum { InvocationTag, ArgsTag };
		TMatcher InvocationMatcher(
			Line,
			&NoFilter,
			Matcher::String<InvocationTag>(TEXT("Sketch")),
			Matcher::Arguments<ArgsTag>()
		);
		const FString AttributeName = TEXT("\"") + Attribute->GetName().ToString() + TEXT("\"");
		for (; InvocationMatcher; ++InvocationMatcher)
		{
			sketch::FStringView Args = InvocationMatcher.View<ArgsTag>();
			TMatcher CommaMatcher(Args, CombinedFilter(&Bracket::AnySubscopeFilter, &String::LiteralFilter), Matcher::String(TEXT(",")));
			if (CommaMatcher) [[likely]]
			{
				sketch::FStringView NameArg = Args.Mid(1, CommaMatcher.Position - 1);
				if (NameArg.Contains(AttributeName)) [[likely]]
				{
					const sketch::FStringView LineView = Line;
					FString NewLine;
					NewLine.Reserve(Line.Len());
					if (!bRemoveSketchInvocation)
					{
						NewLine += LineView.Left(InvocationMatcher.Match.Get<ArgsTag>().Value.FirstOf + CommaMatcher.Position);
						NewLine += TEXT(" ");
						NewLine += Attribute->GetValue()->GenerateCode();
						NewLine += LineView.RightChop(InvocationMatcher.Match.Get<ArgsTag>().Value.FirstAfter - 1);
					}
					else
					{
						NewLine += LineView.Left(InvocationMatcher.Match.Get<InvocationTag>().Value.FirstOf);
						NewLine += Attribute->GetValue()->GenerateCode();

						// Watch out for cases when line is over right after Sketch invocation
						if (LineView.IsValidIndex(InvocationMatcher.Match.Get<ArgsTag>().Value.FirstAfter))[[likely]]
						{
							NewLine += LineView.RightChop(InvocationMatcher.Match.Get<ArgsTag>().Value.FirstAfter);
						}
					}
					Line = MoveTemp(NewLine);
					UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);
					FFileHelper::SaveStringArrayToFile(File, *FilePath);
					break;
				}
			}
		}
	}
	return true;
}

FReply SSketchAttribute::PatchCode()
{
	const bool bRemoveSketchInvocation = FSlateApplication::Get().GetModifierKeys().IsControlDown();
	const bool bSuccess = TryPatchCode(bRemoveSketchInvocation);
	PatchCodeButtonAnimator.Animate(*this, *PatchCodeButtonIcon.Get(), bSuccess ? FLinearColor::Green : FLinearColor::Red);
	return FReply::Handled();
}

FReply SSketchAttribute::CopyCode()
{
	const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin();
	if (!Attribute) [[unlikely]] return FReply::Handled();

	const FString Code = Attribute->GetValue()->GenerateCode();
	FPlatformApplicationMisc::ClipboardCopy(*Code);
	return FReply::Handled();
}

void SSketchAttribute::Reset()
{
	const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin();
	if (!Attribute) [[unlikely]] return;

	Attribute->GetValue()->Reinitialize(*Attribute->GetDefaultValue());
	EditorContainer->SetContent(Attribute->GetValue()->MakeEditor());
	Attribute->GetValue()->OnValueChanged.Broadcast();
}

EVisibility SSketchAttribute::GetResetButtonVisibility() const
{
	const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin();
	if (!Attribute) [[unlikely]] return EVisibility::Hidden;

	const bool bEqual = Attribute->GetDefaultValue()->Equals(*Attribute->GetValue().Get());
	return bEqual ? EVisibility::Hidden : EVisibility::Visible;
}

FReply SSketchAttribute::OnBrowseSourceCode() const
{
	if (const TSharedPtr<sketch::FAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
	{
		FSourceCodeNavigation::OpenSourceFile(
			Attribute->GetSourceLocation().FileName.ToString(),
			Attribute->GetLine(),
			Attribute->GetColumn()
		);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
