// #include "Widgets/SSketchControllerOLD.h"
//
// #include "Sketch.h"
// #include "HAL/PlatformApplicationMisc.h"
// #ifdef PROPERTYEDITOR_API
// #include "PropertyCustomizationHelpers.h"
// #endif
// #ifdef UNREALED_API
// #include "SourceCodeNavigation.h"
// #endif
//
// using namespace sketch;
//
// #define LOCTEXT_NAMESPACE "SSketchController"
//
// template <>
// struct TIsValidListItem<FAttributePath>
// {
// 	enum
// 	{
// 		Value = true
// 	};
// };
//
// template <>
// struct TListTypeTraits<FAttributePath>
// {
// public:
// 	typedef FAttributePath NullableType;
//
// 	using MapKeyFuncs = TDefaultMapHashableKeyFuncs<FAttributePath, TSharedRef<ITableRow>, false>;
// 	using MapKeyFuncsSparse = TDefaultMapHashableKeyFuncs<FAttributePath, FSparseItemInfo, false>;
// 	using SetKeyFuncs = DefaultKeyFuncs<FAttributePath>;
//
// 	template <typename U>
// 	static void AddReferencedObjects(
// 		FReferenceCollector& Collector,
// 		TArray<FAttributePath>& ItemsWithGeneratedWidgets,
// 		TSet<FAttributePath>& SelectedItems,
// 		TMap<const U*, FAttributePath>& WidgetToItemMap
// 	)
// 	{}
//
// 	static bool IsPtrValid(const FAttributePath& InValue)
// 	{
// 		return !InValue.Source.FileName.IsNone();
// 	}
//
// 	static void ResetPtr(FAttributePath& InValue)
// 	{
// 		InValue.Source.FileName = NAME_None;
// 		InValue.Source.FunctionName = NAME_None;
// 	}
//
// 	static FAttributePath MakeNullPtr()
// 	{
// 		return { NoInit };
// 	}
//
// 	static FAttributePath NullableItemTypeConvertToItemType(const FAttributePath& InValue)
// 	{
// 		return InValue;
// 	}
//
// 	static FString DebugDump(FAttributePath InValue)
// 	{
// 		FString Result;
// 		InValue.Source.FileName.AppendString(Result);
// 		Result += TEXT("::");
// 		InValue.Source.FunctionName.AppendString(Result);
// 		Result += TEXT("::");
// 		Result.AppendInt(InValue.Index);
// 		return Result;
// 	}
//
// 	class SerializerType
// 	{};
// };
//
//
// #define Require(Condition) { if(!(Condition)) [[unlikely]] return; }
// void PatchCode(const FAttributePath& Path)
// {
// 	const auto& Host = FSketchModule::Get();
// 	const FAttribute* Attribute = Host[Path];
// 	Require(Attribute);
//
// 	TArray<FString> File;
// 	Require(FFileHelper::LoadFileToStringArray(File, *Path.Source.FileName.ToString()));
// 	const int LineIndex = Attribute->GetLine();
// 	Require(File.IsValidIndex(LineIndex));
// 	FString Line = File[LineIndex - 1];
// 	const FString AttributeName = TEXT("\"") + Attribute->GetName().ToString() + TEXT("\"");
// 	int NameStart = Line.Find(AttributeName, ESearchCase::CaseSensitive);
// 	bool bAnsi = false;
// 	if (NameStart == INDEX_NONE)
// 	{
// 		Line = FString((ANSICHAR*)*Line);
// 		NameStart = Line.Find(AttributeName, ESearchCase::CaseSensitive);
// 		bAnsi = true;
// 	}
// 	Require(NameStart != INDEX_NONE);
// 	const int SketchStart = Line.Find(TEXT("Sketch"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, NameStart);
// 	Require(SketchStart != INDEX_NONE);
// 	const int CommaAfterNameIndex = Line.Find(TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
// 	Require(CommaAfterNameIndex != INDEX_NONE);
// 	const int ParenthesisAfterArgsIndex = Line.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CommaAfterNameIndex);
// 	Require(ParenthesisAfterArgsIndex != INDEX_NONE);
// 	const FString GeneratedCode = Attribute->Apply([]<typename T>(const T& Value) { return TAttributeTraits<T>::GenerateCode(Value); });
// 	Line = Line.Left(CommaAfterNameIndex + 1) + TEXT(" ") + GeneratedCode + Line.RightChop(ParenthesisAfterArgsIndex);
// 	UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);
// 	if (bAnsi)
// 	{
// 		FAnsiString AnsiLine = FAnsiString(*Line);
// 		AnsiLine.AppendChar(ANSICHAR('0'));
// 		AnsiLine[Line.Len() - 1] = ANSICHAR('\0');
// 		Line = FString((TCHAR*)*AnsiLine);
// 		AnsiLine[Line.Len() - 1] = ANSICHAR('0');
// 	}
// 	File[LineIndex - 1] = Line;
// 	FFileHelper::SaveStringArrayToFile(File, *Path.Source.FileName.ToString());
// }
// #undef Require
//
// void SSketchControllerOLD::Construct(const FArguments& InArgs)
// {
// 	Host = &FSketchModule::Get();
// 	CaptureState();
// 	Host->OnAttributesChanged.AddSP(this, &SSketchControllerOLD::OnAttributesChanged);
//
// 	ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot().AutoHeight()
// 		[
// 			SNew(SHorizontalBox)
// 			+ SHorizontalBox::Slot().AutoWidth()
// 			[
// 				SNew(SButton)
// 				.Text(LOCTEXT("Clear", "Clear"))
// 				.OnClicked_Static(
// 					[]
// 					{
// 						FSketchModule::Get().ClearStaleAttributes();
// 						return FReply::Handled();
// 					}
// 				)
// 			]
// 		]
//
// 		+ SVerticalBox::Slot().AutoHeight()
// 		[
// 			SNew(SBox)
// 			.Padding(Sketch("Padding", 0., 0.))
// 			[
// 				SAssignNew(Tree, STreeView<FAttributePath>)
// 				.TreeItemsSource(&CapturedState)
// 				.OnGenerateRow(this, &SSketchControllerOLD::OnGenerateRow)
// 				.OnGetChildren(this, &SSketchControllerOLD::OnGetChildren)
// 			]
// 		]
// 	];
// }
//
// class SAttribute : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SAttribute) {}
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs, const FAttributePath& InItem)
// 	{
// // 		// Cache stuff
// // 		Item = InItem;
// // 		auto& Host = FSketchModule::Get();
// // 		FAttribute* Attribute = Host[Item];
// //
// // 		// Create reset button
// // 		TSharedRef<SWidget> ResetButton =
// // #ifdef PROPERTYEDITOR_API
// // 				PropertyCustomizationHelpers::MakeResetButton(
// // 					FSimpleDelegate::CreateSP(this, &SAttribute::Reset)
// // 				)
// // #else
// // 			SNew(SButton).Text(INVTEXT("Disabled"))
// // #endif
// // 			;
// // 		ResetButton->SetVisibility(TAttribute<EVisibility>::CreateSP(this, &SAttribute::GetResetButtonVisibility));
// //
// // 		// Make content
// // 		ChildSlot
// // 		[
// // 			SNew(SHorizontalBox)
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.HAlign(HAlign_Center)
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SBox)
// // 				.WidthOverride(60)
// // 				[
// // 					SNew(STextBlock)
// // 					.Text(FText::FromString(FString::FromInt(Attribute->GetLine()) + TEXT("::") + FString::FromInt(Attribute->GetColumn())))
// // 					.Font(Private::DefaultFont())
// // 					.Justification(ETextJustify::Center)
// // 					.OverflowPolicy(ETextOverflowPolicy::Clip)
// // 					.OnDoubleClicked(this, &SAttribute::OnBrowseSourceCode)
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SBox)
// // 				.WidthOverride(150)
// // 				[
// // 					SNew(STextBlock)
// // 					.Text(FText::FromName(Attribute->GetName()))
// // 					.Font(Private::DefaultFont())
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SImage)
// // 				.DesiredSizeOverride(FVector2D(16, 16))
// // 				.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Profiler.EventGraph.ExpandHotPath16").GetIcon())
// // 				.Visibility(Attribute->IsDynamic() ? EVisibility::Visible : EVisibility::Hidden)
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SBox)
// // 				.WidthOverride(30)
// // 				[
// // 					SNew(STextBlock)
// // 					.Text(this, &SAttribute::GetNumUsers)
// // 					.Font(Private::DefaultFont())
// // 					.Justification(ETextJustify::Center)
// // 					.OverflowPolicy(ETextOverflowPolicy::Clip)
// // 					.Visibility(Attribute->IsDynamic() ? EVisibility::Visible : EVisibility::Hidden)
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SButton)
// // 				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
// // 				.ToolTipText(LOCTEXT("PatchCode", "Patch code"))
// // 				.OnClicked(this, &SAttribute::PatchCode)
// // 				[
// // 					SNew(SImage)
// // 					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Import").GetIcon())
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SNew(SButton)
// // 				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
// // 				.ToolTipText(LOCTEXT("CopyToClipboard", "Copy to clipboard"))
// // 				.OnClicked(this, &SAttribute::CopyCode)
// // 				[
// // 					SNew(SImage)
// // 					.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Export").GetIcon())
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				SAssignNew(EditorContainer, SBox)
// // 				[
// // 					Attribute <<= [&]<class T>(const T&) { return sketch::MakeEditor<T>(Item); }
// // 				]
// // 			]
// //
// // 			+ SHorizontalBox::Slot()
// // 			.AutoWidth()
// // 			.VAlign(VAlign_Center)
// // 			[
// // 				ResetButton
// // 			]
// // 		];
// 	}
//
// private:
// 	FText GetNumUsers() const
// 	{
// 		const auto& Host = FSketchModule::Get();
// 		const FAttribute* Attribute = Host[Item];
// 		if (!Attribute)
// 			[[unlikely]]
// 				return LOCTEXT("Err", "Err");
//
// 		if (NumDisplayedUsers != Attribute->GetNumUsers())
// 		[[unlikely]]
// 		{
// 			NumDisplayedUsers = Attribute->GetNumUsers();
// 			FString Result;
// 			Result.Reserve(8);
// 			Result += TEXT("x");
// 			Result.AppendInt(Attribute->GetNumUsers());
// 			NumDisplayedUsersText = FText::FromString(Result);
// 		}
// 		return NumDisplayedUsersText;
// 	}
//
// 	FReply PatchCode()
// 	{
// 		::PatchCode(Item);
// 		return FReply::Handled();
// 	}
//
// 	FReply CopyCode()
// 	{
// 		FSketchModule::Get()[Item] <= []<typename T>(const T& Value)
// 		{
// 			FString Code = sketch::TAttributeTraits<T>::GenerateCode(Value);
// 			FPlatformApplicationMisc::ClipboardCopy(*Code);
// 		};
// 		return FReply::Handled();
// 	}
//
// 	void Reset()
// 	{
// 		// FSketchModule::Get()[Item] <= []<class T>(FAttribute& Attribute)
// 		// {
// 		// 	Attribute.GetValueChecked<T>() = Attribute.GetDefaultValueChecked<T>();
// 		// };
// 		// auto& Host = FSketchModule::Get();
// 		// FAttribute* Attribute = Host[Item];
// 		// EditorContainer->SetContent(
// 		// 	Attribute <= SNullWidget::NullWidget <= [&]<class T>(T&) { return sketch::MakeEditor<T>(Item); }
// 		// );
// 	}
//
// 	EVisibility GetResetButtonVisibility() const
// 	{
// 		const bool bEqual = FSketchModule::Get()[Item] <= []<class T>(const FAttribute& Attribute)
// 		{
// 			if constexpr (requires { std::declval<T>() == std::declval<T>(); })
// 			{
// 				return Attribute.GetValueChecked<T>() == Attribute.GetDefaultValueChecked<T>();
// 			}
// 			else
// 			{
// 				return FMemory::Memcmp(
// 					       &Attribute.GetValueChecked<T>(),
// 					       &Attribute.GetDefaultValueChecked<T>(),
// 					       sizeof(T)
// 				       ) == 0;
// 			}
// 		};
// 		return bEqual ? EVisibility::Hidden : EVisibility::Visible;
// 	}
//
// 	FReply OnBrowseSourceCode(const FGeometry& Geometry, const FPointerEvent& PointerEvent) const
// 	{
// #ifdef UNREALED_API
// 		const FAttribute* Attribute = FSketchModule::Get()[Item];
// 		if (Attribute)
// 		[[likely]]
// 		{
// 			FSourceCodeNavigation::OpenSourceFile(
// 				Item.Source.FileName.ToString(),
// 				Attribute->GetLine(),
// 				Attribute->GetColumn()
// 			);
// 		}
// #endif
// 		return FReply::Handled();
// 	}
//
// 	FAttributePath Item;
// 	mutable uint16 NumDisplayedUsers = ~0;
// 	mutable FText NumDisplayedUsersText;
//
// 	TSharedPtr<SBox> EditorContainer;
// };
//
// TSharedRef<ITableRow> SSketchControllerOLD::OnGenerateRow(FAttributePath Item, const TSharedRef<STableViewBase>& InTree)
// {
// 	if (Item.Index == INDEX_NONE)
// 	{
// 		FString FileName = Item.Source.FileName.ToString();
// 		if (int DelimiterIndex; FileName.FindLastChar(TCHAR('\\'), DelimiterIndex))
// 		[[likely]]
// 		{
// 			FileName.RightChopInline(DelimiterIndex + 1, EAllowShrinking::No);
// 		}
//
// 		FString FunctionName = Item.Source.FunctionName.ToString();
// 		if (int BracketBeforeArgumentsIndex; FunctionName.FindChar(TCHAR('('), BracketBeforeArgumentsIndex))
// 		[[likely]]
// 		{
// 			int DelimiterBeforeFunctionNameIndex = INDEX_NONE;
// 			for (int i = BracketBeforeArgumentsIndex; i > 0; --i)
// 			{
// 				if (/*FunctionName[i] == TCHAR(':') ||*/ FunctionName[i] == TCHAR(' '))
// 				[[unlikely]]
// 				{
// 					DelimiterBeforeFunctionNameIndex = i;
// 					break;
// 				}
// 			}
// 			if (DelimiterBeforeFunctionNameIndex != INDEX_NONE)
// 			[[likely]]
// 			{
// 				FunctionName.LeftInline(BracketBeforeArgumentsIndex, EAllowShrinking::No);
// 				FunctionName.RightChopInline(DelimiterBeforeFunctionNameIndex + 1);
// 			}
// 		}
//
// 		FileName += TEXT(" - ");
// 		FileName += FunctionName;
// 		return SNew(STableRow<FAttributePath>, InTree)
// 			[
// 				SNew(SBox)
// 				.VAlign(VAlign_Center)
// 				[
// 					SNew(STextBlock)
// 					.Text(FText::FromString(FileName))
// 					.Font(Private::DefaultFont())
// 				]
// 			];
// 	}
//
// 	if ((*Host)[Item])
// 	[[likely]]
// 	{
// 		return SNew(STableRow<FAttributePath>, InTree)
// 			[
// 				SNew(SAttribute, Item)
// 			];
// 	}
//
// 	return SNew(STableRow<FAttributePath>, InTree)
// 		[
// 			SNew(STextBlock)
// 			.Text(FText::FromString(TEXT("Invalid")))
// 		];
// }
//
// void SSketchControllerOLD::OnGetChildren(FAttributePath Item, TArray<FAttributePath>& Children)
// {
// 	if (Item.Index != INDEX_NONE)
// 		return;
//
// 	const TSharedPtr<TSparseArray<FAttribute>>* AttributesPtr = Host->Attributes.Find(Item.Source);
// 	const TSparseArray<FAttribute>* Attributes = AttributesPtr ? AttributesPtr->Get() : nullptr;
// 	if (!Attributes)
// 		[[unlikely]]
// 			return;
//
// 	for (auto Attribute = Attributes->CreateConstIterator(); Attribute; ++Attribute)
// 	{
// 		FAttributePath& Child = Children.Emplace_GetRef(Item);
// 		Child.Index = Attribute.GetIndex();
// 	}
// }
//
// FText SSketchControllerOLD::GetNumUsers(FAttributePath Item)
// {
// 	const auto& Host = FSketchModule::Get();
// 	const FAttribute* Attribute = Host[Item];
// 	if (!Attribute)
// 		[[unlikely]]
// 			return LOCTEXT("Err", "Err");
//
// 	FString Result;
// 	Result.Reserve(8);
// 	Result += TEXT("x");
// 	Result.AppendInt(Attribute->GetNumUsers());
// 	return FText::FromString(Result);
// }
//
// void SSketchControllerOLD::CaptureState()
// {
// 	CapturedState.Reset(Host->Attributes.Num());
// 	for (const auto& [Source, _] : Host->Attributes)
// 	{
// 		CapturedState.Emplace(Source);
// 	}
// }
//
// void SSketchControllerOLD::OnAttributesChanged()
// {
// 	CaptureState();
// 	if (Tree)
// 		[[likely]]
// 			Tree->RequestTreeRefresh();
// }
//
// #undef LOCTEXT_NAMESPACE
