#include "Widgets/SSketchAttribute.h"

#include "PropertyCustomizationHelpers.h"
#include "Sketch.h"
#include "SketchTypes.h"
#include "SourceCodeNavigation.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "SSketchAttribute"

#define Require(Condition) { if(!(Condition)) [[unlikely]] return; }

void PatchCode(const sketch::FAttributeHandle& Handle)
{
	const auto& Host = FSketchModule::Get();
	const sketch::FAttribute* Attribute = Handle.Get();
	Require(Attribute);

	TArray<FString> File;
	Require(FFileHelper::LoadFileToStringArray(File, *Attribute->GetSourceLocation().FileName.ToString()));
	const int LineIndex = Attribute->GetLine();
	Require(File.IsValidIndex(LineIndex));
	FString Line = File[LineIndex - 1];
	const FString AttributeName = TEXT("\"") + Attribute->GetName().ToString() + TEXT("\"");
	int NameStart = Line.Find(AttributeName, ESearchCase::CaseSensitive);
	bool bAnsi = false;
	if (NameStart == INDEX_NONE)
	{
		Line = FString((ANSICHAR*)*Line);
		NameStart = Line.Find(AttributeName, ESearchCase::CaseSensitive);
		bAnsi = true;
	}
	Require(NameStart != INDEX_NONE);
	const int SketchStart = Line.Find(TEXT("Sketch"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, NameStart);
	Require(SketchStart != INDEX_NONE);
	const int CommaAfterNameIndex = Line.Find(TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
	Require(CommaAfterNameIndex != INDEX_NONE);
	const int ParenthesisAfterArgsIndex = Line.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CommaAfterNameIndex);
	Require(ParenthesisAfterArgsIndex != INDEX_NONE);
	const FString GeneratedCode = Attribute->Apply([]<typename T>(const T& Value) { return sketch::TWrappedAttributeTraits<T>::GenerateCode(Value); });
	Line = Line.Left(CommaAfterNameIndex + 1) + TEXT(" ") + GeneratedCode + Line.RightChop(ParenthesisAfterArgsIndex);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);
	if (bAnsi)
	{
		FAnsiString AnsiLine = FAnsiString(*Line);
		AnsiLine.AppendChar(ANSICHAR('0'));
		AnsiLine[Line.Len() - 1] = ANSICHAR('\0');
		Line = FString((TCHAR*)*AnsiLine);
		AnsiLine[Line.Len() - 1] = ANSICHAR('0');
	}
	File[LineIndex - 1] = Line;
	FFileHelper::SaveStringArrayToFile(File, *Attribute->GetSourceLocation().FileName.ToString());
}
#undef Require

void SSketchAttribute::Construct(
	const FArguments& InArgs,
	const sketch::FAttributeHandle& AttributeHandle
)
{
	// Cache stuff
	Handle = AttributeHandle;

	// Create reset button
	sketch::FAttribute* Attribute = Handle.Get();
	TSharedRef<SWidget> ResetButton = PropertyCustomizationHelpers::MakeResetButton(
		FSimpleDelegate::CreateSP(this, &SSketchAttribute::Reset)
	);
	ResetButton->SetVisibility(TAttribute<EVisibility>::CreateSP(this, &SSketchAttribute::GetResetButtonVisibility));

	// Make content
	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility(InArgs._ShowLine ? EVisibility::Visible : EVisibility::Collapsed)
			.WidthOverride(60)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::FromInt(Attribute->GetLine()) + TEXT("::") + FString::FromInt(Attribute->GetColumn())))
				.Font(sketch::Private::DefaultFont())
				.Justification(ETextJustify::Center)
				.OverflowPolicy(ETextOverflowPolicy::Clip)
				.OnDoubleClicked(this, &SSketchAttribute::OnBrowseSourceCode)
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility(InArgs._ShowName ? EVisibility::Visible : EVisibility::Collapsed)
			.WidthOverride(150)
			[
				SNew(STextBlock)
				.Text(FText::FromName(Attribute->GetName()))
				.Font(sketch::Private::DefaultFont())
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Visibility(InArgs._ShowInteractivity ? Attribute->IsDynamic() ? EVisibility::Visible : EVisibility::Hidden : EVisibility::Collapsed)
			.DesiredSizeOverride(FVector2D(16, 16))
			.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Profiler.EventGraph.ExpandHotPath16").GetIcon())
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility(InArgs._ShowNumUsers ? EVisibility::Visible : EVisibility::Collapsed)
			.WidthOverride(30)
			[
				SNew(STextBlock)
				.Text(this, &SSketchAttribute::GetNumUsers)
				.Font(sketch::Private::DefaultFont())
				.Justification(ETextJustify::Center)
				.OverflowPolicy(ETextOverflowPolicy::Clip)
				.Visibility(Attribute->IsDynamic() ? EVisibility::Visible : EVisibility::Hidden)
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Visibility(InArgs._AllowCodePatching ? EVisibility::Visible : EVisibility::Collapsed)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.ToolTipText(LOCTEXT("PatchCode", "Patch code"))
			.OnClicked(this, &SSketchAttribute::PatchCode)
			[
				SNew(SImage)
				.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Import").GetIcon())
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.ToolTipText(LOCTEXT("CopyToClipboard", "Copy to clipboard"))
			.OnClicked(this, &SSketchAttribute::CopyCode)
			[
				SNew(SImage)
				.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Export").GetIcon())
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(EditorContainer, SBox)
			[
				Attribute <<= [&]<class T>(const T&) { return sketch::TWrappedAttributeTraits<T>::MakeEditor(Handle); }
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			ResetButton
		]
	];
}

FText SSketchAttribute::GetNumUsers() const
{
	const sketch::FAttribute* Attribute = Handle.Get();
	if (!Attribute)
		[[unlikely]]
			return LOCTEXT("Err", "Err");

	if (NumDisplayedUsers != Attribute->GetNumUsers())
	[[unlikely]]
	{
		NumDisplayedUsers = Attribute->GetNumUsers();
		FString Result;
		Result.Reserve(8);
		Result += TEXT("x");
		Result.AppendInt(Attribute->GetNumUsers());
		NumDisplayedUsersText = FText::FromString(Result);
	}
	return NumDisplayedUsersText;
}

FReply SSketchAttribute::PatchCode()
{
	::PatchCode(Handle);
	return FReply::Handled();
}

FReply SSketchAttribute::CopyCode()
{
	const sketch::FAttribute* Attribute = Handle.Get();
	if (!Attribute)
		[[unlikely]]
			return FReply::Handled();

	(*Attribute) <= []<typename T>(const T& Value)
	{
		FString Code = sketch::TWrappedAttributeTraits<T>::GenerateCode(Value);
		FPlatformApplicationMisc::ClipboardCopy(*Code);
	};
	return FReply::Handled();
}

void SSketchAttribute::Reset()
{
	sketch::FAttribute* Attribute = Handle.Get();
	if (!Attribute)
		[[unlikely]]
			return;

	(*Attribute) <<= [&]<class T>(const T&)
	{
		Attribute->GetValueChecked<T>() = Attribute->GetDefaultValueChecked<T>();
	};
	EditorContainer->SetContent(
		Attribute <<= [&]<class T>(T&) { return sketch::TWrappedAttributeTraits<T>::MakeEditor(Handle); }
	);
}

EVisibility SSketchAttribute::GetResetButtonVisibility() const
{
	const sketch::FAttribute* Attribute = Handle.Get();
	if (!Attribute)
		[[unlikely]]
			return EVisibility::Hidden;

	bool bEqual = (*Attribute) <<= [&]<class T>(const T&)
	{
		if constexpr (requires { std::declval<T>() == std::declval<T>(); })
		{
			return Attribute->GetValueChecked<T>() == Attribute->GetDefaultValueChecked<T>();
		}
		else
		{
			return FMemory::Memcmp(
				&Attribute->GetValueChecked<T>(),
				&Attribute->GetDefaultValueChecked<T>(),
				sizeof(T)
			) == 0;
		}
	};
	return bEqual ? EVisibility::Hidden : EVisibility::Visible;
}

FReply SSketchAttribute::OnBrowseSourceCode(const FGeometry& Geometry, const FPointerEvent& PointerEvent) const
{
	if (const sketch::FAttribute* Attribute = Handle.Get())
	[[likely]]
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
