#include "HeaderTool/SketchHeaderTool.h"

#include "HeaderTool/SourceCodeUtility.h"
#include "HeaderTool/StringLiteral.h"
#include "AttributesTraites/AttributesTraits.h"
#include "HeaderTool/SketchWidgetFactoryTools.h"

/*************************************************************************************************/
// Live code to string helpers //
/*************************************************************************************************/

#pragma region Live code to string helpers

#define CONCAT(A,B) A##B

#define EXPR(Expression) (void([&](){ Expression; }), TEXT(#Expression))
#define EXPRT(Expression,OmittedTail) (void([&](){ Expression OmittedTail; }), TEXT(#Expression))

#define CALL_0(Expression,...) (void([&](){ Expression(); }), TEXT(#Expression)"()")
#define CALL_1(Expression,ArgDummy,Arg,...) (void([&](){ Expression(ArgDummy); }), TEXT(#Expression)) << TEXT("(") << Arg << TEXT(")")
#define CALL_2(Expression,Arg1Dummy,Arg1,Arg2Dummy,Arg2,...)\
	(void([&](){ Expression(Arg1Dummy,Arg2Dummy); }), TEXT(#Expression)) << TEXT("(") << Arg1 << TEXT(", ") << Arg2 << TEXT(")")
#define CALL_IMPL(Expression, Arg1Dummy, Arg1, Arg2Dummy, Arg2, SUFFIX,...) CALL_##SUFFIX(Expression, Arg1Dummy, Arg1, Arg2Dummy, Arg2)
#define CALL(...) CALL_IMPL(__VA_ARGS__,2,,1,,0,)

#define AINT(Value) 0, Value
#define ASTR(Value) TEXT(""), TEXT("TEXT(\"") << Value << TEXT("\")")
#define APTR(Value) nullptr, TEXT(#Value)
#define AANY(Value) 0, Value
#define AEXP(Value) {}, EXPR(Value)

#pragma endregion ~ Live code to string helpers

/*************************************************************************************************/
// Parsing //
/*************************************************************************************************/

#pragma region Parsing

TArray<sketch::FStringView> GetFractionalNumericSpecializations()
{
	return {
		sketch::FStringView(SL"float"),
		// sketch::FStringView(SL"double"),
	};
}

TArray<sketch::FStringView> GetAllNumericSpecializations()
{
	// No need in littering widget lists with barely different options
	// A fractional and an integer are enough for design time, and can be clarified manually later
	return {
		sketch::FStringView(SL"int"),
		sketch::FStringView(SL"float"),
		// sketch::FStringView(SL"double"),
		// sketch::FStringView(SL"uint8"),
		// sketch::FStringView(SL"int8"),
		// sketch::FStringView(SL"uint16"),
		// sketch::FStringView(SL"int16"),
		// sketch::FStringView(SL"uint32"),
		// sketch::FStringView(SL"int32"),
		// sketch::FStringView(SL"uint64"),
		// sketch::FStringView(SL"int64"),
	};
}

TMap<FName, TArray<sketch::HeaderTool::FOverride>> sketch::HeaderTool::GOverrides = {
	{
		SL"SThrobber",
		{
			FOverride::Value(SL"NumPieces", SL"3"), // Default value is private, so nothing to do there except copying it
			FOverride::Type(SL"Animate", SL"SThrobber::EAnimation"),
		}
	},
	{ SL"STextComboBox", { FOverride::Value(SL"ContentPadding",SL"FCoreStyle::Get().GetWidgetStyle< FComboBoxStyle >(\"ComboBox\").ContentPadding") } },
	{ SL"SScrollBar", { FOverride::Value(SL"Padding", SL"SScrollBar::DefaultUniformPadding") } },
	{
		SL"SWindow",
		{
			FOverride::Value(SL"LayoutBorder", SL"Arguments._Style->BorderPadding"),
			// FOverride::Type((SL"SupportsTransparency"), (SL"EWindowTransparency")),
		}
	},
	{ SL"SViewport", { FOverride::Value(SL"ViewportSize", SL"Arguments.GetDefaultViewportSize()") } },
	{ SL"SScrollBox", { FOverride::Value(SL"ScrollBarThickness", SL"UE::Slate::FDeprecateVector2DParameter(FVector2f(Arguments._Style->BarThickness, Arguments._Style->BarThickness))") } },
	{ SL"SSplitter", { FOverride::Type(SL"SizeRule", SL"SSplitter::ESizeRule", SL"FSlot") } },
	{ SL"SBlock", { FOverride::Value(SL"Text", SL"INVTEXT(\"Sample text\")") } }, // Just a QOL
	{ SL"SOverlay", { FOverride::Slot(SL"FOverlaySlot", SP_SlotOperatorsUsesGeneralName) } },
	{ SL"SNumericDropDown", { FOverride::TemplateSpecializations(GetFractionalNumericSpecializations()) } },
	{
		SL"SNumericEntryBox", {
			FOverride::TemplateSpecializations(GetAllNumericSpecializations()),
			FOverride{ .Name = SL"LabelLocation", .TypeOverride = SL"SNumericEntryBox<NumericType>::ELabelLocation", .ValueOverride = SL"SNumericEntryBox<NumericType>::ELabelLocation::Outside" },
			FOverride::Value(SL"MinFractionalDigits", SL"1"),
			FOverride::Value(SL"MaxFractionalDigits", SL"6"),
		}
	},
	{ SL"SNumericRotatorInputBox", { FOverride::TemplateSpecializations(GetAllNumericSpecializations()) } },
	{
		SL"SSpinBox", {
			FOverride::TemplateSpecializations(GetAllNumericSpecializations()),
			FOverride::Value(SL"MinFractionalDigits", SL"1"),
			FOverride::Value(SL"MaxFractionalDigits", SL"6"),
		}
	},
	{ SL"SBreadcrumbTrail", { FOverride::TemplateSpecializations({ { SL"FString" } }) } },
};

sketch::HeaderTool::Private::FIndex::FIndex(const sketch::FStringView& Code, FLog& Log)
{
	using namespace sketch::SourceCode;

	const bool bInit = [&]
	{
		Scopes.Reserve(128);
		TArray<int, TInlineAllocator<32>> Stack;
		Stack.Reserve(32);
		Lines.Reserve(Code.Len() / 16);
		Lines.Push(0);
		auto OnNewLine = [&](int NewLine)
		{
			Lines.Push(NewLine);
		};
		for (int i = 0; i < Code.Len() - 1; ++i)
		{
			const int AfterFilters = SkipAll(Code, i, OnNewLine, Comment::GSkip, String::GSkipLiteral);
			if (!Code.IsValidIndex(AfterFilters)) [[unlikely]]
			{
				return AfterFilters == Code.Len() && Stack.IsEmpty() ? true : (Log.Err(SL"Failed to index file", { Lines.Num() }), false);
			}
			i = AfterFilters;

			if (Code[i] == TCHAR('{'))
			{
				Scopes.Emplace(i);
				Stack.Push(Scopes.Num() - 1);
			}
			else if (Code[i] == TCHAR('}'))
			{
				if (Stack.IsEmpty()) [[unlikely]] return Log.Err(SL"Unexpected '}'", { Lines.Num(), int(&Code[i] - &Code[Lines.Last()]) }), false;
				const int Pair = Stack.Pop(EAllowShrinking::No);
				Scopes[Pair].End = i;
			}
			else if (Code[i] == TCHAR('\n'))
			{
				Lines.Push(i);
			}
		}
		return true;
	}();
	if (!bInit) [[unlikely]]
	{
		Scopes.Empty();
		Lines.Empty();
	}
}

// FIndex::FIndex(const FString& Code)
// {
// 	Scopes.Reserve(256);
// 	Lines.Reserve(Code.Len() / 16 + 1);
// 	Lines.Push(0);
// 	Expressions.Reserve(Lines.Num());
// 	struct FScope
// 	{
// 		int DeclarationIndex = INDEX_NONE;
// 		TCHAR ExpectedBracket = 0;
// 		int LastExpressionEnding = -1;
// 	};
// 	TArray<FScope, TInlineAllocator<32>> ScopeStack;
// 	ScopeStack.Reserve(32);
// 	ScopeStack.Push({}); // Global scope
//
// 	auto RegisterLine = [&](int NewLineIndex) { Lines.Push(NewLineIndex); };
// 	for (int i = 0; i < Code.Len() - 1;)
// 	{
// 		if (TChar<TCHAR>::IsWhitespace(Code[i]))
// 		{
// 			if (Code[i] == TCHAR('\n'))
// 				Lines.Push(i);
// 			++i;
// 			continue;
// 		}
//
// 		if (IsDirective(Code, i))
// 		{
// 			const int PreprocessorExpressionEnding = CeilPos(Code, SkipDirective(Code, i, RegisterLine));
// 			Expressions.Emplace(ET_Preprocessor, i, PreprocessorExpressionEnding);
// 			i = PreprocessorExpressionEnding;
// 			continue;
// 		}
//
// 		{
// 			const ECommentType CommentType = DetectCommentType(Code, i);
// 			if (CommentType != CT_None)
// 			{
// 				int CommentEnding;
// 				if (CommentType == CT_SingleLine)
// 				{
// 					CommentEnding = FindSingleLineCommentEnding(Code, i, RegisterLine);
// 				}
// 				else
// 				{
// 					check(CommentType == CT_MultiLine);
// 					CommentEnding = FindMultiLineCommentEnding(Code, i, RegisterLine);
// 				}
// 				CommentEnding = CeilPos(Code, CommentEnding);
// 				Expressions.Emplace(ET_Comment, i, CommentEnding);
// 				i = CommentEnding;
// 				continue;
// 			}
// 		}
//
// 		{
// 			constexpr std::array OpeningBrackets = { TCHAR('{'), TCHAR('('), TCHAR('['), };
// 			constexpr std::array ClosingBrackets = { TCHAR('}'), TCHAR(')'), TCHAR(']'), };
// 			constexpr std::array BracketsTypes = { ET_Subscope, ET_Functional, ET_Subscript, };
// 			for (int j = 0; j < OpeningBrackets.size(); ++j)
// 			{
// 				if (Code[i] == OpeningBrackets[j])
// 				{
// 					ScopeStack.Emplace(Expressions.Num(), ClosingBrackets[j], i);
// 					Expressions.Emplace(BracketsTypes[j], i, INDEX_NONE);
// 					++i;
// 					goto next_char;
// 				}
// 			}
// 			for (int j = 0; j < ClosingBrackets.size(); ++j)
// 			{
// 				if (Code[i] == ClosingBrackets[j])
// 				{
// 					// Make sure there's a pair
// 					if (ScopeStack.Num() == 1) [[unlikely]] goto reset;
//
// 					// Make sure the pair is correct
// 					const FScope Bracket = ScopeStack.Pop(EAllowShrinking::No);
// 					if (Bracket.ExpectedBracket != ClosingBrackets[j]) [[unlikely]] goto reset;
//
// 					// If this is a subscope ending, and there's still an unfinished expression
// 					// somewhere before - then it's a function implementation ending, and is an
// 					// ending of an expression like ";"
// 					// If this is a subscope ending, and first previous complete subscope before
// 					// first previous incomplete subscope (if any) is of other type - then this
// 					// is an ending of function implementation, both regular or lambda, and is an
// 					// ending of an expression like ";"
// 					if (BracketsTypes[j] == ET_Subscope)
// 					{
// 						for (int k = Bracket.DeclarationIndex - 1; k >= 0; --k)
// 						{
// 							[] {};
// 							if (Expressions[k].Type == ET_Functional || Expressions[k].Type == ET_Subscript)
// 							{
// 								if (Expressions[k].End != INDEX_NONE)
// 								{
// 								}
// 								break;
// 							}
// 						}
// 					}
//
// 					Expressions[Bracket.DeclarationIndex].End = i;
// 					++i;
// 					goto next_char;
// 				}
// 			}
// 		}
//
// 		if (Code[i] == TCHAR(';'))
// 		{
// 			Expressions.Emplace(ET_Generic, ScopeStack.Top().LastExpressionEnding + 1, i);
// 			ScopeStack.Top().LastExpressionEnding = i;
// 			continue;
// 		}
//
// 	next_char:
// 		continue;
// 	}
// 	return;
//
// reset:
// 	Expressions.Reset();
// 	Scopes.Reset();
// 	Lines.Reset();
// }

sketch::HeaderTool::FSourceLine sketch::HeaderTool::Private::FIndex::LocatePosition(int Position) const
{
	if (Position > Lines.Last()) [[unlikely]]
	{
		return { Lines.Num() - 1, Position - Lines.Last() };
	}

	int Min = 0;
	int Max = Lines.Num() - 1;
	for (int i = 0;; ++i)
	{
		int Mid = (Min + Max) / 2;
		if (Lines[Mid] <= Position && Lines[Mid + 1] > Position)
			return { Mid + 1, Position - Lines[Mid] }; // +1 cause first line is line #1, not #0
		if (Position > Lines[Mid])
			Min = Mid + 1;
		else
			Max = Mid - 1;
		check(i < 2048);
	}
}

sketch::HeaderTool::FSourceLine sketch::HeaderTool::Private::FIndex::LocatePosition(const FString& Code, const sketch::FStringView& View) const
{
	const ptrdiff_t Diff = &View[0] - &Code[0];
	return LocatePosition(Diff);
}

sketch::HeaderTool::FSourceLine sketch::HeaderTool::Private::FIndex::LocatePosition(const FString& Code, const FStringView& View, int Offset) const
{
	const ptrdiff_t Diff = &View[0] - &Code[0];
	return LocatePosition(Diff + Offset);
}

int sketch::HeaderTool::Private::FIndex::FindScope(int Position) const
{
	if (Scopes.IsEmpty() || Position < Scopes[0].Start) return INDEX_NONE;

	// Locate first scope containing given position
	// Then locate last subscope containing given position
	for (int i = 0; i < Scopes.Num(); ++i)
	{
		if (Scopes[i].End > Position)
		{
			// Must never happen
			// If it does - well, I'm obviously mistaken
			check(Scopes[i].Start < Position);

			int SubScope = INDEX_NONE;
			for (int j = i + 1; Scopes[j].Start < Position; ++j)
			{
				if (Scopes[j].End > Position)
				{
					SubScope = j;
				}
			}
			return SubScope == INDEX_NONE ? i : SubScope;
		}
	}
	return INDEX_NONE;
}

int sketch::HeaderTool::Private::FIndex::FindOuterScope(int ScopeIndex) const
{
	int Result = INDEX_NONE;
	for (int i = ScopeIndex - 1; i >= 0; --i)
	{
		if (Scopes[i].Contains(Scopes[ScopeIndex]))
		{
			Result = i;
			break;
		}
	}
	return Result;
}

sketch::HeaderTool::Private::FScope sketch::HeaderTool::Private::FIndex::GetScope(const sketch::FStringView& Code, int ScopeIndex)
{
	FScope GlobalScope{ -1, Code.Len() };
	return ScopeIndex == INDEX_NONE ? GlobalScope : Scopes[ScopeIndex];
}

bool sketch::HeaderTool::FFileBuilder::Init()
{
	using ELogVerbosity::Error;

	Log.BeginFile(FilePath);
	ON_SCOPE_EXIT { Log.EndFile(); };
	if (!FFileHelper::LoadFileToString(Code, *FilePath)) [[unlikely]] return Log.Err(SL"Failed to load file", {}), false;
	Index = Private::FIndex(Code, Log);
	if (!Index.IsValid()) [[unlikely]] return Log.Err(SL"Failed to index file", {}), false;

	int Position = 0;
	for (;;)
	{
		using namespace sketch::SourceCode;

		// Locate next widget args
		const int ArgsStart = String::Find(Code, Position, SL"SLATE_BEGIN_ARGS");
		if (ArgsStart == INDEX_NONE) { return true; }

		const int ArgsEnd = String::Find(Code, ArgsStart, SL"SLATE_END_ARGS");
		if (ArgsEnd == INDEX_NONE) [[unlikely]]
		{
			Log.Warn(SL"No 'SLATE_END_ARGS' for 'SLATE_BEGIN_ARGS'", Index.LocatePosition(ArgsStart));

			// Update position so we can continue at least somehow
			Position = ArgsStart + 1;
			continue;
		}

		// Locate class beginning
		// Args are always in main class scope
		// Locate first scope that ends after given widget attributes (and make sure it contains them)
		// Then find first inner scope that still contains given widget attributes
		const int ScopeIndex = Index.FindScope(ArgsStart);
		if (ScopeIndex == INDEX_NONE) [[unlikely]]
		{
			Log.Warn(SL"Couldn't determine class scope for 'SLATE_BEGIN_ARGS'", Index.LocatePosition(ArgsStart));
			Position = ArgsEnd + 1;
			continue;
		}

		// Once class extents are determined - set next search position
		Position = Index.Scopes[ScopeIndex].End + 1;

		// Locate class name
		// To do it, select code after outer scope beginning, and before widget scop start
		// An, then, find the very last class declaration there
		// Don't forget to skip any subscopes on the way
		sketch::FStringView ClassName;
		sketch::FStringView TemplateArgs;
		sketch::FStringView TemplateArgType;
		const FOverride* TemplateSpecializations = nullptr;
		{
			const int OuterScopeIndex = Index.FindOuterScope(ScopeIndex);
			Private::FScope OuterScope = Index.GetScope(Code, OuterScopeIndex);
			const sketch::FStringView ClassNameContainer = sketch::FStringView(Code).Mid(OuterScope.Start + 1, Index.Scopes[ScopeIndex].Start - OuterScope.Start - 2);
			enum { ClassNameTag, TemplateArgsTag, FirstCharAfterTag };
			auto Pattern = Matcher::ClassDeclaration<{ .TemplateArgumentsTag = TemplateArgsTag, .ClassNameTag = ClassNameTag, .FirstCharAfterClassNameTag = FirstCharAfterTag }>();
			auto AdvancementFilter = CombinedFilter(&Bracket::SubscopeFilter, &Bracket::ArgumentFilter);
			for (TMatcher It(ClassNameContainer, 0, MoveTemp(AdvancementFilter), &NoFilter, Demangle(Pattern)); It; ++It)
			{
				ClassName = It.View<ClassNameTag>();
				TemplateArgs = It.View<TemplateArgsTag>();

				// We want to skip all subscopes, but class declaration matcher stops on subscope beginning, if any
				// So iterator will start new advancement inside subscope instead of skipping it
				const bool bFollowedBySubscope = It.Match.Get<FirstCharAfterTag>().Value.ToView(ClassNameContainer).Equals(SL"{");
				It.Position -= bFollowedBySubscope ? 1 : 0;
			}
		}
		if (ClassName.IsEmpty())[[unlikely]]
		{
			Log.Warn(SL"Couldn't determine class name for 'SLATE_BEGIN_ARGS'", Index.LocatePosition(ArgsStart));
			continue;
		}
		Log.BeginClass(ClassName);
		ON_SCOPE_EXIT { Log.EndClass(); };
		if (!TemplateArgs.IsEmpty())[[unlikely]]
		{
			// Only one template argument supported right now
			const TArray<FArgument> Args = ParseArguments(TemplateArgs);
			if (Args.Num() != 1) [[unlikely]]
			{
				Log.Display(SL"Skipping class as it's a template with more then 1 argument", Index.LocatePosition(Code, ClassName));
				continue;
			}

			// We only process templates when provided with specializations
			const TArray<FOverride>* Overrides = GOverrides.Find(FName(ClassName));
			TemplateSpecializations = Overrides ? Overrides->FindByPredicate([](const FOverride& Override) { return !Override.Specializations.IsEmpty(); }) : nullptr;
			if (!TemplateSpecializations)
			{
				Log.Display(SL"Skipping class as it's a template and no specializations provided", Index.LocatePosition(Code, ClassName));
				continue;
			}

			// Cache template arg type name
			TemplateArgType = Args[0].Name.ToView(TemplateArgs);
		}

		// Make sure this is not an internal data structure
		if (ClassName[0] != TCHAR('S')) [[unlikely]]
		{
			Log.Display(SL"Skipping class as it doesn't seem to be a widget", Index.LocatePosition(Code, ClassName));
			continue;
		}

		// Now find "Construct" and make sure it has only one argument
		// Nothing to do with widgets with multiple "Construct" arguments
		// It's somewhere inside class body
		// And there must be exactly 0 "," inside it brackets
		// Do not look for "Construct" before slate properties though
		// There may be "Construct" for a slot, but not the one we are looking for 'cause FArguments are not
		// defined before "SLATE_END_ARGS"
		{
			// Locate "Construct"
			enum { ArgumentsTag };
			sketch::FStringView ClassBody(&Code[ArgsEnd], Index.Scopes[ScopeIndex].Length() - 1);
			TMatcher Matcher(
				ClassBody,
				&Bracket::SubscopeFilter,
				Matcher::ModuleApi(),
				Matcher::String(SL"void"),
				Matcher::ModuleApi(),
				Matcher::String(SL"Construct"),
				SourceCode::Matcher::Arguments<ArgumentsTag>() // For some reason, resharper fails without "SourceCode::" when tag is present
			);
			if (Matcher.Match.MatchResult != MR_Success) [[unlikely]]
			{
				Log.Warn(SL"Couldn't find 'Construct'", Index.LocatePosition(Code, ClassName));
				continue;
			}

			// Count its explicit arguments
			const FStringView AllArguments = Matcher.Match.Get<ArgumentsTag>().Value.ToView(ClassBody);
			TArray<FArgument> Arguments = ParseArguments(AllArguments);
			TArrayView<FArgument> AdditionalArguments(Arguments);
			AdditionalArguments.RightChopInline(1);
			const bool bHasExplicitArguments = AdditionalArguments.ContainsByPredicate([](const FArgument& Arg) { return !Arg.DefaultValue.IsValid(); });
			if (bHasExplicitArguments)[[unlikely]]
			{
				Log.Display(SL"Skipping class as it has explicit 'Construct' arguments", Index.LocatePosition(Code, AdditionalArguments[0].Type.ToView(AllArguments)));
				continue;
			}
		}

		// Locate slate properties defaults
		// They begin after "SLATE_BEGIN_ARGS(...):" and ends before first "SLATE_"
		// Can't rely on figure bracket since they can be used inside initialization list
		// Offset by (at least) one so SLATE_BEGIN_ARGS doesn't get matched
		sketch::FStringView SlateProperties(&Code[ArgsStart] + 1, ArgsEnd - ArgsStart - 1);
		constexpr sketch::FStringView Prefix(TEXT("SLATE_"));
		int PropertyPosition = String::Find(SlateProperties, 1, Prefix);
		sketch::FStringView PropertiesInitializers(&SlateProperties[0], PropertyPosition - 1);

		// Parse all slate properties
		{
			FClass& Class = Classes.Emplace_GetRef();
			Class.Name.String = ClassName;
			enum { ArgumentTag, AttributeTag, DefaultSlotTag, NamedSlotTag, SlotArgumentTag, EventTag, StyleArgumentTag, UnknownPropertyTag, DeprecatedSuffixTag, DefaultSuffixTag, ArgumentsTag };
			auto Property = Demangle(
					TMatcher(
						SlateProperties,
						Matcher::String(SL"SLATE_"),
						Matcher::OneOf(
							Matcher::String<ArgumentTag>(SL"ARGUMENT"),
							Matcher::String<AttributeTag>(SL"ATTRIBUTE"),
							Matcher::String<DefaultSlotTag>(SL"DEFAULT_SLOT"),
							Matcher::String<NamedSlotTag>(SL"NAMED_SLOT"),
							Matcher::String<SlotArgumentTag>(SL"SLOT_ARGUMENT"),
							Matcher::String<EventTag>(SL"EVENT"),
							Matcher::String<StyleArgumentTag>(SL"STYLE_ARGUMENT"),
							Matcher::String(SL"ITEMS_SOURCE_ARGUMENT"),
							Matcher::Name<UnknownPropertyTag>()
						),
						Matcher::OneOf(
							ST_Optional,
							Matcher::String<DeprecatedSuffixTag>(SL"_DEPRECATED"),
							Matcher::String<DefaultSuffixTag>(SL"_DEFAULT")
						),
						Matcher::Arguments<ArgumentsTag>()
					)
				);
			for (; Property; ++Property)
			{
				if (Property.Match.Get<UnknownPropertyTag>().MatchResult == MR_Success)[[unlikely]]
				{
					Log.Warn(SL"Unknown slate property '%s'", Index.LocatePosition(Code, SlateProperties, Property.Match.Value.FirstOf), *Property.Match.Get<UnknownPropertyTag>().Value.ToView(SlateProperties).ToString());
					continue;
				}

				FLocalStringView Arguments = Property.Match.Get<ArgumentsTag>().Value;
				++Arguments.FirstOf;
				--Arguments.FirstAfter;
				if (Property.Match.Get<ArgumentTag>().MatchResult == MR_Success || Property.Match.Get<AttributeTag>().MatchResult == MR_Success)
				{
					const bool bDeprecated = Property.Match.Get<DeprecatedSuffixTag>().MatchResult == MR_Success;
					const bool bExplicitDefault = Property.Match.Get<DefaultSuffixTag>().MatchResult == MR_Success;
					FProperty Attribute = ProcessAttribute(SlateProperties, Arguments, bDeprecated, bExplicitDefault, PropertiesInitializers);
					if (Attribute.IsValid()) Class.Properties.Emplace(MoveTemp(Attribute));
				}
				else if (Property.Match.Get<NamedSlotTag>().MatchResult == MR_Success || Property.Match.Get<DefaultSlotTag>().MatchResult == MR_Success)
				{
					FSlot Slot = ProcessUniqueSlot(SlateProperties, Arguments);
					if (Slot.IsValid()) Class.NamedSlots.Emplace(MoveTemp(Slot));
				}
				else if (Property.Match.Get<SlotArgumentTag>().MatchResult == MR_Success)
				{
					FSlot Slot = ProcessDynamicSlot(SlateProperties, Arguments, ScopeIndex);
					if (Slot.IsValid()) Class.DynamicSlots.Emplace(MoveTemp(Slot));
				}
			}
		}

		// Instantiate all provided template specializations
		if (TemplateSpecializations)[[unlikely]]
		{
			// Create all additional copies
			const int FirstInstance = Classes.Num() - 1;
			Classes.Reserve(Classes.Num() + TemplateSpecializations->Specializations.Num() - 1);
			for (int i = 1; i < TemplateSpecializations->Specializations.Num(); ++i)
			{
				// It's an UB if new classes are not preallocated
				Classes.Emplace(Classes[FirstInstance]);
			}

			// Replace each copy template type with specialized type
			for (int i = 0; i < TemplateSpecializations->Specializations.Num(); ++i)
			{
				// Update class name
				FClass& ClassInstance = Classes[FirstInstance + i];
				ClassInstance.Name.Container = ClassInstance.Name.String.ToString();
				ClassInstance.Name.Container.AppendChar(TCHAR('<'));
				ClassInstance.Name.Container.Append(TemplateSpecializations->Specializations[i].ToCommonStringView());
				ClassInstance.Name.Container.AppendChar(TCHAR('>'));
				ClassInstance.Name.String = ClassInstance.Name.Container;

				// Make property type updater
				auto TryPatchProperty = [&](FProperty& Property)
				{
					auto PatchExpression = [&](SourceCode::FProcessedString& Expression)
					{
						const int TemplateTypeLocation = Expression.String.Find(TemplateArgType);
						const int PreviousCharIndex = TemplateTypeLocation - 1;
						const int FollowingCharIndex = TemplateTypeLocation + TemplateArgType.Len();
						const bool bPreviousCharIsUnrelated = (PreviousCharIndex < 0) || !IsNameChar(Expression.String[PreviousCharIndex]);
						const bool bFollowingCharIsUnrelated = (FollowingCharIndex >= Expression.String.Len()) || !IsNameChar(Expression.String[FollowingCharIndex]);
						if (TemplateTypeLocation != INDEX_NONE && bPreviousCharIsUnrelated && bFollowingCharIsUnrelated)
						{
							Expression.Container = Expression.String.Left(TemplateTypeLocation).ToCommonStringView();
							Expression.Container.Append(TemplateSpecializations->Specializations[i].ToCommonStringView());
							Expression.Container.Append(Expression.String.RightChop(TemplateTypeLocation + TemplateArgType.Len()).ToCommonStringView());
							Expression.String = Expression.Container;
							return true;
						}
						return false;
					};

					if (Property.Type.String == TemplateArgType)
					{
						Property.Type = { .String = TemplateSpecializations->Specializations[i] };
						Property.bSupported = IsSupportedAttributeType(Property.Type.String);
					}
					else if (PatchExpression(Property.Type))
					{
						Property.bSupported = IsSupportedAttributeType(Property.Type.String);
					}
					PatchExpression(Property.DefaultValue);
				};

				// Fix properties types
				for (FProperty& Property : ClassInstance.Properties)
				{
					TryPatchProperty(Property);
				}

				// Fix dynamic slots properties types
				// Named slots are skipped since they can't own any properties
				for (FSlot& Slot : ClassInstance.DynamicSlots)
				{
					for (FProperty& Property : Slot.Properties)
					{
						TryPatchProperty(Property);
					}
				}
			}
		}
	}
}

sketch::HeaderTool::FProperty sketch::HeaderTool::FFileBuilder::ProcessProperty(
	const FStringView& Properties,
	const SourceCode::FLocalStringView& PropertyBody,
	bool bDeprecated,
	bool bExplicitDefaultValue,
	const FStringView& PropertiesDefaults,
	const FStringView& SlotType
)
{
	// Locate property components
	using namespace SourceCode;
	enum { TypeTag, CommaTag, NameTag };
	FStringView PropertyBodyView = PropertyBody.ToView(Properties);
	auto PropertyBodyStructure = Match(
		PropertyBodyView,
		0,
		&NoFilter,
		Matcher::TypeName<TypeTag>(),
		Matcher::String<CommaTag>(SL","),
		Matcher::Name<NameTag>(),
		Matcher::Subsequence(
			bDeprecated ? ST_Required : ST_MustNotBePresent,
			&NoFilter,
			Matcher::String(SL","),
			Matcher::Digit(),
			Matcher::String(SL"."),
			Matcher::Digit(),
			Matcher::String(SL","),
			Matcher::StringLiteral()
		)
	);
	if (PropertyBodyStructure.MatchResult != MR_Success) [[unlikely]]
	{
		const TCHAR* What =
			PropertyBodyStructure.Get<TypeTag>().MatchResult == MR_Failure && PropertyBodyStructure.Get<CommaTag>().MatchResult == MR_Failure
				? SL"type"
				: PropertyBodyStructure.Get<NameTag>().MatchResult == MR_Failure
				? SL"name"
				: SL"deprecation message";
		Log.Err(SL"Failed parsing property %s", Index.LocatePosition(Code, PropertyBodyView), What);
		return {};
	}

	// Init basic info
	FProperty Result;
	Result.Type = CleanCode(PropertyBodyStructure.Get<TypeTag>().Value.ToView(PropertyBodyView));
	Result.Name = PropertyBodyStructure.Get<NameTag>().Value.ToView(PropertyBodyView);

	// Locate property default value
	// Note that constructor initializer list has a higher priority then explicit default value
	{
		enum { ValueTag };
		TMatcher InitializerMatcher(
			PropertiesDefaults,
			Matcher::String(SL"_"),
			Matcher::String(Result.Name),
			Matcher::OneOf<ValueTag>(
				Matcher::Arguments(),
				Matcher::Subscope()
			)
		);
		if (InitializerMatcher)
		{
			sketch::FStringView InitializerView = InitializerMatcher.Match.Get<ValueTag>().Value.ToView(PropertiesDefaults);
			InitializerView = InitializerView.Mid(1, InitializerView.Len() - 2);
			FProcessedString Initializer = CleanCode(InitializerView);
			if (!Initializer.String.IsEmpty())
			{
				Result.DefaultValue = MoveTemp(Initializer);
			}
		}
		else if (bExplicitDefaultValue)
		{
			enum { SemicolonTag };
			constexpr auto AnySubscopeFilter = CombinedFilter(&Bracket::SubscopeFilter, &Bracket::ArgumentFilter, &Bracket::TemplateFilter);
			TMatcher DefaultValueMatcher(
				Properties,
				PropertyBody.FirstAfter,
				CopyTemp(AnySubscopeFilter),
				CopyTemp(AnySubscopeFilter),
				Matcher::String<SemicolonTag>(SL";"),
				Matcher::String(SL"SLATE_")
			);
			if (DefaultValueMatcher.Match.MatchResult == MR_Success)[[likely]]
			{
				sketch::FStringView DefaultValueView = Properties.Mid(PropertyBody.FirstAfter, DefaultValueMatcher.Match.Get<SemicolonTag>().Value.FirstOf - PropertyBody.FirstAfter);
				DefaultValueView.TrimStartAndEndInline();
				DefaultValueView.RemovePrefixInline(SL"=");
				FProcessedString DefaultValue = CleanCode(DefaultValueView);
				if (!DefaultValue.String.IsEmpty())
				{
					Result.DefaultValue = MoveTemp(DefaultValue);
				}
			}
			else
			{
				Log.Warn(SL"Failed parsing property '%s' default value", Index.LocatePosition(Code, DefaultValueMatcher.Match.Get<SemicolonTag>().Value.ToView(Properties)), *Result.Name.ToString());
			}
		}
	}

	// Apply predefined override
	const FName ClassNameName = FName(Classes.Last().Name.String.ToCommonStringView(), FNAME_Find);
	if (TArray<FOverride>* Overrides = GOverrides.Find(ClassNameName))
	[[unlikely]]
	{
		const FOverride* Override = Overrides->FindByPredicate([&](const FOverride& SomeOverride)
		{
			return SomeOverride.Name == Result.Name && SomeOverride.SlotType == SlotType;
		});
		if (Override) [[unlikely]]
		{
			Result.bSupported = true;
			if (Override->TypeOverride.GetData())
			{
				Result.Type.Container.Empty();
				Result.Type.String = Override->TypeOverride;
			}
			if (Override->ValueOverride.GetData())
			{
				Result.DefaultValue.String = Override->ValueOverride;
			}
		}
	}

	return Result;
}

sketch::HeaderTool::FProperty sketch::HeaderTool::FFileBuilder::ProcessAttribute(
	const FStringView& Properties,
	const SourceCode::FLocalStringView& PropertyBody,
	bool bDeprecated,
	bool bExplicitDefaultValue,
	const FStringView& PropertiesDefaults,
	const FStringView SlotType
)
{
	auto Result = ProcessProperty(Properties, PropertyBody, bDeprecated, bExplicitDefaultValue, PropertiesDefaults, SlotType);
	Result.bSupported = IsSupportedAttributeType(Result.Type.String);
	return Result;
}

sketch::HeaderTool::FSlot sketch::HeaderTool::FFileBuilder::ProcessUniqueSlot(
	const FStringView& Properties,
	const SourceCode::FLocalStringView& PropertyBody
)
{
	FProperty Property = ProcessProperty(Properties, PropertyBody);
	if (!Property.IsValid()) [[unlikely]] return {};

	FSlot Result;
	Result.Name = Property.Name;
	return Result;
}

sketch::HeaderTool::FSlot sketch::HeaderTool::FFileBuilder::ProcessDynamicSlot(
	const FStringView& Properties,
	const SourceCode::FLocalStringView& PropertyBody,
	int ClassScopeIndex
)
{
	// Process common property data
	FSlot Result;
	{
		FProperty Property = ProcessProperty(Properties, PropertyBody);
		if (!Property.IsValid()) [[unlikely]] return {};
		if (!Property.Type.Container.IsEmpty()) [[unlikely]] return Log.Warn(SL"Unexpected slot type '%s'", Index.LocatePosition(Code, PropertyBody.ToView(Properties)), *Property.Type.Container), FSlot{};
		Result.Name = Property.Name;
		Result.Type = Property.Type.String;
		if (const int ColonIndex = Result.Type.FindLast(TCHAR(':')))
		{
			Result.Type = Result.Type.RightChop(ColonIndex + 1);
		}
	}
	Log.BeginSlot(Result.Type);
	ON_SCOPE_EXIT { Log.EndSlot(); };

	// // Make sure class contains "static FSlot::FSlotArguments Slot()"
	// FStringView ClassBody(&Code[Index.Scopes[ClassScopeIndex].Start + 1], Index.Scopes[ClassScopeIndex].End - Index.Scopes[ClassScopeIndex].Start - 2);
	// // Or don't 'cause we are not going to use it either way
	// // {
	// // 	auto FindSlot = [&](int Position)
	// // 	{
	// // 		return Sequence::Find(
	// // 			ClassBody,
	// // 			Position,
	// // 			Sequence::SubscopeFilter,
	// // 			Sequence::TemplateDeclaration(Sequence::ST_MustNotBePresent),
	// // 			Sequence::String(TEXT("static")),
	// // 			Sequence::ModuleApi(Sequence::ST_Optional),
	// // 			Sequence::String(Slot.TypeName),
	// // 			Sequence::String(TEXT("::")),
	// // 			Sequence::String(GET_TYPE_NAME_STRING_CHECKED(SHorizontalBox::FSlot::, FSlotArguments)),
	// // 			Sequence::String(Slot.TypeName.RightChop(1)),
	// // 			Sequence::String(TEXT("(")),
	// // 			Sequence::String(TEXT(")"))
	// // 		);
	// // 	};
	// // 	auto SlotConstructor = FindSlot(0);
	// // 	if (!SlotConstructor) [[unlikely]]
	// // 	{
	// // 		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
	// // 		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find slot constructor for dynamic slot '%s' at %i::%i"), *FString(Slot.TypeName), Position.Line, Position.Column);
	// // 		return {};
	// // 	}
	// // }

	// Make sure class contains "ReturnType AddSlot(...)"
	using namespace SourceCode;
	const FStringView ClassBody = Index.Scopes[ClassScopeIndex].ToView(Code);
	TArray<FOverride>* Overrides = GOverrides.Find(FName(Classes.Last().Name.String.ToCommonStringView(), FNAME_Find));
	FOverride* Override = Overrides ? Overrides->FindByPredicate([&](const FOverride& SomeOverride) { return SomeOverride.SlotType == Result.Type; }) : nullptr;
	if (!Override || !EnumHasAnyFlags(Override->SlotProperties, SP_ConstructorInherited)) [[likely]]
	{
		enum { ArgumentsTag };
		FString MethodName = TEXT("Add");
		MethodName.Append(
			Override && EnumHasAllFlags(Override->SlotProperties, SP_SlotOperatorsUsesGeneralName)
				? ::FStringView(SL"Slot")
				: Result.Type.RightChop(1).ToCommonStringView()
		);
		TMatcher ConstructorMatcher(
			ClassBody,
			CombinedFilter(&Bracket::SubscopeFilter, &Bracket::TemplateFilter),
			Matcher::ModuleApi(),
			Matcher::Name(),
			Matcher::ModuleApi(),
			Matcher::String(MethodName),
			Matcher::Arguments<ArgumentsTag>()
		);
		if (!ConstructorMatcher) [[unlikely]]
			return Log.Warn(SL"Couldn't find AddSlot", Index.LocatePosition(Code, PropertyBody.ToView(Properties))), FSlot{};

		// Collect constructor arguments
		// For now make sure there are none, or all of them has default values
		// Their support will be implemented later
		sketch::FStringView ArgsView = ConstructorMatcher.Match.Get<ArgumentsTag>().Value.ToView(ClassBody);
		ArgsView.MidInline(1, ArgsView.Len() - 2);
		ArgsView.TrimStartAndEndInline();
		TArray<FArgument> Args = ParseArguments(ArgsView);
		if (Args.ContainsByPredicate([](const FArgument& Argument) { return !Argument.DefaultValue.IsValid(); })) [[unlikely]] return Log.Warn(SL"Couldn't find AddSlot without explicit arguments", Index.LocatePosition(Code, PropertyBody.ToView(Properties))), FSlot{};
	}

	// Make sure class contains "ReturnType RemoveSlot(...)"
	if (!Override || !EnumHasAllFlags(Override->SlotProperties, SP_DestructorInherited)) [[likely]]
	{
		FString MethodName = SL"Remove";
		MethodName.Append(
			Override && EnumHasAllFlags(Override->SlotProperties, SP_SlotOperatorsUsesGeneralName)
				? ::FStringView(TEXT("Slot"))
				: Result.Type.RightChop(1).ToCommonStringView()
		);
		TMatcher ConstructorMatcher(
			ClassBody,
			CombinedFilter(&Bracket::SubscopeFilter, &Bracket::TemplateFilter),
			Matcher::ModuleApi(),
			Matcher::OneOf(
				Matcher::String(SL"void"),
				Matcher::String(SL"bool"),
				Matcher::String(SL"int32"),
				Matcher::String(SL"int")
			),
			Matcher::ModuleApi(),
			Matcher::String(MethodName),
			Matcher::String(SL"("),
			Matcher::OneOf(
				Matcher::String(SL"int32"),
				Matcher::String(SL"int"),
				Matcher::Subsequence(
					Matcher::String(SL"const", ST_Optional),
					Matcher::String(SL"TSharedRef"),
					Matcher::TemplateArguments(),
					Matcher::String(SL"&", ST_Optional)
				)
			),
			Matcher::Name(ST_Optional),
			Matcher::String(SL")")
		);
		if (!ConstructorMatcher)[[unlikely]] return Log.Warn(SL"Couldn't find RemoveSlot", Index.LocatePosition(Code, PropertyBody.ToView(Properties))), FSlot{};
	}

	// Locate slot class within class body. Note that "SLATE_SLOT_ARGUMENT" depends on slot public interface,
	// so full class definition have to be present before widget's properties
	int SlotScopeIndex = INDEX_NONE;
	FSourceLine SlotClassLocation;
	{
		const FStringView SlotDeclarationScope = sketch::FStringView(Code).Mid(
			Index.Scopes[ClassScopeIndex].Start + 1,
			&Properties[0] - &Code[Index.Scopes[ClassScopeIndex].Start] - 2
		);
		enum { ClassNameTag, FirstCharAfterTag };
		TMatcher SlotDeclarationMatcher(
			SlotDeclarationScope,
			&Bracket::AnySubscopeFilter,
			Matcher::ClassDeclaration<{ .ClassNameTag = ClassNameTag, .FirstCharAfterClassNameTag = FirstCharAfterTag }>()
		);
		for (;;)
		{
			if (!SlotDeclarationMatcher)[[unlikely]]return Log.Err(SL"Couldn't find slot class", Index.LocatePosition(Code, PropertyBody.ToView(Properties))), FSlot{};

			const bool bNameMatch = SlotDeclarationMatcher.View<ClassNameTag>() == Result.Type;
			const bool bForwardDeclaration = SlotDeclarationMatcher.View<FirstCharAfterTag>() == SL";";
			if (bNameMatch && !bForwardDeclaration) [[likely]] break;
			++SlotDeclarationMatcher;
		}

		int SlotBodyScopeBeginning;
		if (SlotDeclarationMatcher.View<FirstCharAfterTag>() == SL"{")
		{
			SlotBodyScopeBeginning = &SlotDeclarationMatcher.View<FirstCharAfterTag>()[0] - &Code[0];
		}
		else
		{
			enum { BracketTag };
			TMatcher ScopeBeginningMatcher(
				SlotDeclarationScope,
				SlotDeclarationMatcher.Match.Get<FirstCharAfterTag>().Value.FirstAfter,
				&NoFilter,
				CombinedFilter(&Bracket::TemplateFilter, &Bracket::ArgumentFilter),
				Matcher::String<BracketTag>(SL"{")
			);
			if (ScopeBeginningMatcher) [[likely]]
			{
				SlotBodyScopeBeginning = &ScopeBeginningMatcher.View<BracketTag>()[0] - &Code[0];
			}
			else
			{
				return Log.Err(SL"Couldn't find slot class", Index.LocatePosition(Code, PropertyBody.ToView(Properties))), FSlot{};
			}
		}
		SlotScopeIndex = Index.FindScope(SlotBodyScopeBeginning + 1);
		SlotClassLocation = Index.LocatePosition(Code, SlotDeclarationScope, SlotDeclarationMatcher.Match.Get<ClassNameTag>().Value.FirstOf);
	}

	FStringView SlotProperties;
	{
		// Parse slot declaration
		const sketch::FStringView SlotBody = FStringView(Code).Mid(
			Index.Scopes[SlotScopeIndex].Start + 1,
			Index.Scopes[SlotScopeIndex].Length() - 2
		);
		enum { OneMixinTag, TwoMixinsTag, ThreeMixinsTag, FourMixinsTag, UnknownMixinsTag, ParentTag, FirstMixinTag, SecondMixinTag, ThirdMixinTag, FourthMixinTag };
		auto SlotClassArgsMatcher = Demangle(
				TMatcher(
					SlotBody,
					0,
					&Bracket::AnySubscopeFilter,
					&NoFilter,
					Matcher::String(SL"SLATE_SLOT_BEGIN_ARGS"),
					Matcher::Subsequence(
						ST_Optional,
						Matcher::String(SL"_"),
						Matcher::OneOf(
							Matcher::String<OneMixinTag>(SL"OneMixin"),
							Matcher::String<TwoMixinsTag>(SL"TwoMixins"),
							Matcher::String<ThreeMixinsTag>(SL"ThreeMixins"),
							Matcher::String<FourMixinsTag>(SL"FourMixins"),
							Matcher::Name<UnknownMixinsTag>()
						)
					),
					Matcher::String(SL"("),
					Matcher::String(Result.Type),
					Matcher::String(SL","),
					Matcher::Subsequence<ParentTag>(
						Matcher::Name(),
						Matcher::TemplateArguments()
					),
					Matcher::Subsequence(
						ST_Optional,
						Matcher::String(SL","),
						Matcher::TypeName<FirstMixinTag>(),
						Matcher::Subsequence(
							ST_Optional,
							Matcher::String(SL","),
							Matcher::TypeName<SecondMixinTag>(),
							Matcher::Subsequence(
								ST_Optional,
								Matcher::String(SL","),
								Matcher::TypeName<ThirdMixinTag>(),
								Matcher::Subsequence(
									ST_Optional,
									Matcher::String(SL","),
									Matcher::TypeName<FourthMixinTag>()
								)
							)
						)
					),
					Matcher::String(SL")")
				)
			);
		if (!SlotClassArgsMatcher) [[unlikely]]
			return Log.Err(SL"Failed parsing slate slot structure", SlotClassLocation), FSlot{};
		if (SlotClassArgsMatcher.Matched<UnknownMixinsTag>()) return Log.Err(SL"Unknown number of slot mixins '%s'", SlotClassLocation, *SlotClassArgsMatcher.String<UnknownMixinsTag>()), FSlot{};
		{
			const bool bOneMixin = SlotClassArgsMatcher.Matched<OneMixinTag>() == SlotClassArgsMatcher.Matched<FirstMixinTag>();
			const bool bTwoMixins = SlotClassArgsMatcher.Matched<TwoMixinsTag>() == SlotClassArgsMatcher.Matched<SecondMixinTag>();
			const bool bThreeMixins = SlotClassArgsMatcher.Matched<ThreeMixinsTag>() == SlotClassArgsMatcher.Matched<ThirdMixinTag>();
			const bool bFourMixins = SlotClassArgsMatcher.Matched<FourMixinsTag>() == SlotClassArgsMatcher.Matched<FourthMixinTag>();
			if (!bOneMixin && !bTwoMixins && !bThreeMixins && !bFourMixins) [[unlikely]] return Log.Err(SL"Mismatching number of slot mixins", SlotClassLocation), FSlot{};
		}

		// Gather mixins
		enum { MIXIN_Padding, MIXIN_Alignment, MIXIN_Resizing, NUM_MIXINS };
		std::array<bool, NUM_MIXINS> Mixins;
		Mixins.fill(false);
		if (SlotClassArgsMatcher.View<ParentTag>().StartsWith(SL"TBasicLayoutWidgetSlot"))
		{
			Mixins[MIXIN_Padding] = true;
			Mixins[MIXIN_Alignment] = true;
		}
		for (const sketch::FStringView& Mixin : {
			     SlotClassArgsMatcher.View<FirstMixinTag>(),
			     SlotClassArgsMatcher.View<SecondMixinTag>(),
			     SlotClassArgsMatcher.View<ThirdMixinTag>(),
			     SlotClassArgsMatcher.View<FourthMixinTag>()
		     })
		{
			if (!Mixin.IsEmpty())
			{
				const int MixinIndex = Mixin.StartsWith(SL"TAlignment") ? MIXIN_Alignment : Mixin.StartsWith(SL"TPadding") ? MIXIN_Padding : MIXIN_Resizing;
				Mixins[MixinIndex] = true;
			}
		}

		// Add each mixin attribute
		if (Mixins[MIXIN_Padding])
		{
			constexpr sketch::FStringView MarginName = EXPR(FMargin);
			Result.Properties.Emplace(
				FProperty{
					.Type = { .String = MarginName },
					.Name = TEXT("Padding"),
					.bSupported = IsSupportedAttributeType(MarginName)
				}
			);
		}
		if (Mixins[MIXIN_Alignment])
		{
			constexpr FStringView HAlignmentName = EXPR(EHorizontalAlignment);
			constexpr FStringView VAlignmentName = EXPR(EVerticalAlignment);
			Result.Properties.Emplace(
				FProperty{
					.Type = { .String = HAlignmentName },
					.Name = TEXT("HAlign"),
					.DefaultValue = FProcessedString{ .String = EXPR(HAlign_Fill) },
					.bSupported = IsSupportedAttributeType(HAlignmentName)
				});
			Result.Properties.Emplace(
				FProperty{
					.Type = { .String = VAlignmentName },
					.Name = TEXT("VAlign"),
					.DefaultValue = FProcessedString{ .String = EXPR(VAlign_Fill) },
					.bSupported = IsSupportedAttributeType(VAlignmentName)
				});
		}
		if (Mixins[MIXIN_Resizing])
		{
			// As of 5.6 epic uses slate attributes for this mixin, but not every widget registers its slot attributes
			// So this mixin's attributes can't be binds, only plain values
			// If widget explicitly registers - we can assume it registers its slot's attributes as well
			[[maybe_unused]] SBoxPanel::FSlot* Slot;
			const int WidgetDeclaration = String::Find(ClassBody, 0, SL"SLATE_DECLARE_WIDGET", &Bracket::AnySubscopeFilter);
			Result.Properties.Emplace(
				FProperty{
					.CustomEntityInitializationCode = {
						.String = WidgetDeclaration == INDEX_NONE
							          ? EXPR(HeaderTool::InitResizingMixinProperties(*Slot, false);)
							          : EXPR(HeaderTool::InitResizingMixinProperties(*Slot, true);)
					}
				}
			);
		}

		// Locate slot attributes ending
		const int ArgsEnding = String::Find(SlotBody, SlotClassArgsMatcher.Match.Value.FirstAfter, SL"SLATE_SLOT_END_ARGS", &Bracket::ArgumentFilter);
		if (ArgsEnding == INDEX_NONE) [[unlikely]] return Log.Err(SL"Failed to locate 'SLATE_SLOT_END_ARGS'", SlotClassLocation), FSlot{};
		SlotProperties = SlotBody.Mid(SlotClassArgsMatcher.Match.Value.FirstAfter, ArgsEnding - SlotClassArgsMatcher.Match.Value.FirstAfter);
	}

	// Parse slate properties
	enum { EventPropertyTag, UnknownPropertyTag, DeprecatedSuffixTag, DefaultSuffixTag, ArgumentsTag };
	auto PropertyMatcher = Demangle(
			TMatcher(
				SlotProperties,
				Matcher::String(SL"SLATE_"),
				Matcher::OneOf(
					Matcher::String(SL"ARGUMENT"),
					Matcher::String(SL"ATTRIBUTE"),
					Matcher::String<EventPropertyTag>(SL"EVENT"),
					Matcher::Name<UnknownPropertyTag>()
				),
				Matcher::OneOf(
					ST_Optional,
					Matcher::String<DeprecatedSuffixTag>(SL"_DEPRECATED"),
					Matcher::String<DefaultSuffixTag>(SL"_DEFAULT")
				),
				Matcher::Arguments<ArgumentsTag>()
			)
		);
	for (; PropertyMatcher; ++PropertyMatcher)
	{
		if (PropertyMatcher.Matched<UnknownPropertyTag>()) [[unlikely]]
		{
			Log.Warn(SL"Unknown slate property '%s'", Index.LocatePosition(Code, SlotProperties, PropertyMatcher.Match.Get<UnknownPropertyTag>().Value.FirstOf), *PropertyMatcher.String<UnknownPropertyTag>());
			continue;
		}
		if (PropertyMatcher.Matched<EventPropertyTag>())[[unlikely]] continue;

		FLocalStringView Arguments = PropertyMatcher.Match.Get<ArgumentsTag>().Value;
		++Arguments.FirstOf;
		--Arguments.FirstAfter;
		const bool bDeprecated = PropertyMatcher.Matched<DeprecatedSuffixTag>();
		const bool bExplicitDefault = PropertyMatcher.Matched<DefaultSuffixTag>();
		FProperty Attribute = ProcessAttribute(SlotProperties, Arguments, bDeprecated, bExplicitDefault, {}, Result.Type);
		Result.Properties.Emplace(MoveTemp(Attribute));
	}

	return MoveTemp(Result);
}

sketch::HeaderTool::FFileBuilder::FFileBuilder(FString&& InFilePath, FLog& InLog) : Log(InLog)
{
	FilePath = MoveTemp(InFilePath);
	Init();
}

#pragma endregion ~ Parsing

/*************************************************************************************************/
// Generation //
/*************************************************************************************************/

#pragma region Generation

inline FString& operator<<(FString& A, const TCHAR* B) { return A.Append(B); }
inline FString& operator<<(FString& A, const FStringView& B) { return A.Append(B); }
inline FString& operator<<(FString& A, const sketch::SourceCode::FProcessedString& B) { return A << B.String; }
inline FString& operator<<(FString& A, int B) { return A.AppendInt(B), A; }

FString& operator<<(FString& A, const sketch::HeaderTool::FProperty& B)
{
	const bool bPointerType = B.Type.String.EndsWith(SL"*");
	if (bPointerType)
		A << TEXT("(");
	A << B.Type;
	if (bPointerType)
		A << TEXT(")");

	if (bPointerType && B.DefaultValue.String.IsEmpty()) [[unlikely]]
		A << TEXT(" nullptr ");
	else
		A << TEXT("( ") << B.DefaultValue << TEXT(" )");
	return A;
}

template <class T>
struct TOpt
{
	TOpt(bool InCondition, const T& InValue)
		: Condition(InCondition)
		, Value(InValue) {}

	bool Condition;
	const T& Value;
};

template <class T>
FString& operator<<(FString& A, const TOpt<T>& B)
{
	return B.Condition ? (A << B.Value) : A;
}

TArray<sketch::HeaderTool::FFile> sketch::HeaderTool::Scan(FLog& Log, const FString& Path, bool bRecursive)
{
	auto& FileManager = IFileManager::Get();
	TArray<FString> Files;
	if (bRecursive)
	{
		FileManager.FindFilesRecursive(Files, *Path, TEXT("*.h"), true, false, false);
	}
	else
	{
		FileManager.FindFiles(Files, *Path, TEXT(".h"));
	}

	// File gatherer is not deterministic, so fix it now
	Files.Sort();

	TArray<FFile> Result;
	Result.Reserve(Files.Num());
	for (FString& File : Files)
	{
		FString Code;
		if (bRecursive)
		{
			FFileHelper::LoadFileToString(Code, *File);
		}
		else
		{
			File = Path / File;
			FFileHelper::LoadFileToString(Code, *File);
		}
		FString FileName = FPaths::GetCleanFilename(File);
		FFileBuilder FileBuilder(MoveTemp(File), Log);
		if (!FileBuilder.Classes.IsEmpty())
		{
			Result.Emplace(MoveTemp(FileBuilder));
		}
	}
	return Result;
}

sketch::HeaderTool::FReflectionGenerator::FReflectionGenerator(FStringView RegistrarName)
{
	// Generate prologue
	Prologue << TEXT("// Generated by Sketch Header Tool on ") << FDateTime::Now().ToString() << TEXT("\r\n");
	Prologue << TEXT("#include \"Sketch.h\"\r\n");
	Prologue << TEXT("#include \"Sketch/Public/Widgets/SSketchWidget.h\"\r\n");
	Prologue << TEXT("#include \"HeaderTool/SketchWidgetFactoryTools.h\"\r\n");
	Prologue << TEXT("\r\n");

	// Generate epilogue
	Epilogue
		<< TEXT("void Register") << RegistrarName << TEXT("Factories()\r\n")
		<< TEXT("{\r\n");
}

void sketch::HeaderTool::FReflectionGenerator::Add(const FClass& Class)
{
	// Dereference pointers for convenience
	const FFile& File = *FilePtr;
	const FString& InclusionRoot = *InclusionRootPtr;

	// Prologue
	FStringView Include(&File.FilePath[InclusionRoot.Len() + 1], File.FilePath.Len() - InclusionRoot.Len() - 1);
	Prologue << SL"#include \"" << Include << SL"\"\r\n";

	{
		// Watch out for templates
		// If clas is a template instantiation - synthesize an acceptable registrar name first
		SourceCode::FProcessedString ClassName{ .String = Class.Name.String };
		if (Class.Name.String.EndsWith(SL">"))[[unlikely]]
		{
			ClassName.Container = ClassName.String.ToCommonStringView();
			ClassName.Container.LeftInline(ClassName.Container.Len() - 1);
			ClassName.Container.ReplaceCharInline(TCHAR('<'), TCHAR('_'));
			ClassName.String = ClassName.Container;
		}
		ClassName.String.RightChopInline(1);

		// Epilogue
		Epilogue << SL"\tRegister" << ClassName << SL"();\r\n";

		// Registrar beginning
		Code << SL"void Register" << ClassName << SL"()\r\n{\r\n";
		Code << SL"\tusing namespace sketch;\r\n";
	}

	// Widget factory, including named slots
	PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
	// ReSharper disable CppDeclarationHidesLocal
	{
		[[maybe_unused]] FFactory Factory;
		Code
			<< SL"\t" << EXPR(FFactory Factory;) << SL"\r\n"
			<< SL"\t" << EXPR(Factory.Name) << SL" = TEXT(\"" << Class.Name.String.RightChop(1) << SL"\";)\r\n"
			<< SL"\t" << EXPR(Factory.ConstructWidget) << SL" = [](SWidget* WidgetToTakeUniqueSlotsFrom)\r\n"
			<< SL"\t{\r\n";
		if (!Class.NamedSlots.IsEmpty())
		{
			using namespace sketch;
			[[maybe_unused]] TSharedPtr<HeaderTool::FUniqueSlotMeta> Meta;
			Code << SL"\t\t" << EXPR(TSharedRef<HeaderTool::FUniqueSlotMeta> Meta = MakeShared<HeaderTool::FUniqueSlotMeta>();) << SL"\r\n";
			Code << SL"\t\t" << CALL(Meta->Slots.Reserve, AINT(Class.NamedSlots.Num())) << SL";\r\n";
		}
		Code
			<< SL"\t\t" << Class.Name << SL"::FArguments Arguments;\r\n"
			<< SL"\t\tArguments\r\n";
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.WithCustomInitialization()) [[unlikely]]
				continue;

			Code << SL"\t\t\t" << TOpt{ !Property.bSupported, SL"// " };
			Code << SL"." << Property.Name << SL"(" << CALL(Sketch, ASTR(Property.Name), AANY(Property)) << SL")\r\n";
		}
		for (const FSlot& UniqueSlot : Class.NamedSlots)
		{
			[[maybe_unused]] TSharedPtr<HeaderTool::FUniqueSlotMeta> Meta;
			Code
				<< SL"\t\t\t."
				<< UniqueSlot.Name
				<< SL"()[ "
				<< CALL(Meta->AddSlot, APTR(WidgetToTakeUniqueSlotsFrom), ASTR(UniqueSlot.Name))
				<< SL" ]\r\n";
		}
		Code << TOpt{ !Class.NamedSlots.IsEmpty(), SL"\t\t\t.AddMetaData(MoveTemp(Meta))\r\n" };
		Code << SL"\t\t\t.SKETCH_WIDGET_FACTORY_BOILERPLATE();\r\n";
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.CustomArgumentInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << SL"\t\t\t" << Property.CustomArgumentInitializationCode << SL"\r\n";
		}
		Code << SL"\t\tTSharedRef<" << Class.Name << SL"> Widget = SArgumentNew(Arguments, " << Class.Name << SL");\r\n";
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.CustomEntityInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << SL"\t\t\t" << Property.CustomEntityInitializationCode << SL"\r\n";
		}
		Code
			<< SL"\t\treturn MoveTemp(Widget);\r\n"
			<< SL"\t};\r\n";
	}

	// Unique slot enumerator
	if (!Class.NamedSlots.IsEmpty())
	{
		[[maybe_unused]] FFactory Factory;
		Code << SL"\t" << EXPR(Factory.EnumerateUniqueSlots = &HeaderTool::FUniqueSlotMeta::GetSlots;) << SL"\r\n";
	}

	// Dynamic slots
	const bool bHasDynamicSlots = [&]
	{
		if (Class.DynamicSlots.Num() > 1) [[unlikely]]
		{
			// TODO Would be nice to replace LogTemp with something more consistent
			UE_LOG(LogTemp, Error, SL"Multiple types of dynamic slots (%s) aren't currently supported", *Class.Name.String.ToString());
		}
		return Class.DynamicSlots.Num() == 1;
	}();
	if (bHasDynamicSlots)
	{
		// Type enumerator
		[[maybe_unused]] FFactory Factory;
		[[maybe_unused]] sketch::FFactory::FDynamicSlotTypes Result;
		Code
			<< SL"\t" << EXPRT(Factory.EnumerateDynamicSlotTypes = [](const SWidget& Widget)->sketch::FFactory::FDynamicSlotTypes, { return {}; }) << SL"\r\n"
			<< SL"\t{\r\n"
			<< SL"\t\t" << EXPR(sketch::FFactory::FDynamicSlotTypes Result;) << SL"\r\n"
			<< SL"\t\t" << CALL(Result.Reserve, AINT(Class.DynamicSlots.Num())) << SL";\r\n"
			<< SL"\t\tResult = { ";
		for (const FSlot& Slot : Class.DynamicSlots)
		{
			Code << SL"TEXT(\"" << Slot.Type << SL"\"), ";
		}
		Code
			<< SL" };\r\n"
			<< SL"\t\treturn Result;\r\n"
			<< SL"\t};\r\n";

		// Constructor
		Code
			<< SL"\t" << EXPRT(Factory.ConstructDynamicSlot = [](SWidget& Widget, const FName& SlotType) -> ::FSlotBase&, { return *(FSlotBase*)nullptr; }) << SL"\r\n"
			<< SL"\t{\r\n"
			<< SL"\t\tcheck(SlotType == TEXT(\"" << Class.DynamicSlots[0].Type << SL"\"));\r\n"
			<< SL"\t\t" << Class.Name << SL"::" << Class.DynamicSlots[0].Type << SL"* Slot;\r\n"
			<< SL"\t\t{\r\n"
			<< SL"\t\t\tdecltype(auto) SlotArgs = static_cast<" << Class.Name << SL"&>(Widget).AddSlot();\r\n"
			<< SL"\t\t\tSlotArgs\r\n";
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.WithCustomInitialization()) [[unlikely]]
				continue;

			Code << SL"\t\t\t\t";
			Code << TOpt{ !Property.bSupported, SL"// " };
			Code << SL"." << Property.Name << SL"(" << CALL(Sketch, ASTR(Property.Name), AANY(Property)) << SL")\r\n";
		}
		Code
			<< SL"\t\t\t;\r\n";
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.CustomArgumentInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << SL"\t\t\t" << Property.CustomArgumentInitializationCode << SL"\r\n";
		}
		Code
			<< SL"\t\t\tSlot = SlotArgs.GetSlot();\r\n"
			<< SL"\t\t}\r\n";
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.CustomEntityInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << SL"\t\t" << Property.CustomEntityInitializationCode << SL"\r\n";
		}
		Code
			<< SL"\t\treturn *Slot;\r\n"
			<< SL"\t};\r\n";

		// Destructor
		Code << SL"\t" << EXPRT(Factory.DestroyDynamicSlot = &HeaderTool::DestroyDynamicSlot, <SBoxPanel>) << SL"<" << Class.Name << SL">;\r\n";
	}

	// Registrar ending
	::FStringView CategoryView = SL"Unknown";
	{
		::FStringView MayBeCategoryView = File.FilePath;
		int LastSlash = INDEX_NONE;
		if (MayBeCategoryView.FindLastChar(TCHAR('/'), LastSlash)) [[likely]]
		{
			MayBeCategoryView.LeftChopInline(MayBeCategoryView.Len() - LastSlash);
			if (MayBeCategoryView.FindLastChar(TCHAR('/'), LastSlash)) [[likely]]
			{
				MayBeCategoryView.RightChopInline(LastSlash + 1);
				CategoryView = MayBeCategoryView;
			}
		}
	}
	[[maybe_unused]] FSketchCore& Core = *(FSketchCore*)nullptr;
	[[maybe_unused]] FFactory Factory;
	Code << SL"\t" << EXPR(FSketchCore& Core = FSketchCore::Get();) << SL"\r\n";
	Code << SL"\t" << CALL(Core.RegisterFactory, ASTR(CategoryView), AEXP(MoveTemp(Factory))) << SL";\r\n";
	Code << SL"}\r\n";
	Code << SL"\r\n";
	PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
	// ReSharper restore CppDeclarationHidesLocal
}

void sketch::HeaderTool::FReflectionGenerator::Add(const FFile& File, const FString& InclusionRoot)
{
	SetFile(File, InclusionRoot);
	for (const FClass& Class : File.Classes)
	{
		Add(Class);
	}
}

FString sketch::HeaderTool::FReflectionGenerator::GenerateReflection()
{
	FString Result;
	Result.Reserve(Prologue.Len() + Code.Len() + Epilogue.Len() + 256);
	Result
		<< Prologue
		<< SL"\r\n"
		<< SL"#define LOCTEXT_NAMESPACE \"" << SL"Sketch" << SL"\"\r\n"
		<< SL"\r\n"
		<< SL"PRAGMA_DISABLE_DEPRECATION_WARNINGS\r\n"
		<< SL"\r\n"
		<< Code
		<< SL"\r\n"
		<< Epilogue << SL"}\r\n"
		<< SL"\r\n"
		<< SL"PRAGMA_ENABLE_DEPRECATION_WARNINGS\r\n"
		<< SL"\r\n"
		<< SL"#undef LOCTEXT_NAMESPACE\r\n";
	return Result;
}

sketch::FStringView sketch::HeaderTool::TryGetModuleName(sketch::FStringView InclusionRoot, sketch::FStringView FallbackValue)
{
	constexpr sketch::FStringView Public(SL"Public");
	constexpr sketch::FStringView Private(SL"Private");
	auto EndsWith = [&InclusionRoot](sketch::FStringView Suffix)
	{
		const int PrecedingCharIndex = InclusionRoot.Len() - 1 - Suffix.Len();
		return
			InclusionRoot.EndsWith<ESearchCase::IgnoreCase>(Suffix)
			&& InclusionRoot.IsValidIndex(PrecedingCharIndex)
			&& (InclusionRoot[PrecedingCharIndex] == TCHAR('/') || InclusionRoot[PrecedingCharIndex] == TCHAR('\\'));
	};
	if (EndsWith(Public))
	{
		InclusionRoot.LeftChopInline(Public.Len() + 1);
	}
	else if (EndsWith(Private))
	{
		InclusionRoot.LeftChopInline(Private.Len() + 1);
	}
	else if (const int DotPosition = InclusionRoot.FindLast(TCHAR('.')); DotPosition != INDEX_NONE)
	{
		InclusionRoot.LeftInline(DotPosition);
	}
	for (int i = InclusionRoot.Len() - 1; i >= 0; --i)
	{
		if (InclusionRoot[i] == TCHAR('/'))
		{
			return FStringView(&InclusionRoot[i + 1], InclusionRoot.Len() - i - 1);
		}
	}
	return FallbackValue;
}

#pragma endregion ~ Generation

#undef CONCAT

#undef EXPR
#undef EXPRT

#undef CALL_0
#undef CALL_1
#undef CALL_2
#undef CALL_IMPL
#undef CALL

#undef AINT
#undef ASTR
#undef APTR
#undef AANY
#undef AEXP
