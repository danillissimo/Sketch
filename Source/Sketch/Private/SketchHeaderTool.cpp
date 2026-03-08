#include "SketchHeaderTool.h"

#include "SketchHeaderToolTypes.h"
#include "SketchTypes.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Widgets/SSketchWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogSketchHeaderTool, Display, All);

TMap<FName, TArray<sketch::HeaderTool::FHeaderTool::FOverride>> sketch::HeaderTool::FHeaderTool::ClassOverrides = {
	{ TEXT("SThrobber"), { { TEXT("NumPieces"), TEXT("3") } } }, // Default value is private, so nothing to do there except copying it
	{ TEXT("STextComboBox"), { { TEXT("ContentPadding"),TEXT("FCoreStyle::Get().GetWidgetStyle< FComboBoxStyle >(\"ComboBox\").ContentPadding") } } },
	{ TEXT("SScrollBar"), { { TEXT("Padding"), TEXT("SScrollBar::DefaultUniformPadding") } } }
};

#define GET_TYPE_NAME_STRING_CHECKED(Namespace, Class) ((Namespace Class*)nullptr, TEXT(#Class))
#define GET_TYPE_NAME_VIEW_CHECKED(Namespace, Class) ((Namespace Class*)nullptr, FStringView(TEXT(#Class)))

// Engine\Source\Runtime\Slate\Public
// Actually sux:
// - SComboRow as it's a template
// - SComboBox as it's a template
// - SEditableComboBox as it's a template
// - SNumericDropDown as it's a template
// - SNumericEntryBox as it's a template
// - SRichTextHyperlink due to multiple arguments in 'Construct'
// - SNumericRotatorInputBox as it's a template
// - SSegmentedControl as it's a template
// - SSpinBox as it's a template
// - SSubMenuHandler due to multiple arguments in 'Construct'
// - SLinkedBox due to multiple arguments in 'Construct'
// - SResponsiveGridPanel due to multiple arguments in 'Construct'
// - SBreadcrumbTrail as it's a template
// - SScrollBorder due to multiple arguments in 'Construct'
// - SScrollPanel due to multiple arguments in 'Construct'
// - SWindowTitleBar due to multiple arguments in 'Construct'
// - STabDrawer due to multiple arguments in 'Construct'
// - SExpanderArrow due to multiple arguments in 'Construct'
// - SListView as it's a template
// - STableRow as it's a template
// - STileView as it's a template
// - STreeView as it's a template

/*************************************************************************************************/
// Comments //
/*************************************************************************************************/

#pragma region Comments

enum ECommentType
{
	CT_None,
	CT_SingleLine,
	CT_MultiLine
};

ECommentType DetectCommentType(const FStringView& Code, int Position)
{
	if (Code[Position] == TCHAR('/'))[[unlikely]]
	{
		if (Code[Position + 1] == TCHAR('/')) return CT_SingleLine;
		if (Code[Position + 1] == TCHAR('*')) return CT_MultiLine;
	}
	return CT_None;
}

/** @return Index of first character that doesn't belong to a comment, or INDEX_NONE if there's no such */
int FindSingleLineCommentEnding(const FStringView& Code, int Position, const auto& OnNewLine)
{
	int i = Position + 2;
	for (; i < Code.Len() - 1; ++i)
	{
		if (Code[i] == TCHAR('\n'))
		{
			OnNewLine(i);
			if (Code[i - 1] != TCHAR('\\')) [[likely]]
				return i + 1;
		}
	}
	return INDEX_NONE;
}

int FindSingleLineCommentEnding(const FStringView& Code, int Position)
{
	return FindSingleLineCommentEnding(Code, Position, [](int) {});
}

/** @return Index of first character that doesn't belong to a comment, or INDEX_NONE if there's no such */
int FindMultiLineCommentEnding(const FStringView& Code, int Position, const auto& OnNewLine)
{
	int i = Position + 2;
	for (; i < Code.Len() - 1; ++i)
	{
		if (Code[i] == TCHAR('\n'))
			OnNewLine(i);
		if (Code[i] == TCHAR('/') && Code[i - 1] == TCHAR('*'))
			return i + 1;
	}
	return INDEX_NONE;
}

int FindMultiLineCommentEnding(const FStringView& Code, int Position)
{
	return FindMultiLineCommentEnding(Code, Position, [](int) {});
}

/** @return Index of first character that doesn't belong to any comment */
int SkipComments(const FStringView& Code, int Position, const auto& OnNewLine)
{
	// Keep in mind one comment can be followed by another
	for (int i = Position;;)
	{
		const ECommentType CommentType = DetectCommentType(Code, i);
		switch (CommentType)
		{
		case CT_None: return i;
		case CT_SingleLine:
			i = FindSingleLineCommentEnding(Code, i, OnNewLine);
			break;
		case CT_MultiLine:
			i = FindMultiLineCommentEnding(Code, i, OnNewLine);
			break;
		}
		if (i == INDEX_NONE) [[unlikely]]
			return INDEX_NONE;
	}
}

int SkipComments(const FStringView& Code, int Position)
{
	return SkipComments(Code, Position, [](int) {});
}

/** @return Indices of first and last comment characters, or { INDEX_NONE, INDEX_NONE } */
FInt32Interval FindComment(const FStringView& Code, int Position)
{
	for (int i = Position; i < Code.Len() - 1; ++i)
	{
		int CommentEnding = INDEX_NONE;
		const ECommentType CommentType = DetectCommentType(Code, i);
		switch (CommentType)
		{
		case CT_None: continue;
		case CT_SingleLine:
			CommentEnding = FindSingleLineCommentEnding(Code, i);
			break;
		case CT_MultiLine:
			CommentEnding = FindMultiLineCommentEnding(Code, i);
			break;
		}
		return FInt32Interval(i, CommentEnding == INDEX_NONE ? Code.Len() - 1 : CommentEnding - 1);
	}
	return { INDEX_NONE, INDEX_NONE };
}

#pragma endregion ~ Comments

/*************************************************************************************************/
// Brackets //
/*************************************************************************************************/

#pragma region Brackets

int FindPairedBracket(
	const FStringView& Code,
	int OpeningBracketPosition,
	TCHAR OpeningBracket,
	TCHAR ClosingBracket,
	const auto& OnBeginSubscope,
	const auto& OnEndSubscope,
	const auto& OnNewLine
)
{
	check(Code[OpeningBracketPosition] == OpeningBracket);

	int Depth = 1;
	for (int i = OpeningBracketPosition + 1; i < Code.Len() - 1; ++i)
	{
		i = SkipComments(Code, i, OnNewLine);
		if (i == INDEX_NONE) [[unlikely]] return INDEX_NONE;

		if (Code[i] == OpeningBracket)
		{
			OnBeginSubscope(i);
			++Depth;
		}
		else if (Code[i] == ClosingBracket)
		{
			OnEndSubscope(i);
			--Depth;
			if (Depth == 0)
				return i;
		}
		else if (Code[i] == TCHAR('\n'))
		{
			OnNewLine(i);
		}
	}
	return INDEX_NONE;
}

int FindPairedBracket(
	const FStringView& Code,
	int OpeningBracketPosition,
	TCHAR OpeningBracket,
	TCHAR ClosingBracket)
{
	return FindPairedBracket(Code, OpeningBracketPosition, OpeningBracket, ClosingBracket, [](int) {}, [](int) {}, [](int) {});
}

#pragma endregion ~ Brackets

/*************************************************************************************************/
// Navigation //
/*************************************************************************************************/

#pragma region Navigation

int CeilPos(const FStringView& Code, int Pos)
{
	return Pos <= INDEX_NONE ? Code.Len() - 1 : Pos;
}

int OverflowPos(const FStringView& Code, int Pos)
{
	return Pos <= INDEX_NONE ? Code.Len() : Pos;
}

int InvalidatePos(const FStringView& Code, int Pos)
{
	return Code.IsValidIndex(Pos) ? Pos : INDEX_NONE;
}

int FloorPos(int Pos)
{
	return Pos == INDEX_NONE ? 0 : Pos;
}

int FindString(
	const FStringView& Code,
	int InitialPosition,
	const FStringView& String,
	const auto& CustomFilter,
	ESearchCase::Type SearchCase = ESearchCase::CaseSensitive
)
{
	for (int i = InitialPosition; i < Code.Len() - String.Len(); ++i)
	{
		i = SkipComments(FStringView(&Code[0], Code.Len() - String.Len()), i);
		if (i == INDEX_NONE) [[unlikely]] return INDEX_NONE;

		const int AfterFilter = CustomFilter(Code, i);
		if (AfterFilter == INDEX_NONE) return INDEX_NONE;
		if (i != AfterFilter)
		{
			i = AfterFilter - 1;
			continue;
		}

		if (Code.Mid(i, String.Len()).Equals(String, SearchCase))
			return i;
	}
	return INDEX_NONE;
}

int FindString(
	const FStringView& Code,
	int InitialPosition,
	const FStringView& String,
	ESearchCase::Type SearchCase = ESearchCase::CaseSensitive
)
{
	return FindString(Code, InitialPosition, String, [](const FStringView&, int Pos) { return Pos; }, SearchCase);
}

int FindLastString(
	const FStringView& Code,
	int InitialPosition,
	const FStringView& String,
	ESearchCase::Type SearchCase = ESearchCase::CaseSensitive
)
{
	int LastMatch = FindString(Code, InitialPosition, String, SearchCase);
	if (LastMatch == INDEX_NONE) [[unlikely]] return INDEX_NONE;

	for (;;)
	{
		const int NextMatch = FindString(Code, LastMatch + 1, String, SearchCase);
		if (NextMatch == INDEX_NONE) break;
		LastMatch = NextMatch;
	}
	return LastMatch;
}

/**
 * Finds first character that is not an empty space and is not a part of a comment
 * @return INDEX_NONE on failure
 */
int SkipNoneCode(const FStringView& Code, int InitialPosition = 0)
{
	for (int i = InitialPosition; i < Code.Len(); ++i)
	{
		if (TChar<TCHAR>::IsWhitespace(Code[i])) continue;
		if (TChar<TCHAR>::IsControl(Code[i])) continue;


		const int AfterComments = SkipComments(Code, i);
		if (AfterComments == INDEX_NONE) [[unlikely]] return INDEX_NONE;
		if (AfterComments != i)
		{
			i = AfterComments - 1;
			continue;
		}

		return i;
	}
	return INDEX_NONE;
}

/**
 * Assuming InitialPosition points to a beginning of an expression, finds first character that does not
 * belong to this expression
 * @return INDEX_NONE on failure
 */
int SkipExpression(const FStringView& Code, int InitialPosition = 0)
{
	for (int i = InitialPosition; i < Code.Len(); ++i)
	{
		// Check brackets first as IsGraph returns true for them
		constexpr std::array OpeningBrackets = { TCHAR('('), TCHAR('['), TCHAR('{') };
		constexpr std::array ClosingBrackets = { TCHAR(')'), TCHAR(']'), TCHAR('}') };
		for (int j = 0; j < OpeningBrackets.size(); ++j)
		{
			if (Code[i] == OpeningBrackets[j])
			{
				i = FindPairedBracket(Code, i, OpeningBrackets[j], ClosingBrackets[j]);
				if (i == INDEX_NONE) [[unlikely]] return INDEX_NONE;
				goto next_iteration;
			}
		}

		if (TChar<TCHAR>::IsGraph(Code[i])) continue;

		return i;

	next_iteration:
		continue;
	}
	return INDEX_NONE;
}

bool IsAlnum(TCHAR Char) { return TChar<TCHAR>::IsAlnum(Char) || Char == TCHAR('_'); }

int SkipAlnumWord(const FStringView& Code, int InitialPosition = 0)
{
	for (int i = InitialPosition; i < Code.Len(); ++i)
	{
		if (!IsAlnum(Code[i])) return i;
	}
	return INDEX_NONE;
}

bool IsPunct(TCHAR Char)
{
	return Char != TCHAR('_') && TChar<TCHAR>::IsPunct(Char);
};

int SkipPunctWord(const FStringView& Code, int InitialPosition = 0)
{
	auto IsAlwaysAlone = [](TCHAR Char)
	{
		constexpr auto AlwaysAlone = {
			TCHAR('('), TCHAR(')'),
			TCHAR('['), TCHAR(']'),
			TCHAR('{'), TCHAR('}'),
			TCHAR(','),
			TCHAR('?'),
			TCHAR('.'),
			TCHAR(';')
		};
		for (const TCHAR AlwaysAloneChar : AlwaysAlone)
		{
			if (Char == AlwaysAloneChar) return true;
		}
		return false;
	};

	if (IsAlwaysAlone(Code[InitialPosition]))
	{
		return Code.IsValidIndex(InitialPosition + 1) ? InitialPosition + 1 : INDEX_NONE;
	}
	for (int i = InitialPosition; i < Code.Len(); ++i)
	{
		if (!IsPunct(Code[i]) || IsAlwaysAlone(Code[i])) return i;
	}
	return INDEX_NONE;
}

int SkipAnyWord(const FStringView& Code, int InitialPosition = 0)
{
	if (IsAlnum(Code[InitialPosition])) return SkipAlnumWord(Code, InitialPosition);
	check(IsPunct(Code[InitialPosition]));
	return SkipPunctWord(Code, InitialPosition);
}

int FindFirstCharacter(const FStringView& Code, int Position)
{
	for (int i = Position; i < Code.Len(); ++i)
		if (!TChar<TCHAR>::IsWhitespace(Code[i])) return i;
	return INDEX_NONE;
}

int FindFirstCharacterBackwards(const FStringView& Code, int Position)
{
	for (int i = Position; i >= 0; --i)
		if (!TChar<TCHAR>::IsWhitespace(Code[i])) return i;
	return INDEX_NONE;
}

bool IsDirective(const FStringView& Code, int Position)
{
	return Code[Position] == TCHAR('#');
}

int SkipDirective(const FStringView& Code, int Position, const auto& OnNewLine)
{
	check(IsDirective(Code, Position));
	for (int i = Position + 1; i < Code.Len() - 1; ++i)
	{
		if (Code[i] == TCHAR('\n'))
		{
			OnNewLine(i);
			if (Code[i - 1] != TCHAR('\\'))
				return i + 1;
		}
	}
	return INDEX_NONE;
}

struct FClassName
{
	FStringView Name;
	bool bTemplate = false;
};

FClassName FindClassName(const FString& Code, const sketch::HeaderTool::FIndex& Index, int ClassScopeIndex)
{
	// Class name is within scope, containing class scope
	// It follows last "class" instance within outer scope
	// And is not surrounded by any kind of parenthesis
	// Has to begin from outer scope beginning, instead of previous scope ending
	// Because previous scope can actually be a lambda or an initializer in a template
	using namespace sketch::HeaderTool;

	// First, determine part of code containing class name
	// It begins after outer scope beginning, and ends at class scope start
	int OuterScopeIndex = INDEX_NONE;
	for (int i = ClassScopeIndex - 1; i >= 0; --i)
	{
		if (Index.Scopes[i].Contains(Index.Scopes[ClassScopeIndex]))
		{
			OuterScopeIndex = i;
			break;
		}
	}
	FScope GlobalScope{ -1, Code.Len() };
	const FScope& OuterScope = OuterScopeIndex == INDEX_NONE ? GlobalScope : Index.Scopes[OuterScopeIndex];
	FStringView WideRegion(&Code[OuterScope.Start + 1], Index.Scopes[ClassScopeIndex].Start - 1 - OuterScope.Start - 1);

	// Now traverse containing code skipping ANY parenthesis on the way
	// And remember each last "class" instance
	constexpr FStringView Class(TEXT("class"));
	int LastClassInstance = INDEX_NONE;
	bool bTemplate = false;
	for (int i = 0; i < WideRegion.Len();)
	{
		// Skip comments
		const ECommentType CommentType = DetectCommentType(WideRegion, i);
		if (CommentType != CT_None) [[unlikely]]
		{
			if (CommentType == CT_SingleLine)
				i = FindSingleLineCommentEnding(WideRegion, i);
			else
				i = FindMultiLineCommentEnding(WideRegion, i);
			if (i == INDEX_NONE) [[unlikely]] break;
			continue;
		}

		// Skip empty spaces
		const int AfterSpaces = OverflowPos(WideRegion, FindFirstCharacter(WideRegion, i));
		if (AfterSpaces != i)
		{
			i = AfterSpaces;
			continue;
		}

		// Check if this a "class" instance
		{
			FStringView ConsideredCode = WideRegion.Mid(i, Class.Len());
			if (ConsideredCode.Equals(Class, ESearchCase::CaseSensitive)) [[unlikely]]
			{
				LastClassInstance = i;
				i += Class.Len();
				continue;
			}
		}

		// Check if this a "template" instance
		{
			constexpr FStringView Template(TEXT("template"));
			FStringView ConsideredCode = WideRegion.Mid(i, Template.Len());
			if (ConsideredCode.Equals(Template, ESearchCase::CaseSensitive)) [[unlikely]]
			{
				bTemplate = true;
				i += Template.Len();
				continue;
			}
		}

		// Right now we are pointing to something valuable - not a whitespace, not a comment
		// Not a "class" instance either
		// Skip it whatever it is, and any of its subscopes
		i = SkipExpression(WideRegion, i);
		if (i == INDEX_NONE) [[unlikely]] break;

		// Consider template expression ending
		const bool bIsScopeBeginning = WideRegion[i] == TCHAR('{');
		const bool bIsSemicolon = i > 0 && WideRegion[i - 1] == TCHAR(';');
		if (bTemplate && (bIsScopeBeginning || bIsSemicolon)) [[unlikely]]
			bTemplate = false;
	}
	if (LastClassInstance == INDEX_NONE) [[unlikely]] return {};

	// Now it looks like:
	// class alignas(...) SOME_API ClassName
	// Note that both alignas and API are optional and are unlikely to be present
	// Also note that they are allowed to repeat multiple times
	// Also note comments may be present even there
	FStringView NarrowRegion = WideRegion.RightChop(LastClassInstance + Class.Len() + 1);
	for (int i = 0; i < NarrowRegion.Len(); ++i)
	{
		// Skip comments
		const ECommentType CommentType = DetectCommentType(NarrowRegion, i);
		if (CommentType != CT_None) [[unlikely]]
		{
			if (CommentType == CT_SingleLine)
				i = FindSingleLineCommentEnding(NarrowRegion, i);
			else
				i = FindMultiLineCommentEnding(NarrowRegion, i);
			if (i == INDEX_NONE) [[unlikely]] break;
			continue;
		}

		// Skip empty spaces
		const int AfterSpaces = CeilPos(NarrowRegion, FindFirstCharacter(NarrowRegion, i));
		if (AfterSpaces != i)
		{
			i = AfterSpaces;
			continue;
		}

		// Right now we are pointing to something valuable - not a whitespace, not a comment
		// It's one of API macro, some specifier (like alignas) or a macro with arguments, or the class name
		// API macro ends, obviously, end with API
		// Specifiers and macros are followed by a "("
		const int WordEnding = SkipExpression(NarrowRegion, i);
		if (i == INDEX_NONE) [[unlikely]] break;
		FStringView Word = NarrowRegion.Mid(i, WordEnding - i);
		const bool bApiMacro = Word.EndsWith(TEXT("_API"), ESearchCase::CaseSensitive);
		const bool bSpecifierOrMacro = Word.EndsWith(TEXT(")"), ESearchCase::CaseSensitive);
		if (bApiMacro || bSpecifierOrMacro) [[unlikely]]
		{
			i = WordEnding;
			continue;
		}

		// Peek following word
		// Specifier arguments are processed by previous block if there's no spaces between specifier
		// and its arguments. This block handles cases when there is some space.
		// Note that there might be NO following word
		// This means Word is not a specifier
		const int NextWord = SkipNoneCode(NarrowRegion, WordEnding + 1);
		if (NextWord != INDEX_NONE) [[unlikely]]
		{
			if (NarrowRegion[NextWord] == TCHAR('(')) [[unlikely]]
			{
				i = SkipExpression(NarrowRegion, NextWord);
				if (i == INDEX_NONE) [[unlikely]] break;
				continue;
			}
		}

		// Holy ducks, we've made it
		if (Word.Right(1) == TEXT(":"))
			Word.RemoveSuffix(1);
		return { Word, bTemplate };
	}
	return {};
}

namespace Sequence
{
	enum ESegmentType
	{
		ST_Required,
		ST_Optional,
		ST_MustNotBePresent
	};

	template <class T>
	concept CMatcher = requires { std::declval<TOptional<int>>() = std::declval<T>()(FStringView(), 0); };

	template <CMatcher T>
	struct TSegment
	{
		/** Returns { LastWordCharacterIndex } on match, { INDEX_NONE } on mismatch, {} when matched expression begun, but didn't end in given scope */
		T Matcher;
		ESegmentType Type = ST_Required;
	};

	template <class>
	struct TIsSegment
	{
		enum { Value = false };
	};

	template <class T>
	struct TIsSegment<TSegment<T>>
	{
		enum { Value = true };
	};

	template <class T>
	concept CSegment = bool(TIsSegment<T>::Value);

	template <CMatcher T>
	TSegment<T> Make(T&& Matcher, ESegmentType Type = ST_Required) { return TSegment<T>{ MoveTemp(Matcher), Type }; }

	enum EStringType
	{
		ST_Any,
		ST_Alnum,
		ST_Punct
	};

	template <EStringType StringType = ST_Any>
	auto String(const FStringView& String, ESegmentType Type = ST_Required)
	{
		check(!String.IsEmpty())
		auto Handler = [String](const FStringView& Code, int Position)-> TOptional<int>
		{
			const int WordEnding =
				StringType == ST_Any
					? SkipAnyWord(Code, Position)
					: StringType == ST_Alnum
					? SkipAlnumWord(Code, Position)
					: SkipPunctWord(Code, Position);
			FStringView Word = WordEnding == INDEX_NONE ? Code.RightChop(Position) : Code.Mid(Position, WordEnding - Position);
			if (Word.Equals(String, ESearchCase::CaseSensitive))
				return CeilPos(Code, Position + String.Len() - 1);
			return INDEX_NONE;
		};
		return Make(MoveTemp(Handler), Type);
	}

	auto OneOf(const std::initializer_list<FStringView>& Strings, ESegmentType Type = ST_Required)
	{
		auto Handler = [Strings](const FStringView& Code, int Position)-> TOptional<int>
		{
			const int WordEnding = SkipAnyWord(Code, Position);
			FStringView Word = WordEnding == INDEX_NONE ? Code.RightChop(Position) : Code.Mid(Position, WordEnding - Position);
			for (const auto& String : Strings)
			{
				if (Word.Equals(String, ESearchCase::CaseSensitive))
					return CeilPos(Code, Position + String.Len() - 1);
			}
			return INDEX_NONE;
		};
		return Make(MoveTemp(Handler), Type);
	}

	/** Any_validName */
	auto AlnumWord(ESegmentType Type = ST_Required)
	{
		auto Handler = [](const FStringView& Code, int Position)-> TOptional<int>
		{
			if (!IsAlnum(Code[Position]))
				return INDEX_NONE;
			const int Result = SkipAlnumWord(Code, Position);
			return Result == INDEX_NONE ? Code.Len() - 1 : Result - 1;
		};
		return Make(MoveTemp(Handler), Type);
	}

	/** Any_validName_API */
	auto ModuleApi(ESegmentType Type = ST_Optional)
	{
		auto Handler = [](const FStringView& Code, int Position)-> TOptional<int>
		{
			const int WordEnding = SkipAnyWord(Code, Position);
			FStringView Word = WordEnding == INDEX_NONE ? Code.RightChop(Position) : Code.Mid(Position, WordEnding - Position);
			if (Word.EndsWith(TEXT("_API"), ESearchCase::CaseSensitive))
				return WordEnding == INDEX_NONE ? Code.Len() - 1 : WordEnding - 1;
			return INDEX_NONE;
		};
		return Make(MoveTemp(Handler), Type);
	}

	/** template<...> */
	/** Can be replaced with Sequence(String, Subscope) */
	// auto TemplateDeclaration(ESegmentType Type = ST_Required)
	// {
	// 	auto Handler = [](const FStringView& Code, int Position)-> TOptional<int>
	// 	{
	// 		const int WordEnding = SkipAlnumWord(Code, Position);
	// 		if (WordEnding == Position) return INDEX_NONE;
	// 		FStringView Word = WordEnding == INDEX_NONE ? Code.RightChop(Position) : Code.Mid(Position, WordEnding - Position);
	// 		constexpr FStringView Template(TEXT("template"));
	// 		if (Word.Equals(Template, ESearchCase::CaseSensitive))
	// 		{
	// 			const int NextWordStart = SkipNoneCode(Code, Position + Template.Len());
	// 			if (NextWordStart == INDEX_NONE) [[unlikely]] return {};
	// 			const int NextWordEnd = SkipAnyWord(Code, NextWordStart);
	// 			if (NextWordEnd == INDEX_NONE) [[unlikely]] return {};
	// 			FStringView NextWord = Code.Mid(NextWordStart, NextWordEnd - NextWordStart);
	// 			if (!NextWord.Equals(TEXT("<"), ESearchCase::CaseSensitive)) return INDEX_NONE;
	// 			int ClosingBracket = FindPairedBracket(Code, NextWordStart, TCHAR('<'), TCHAR('>'));
	// 			if (ClosingBracket == INDEX_NONE) [[unlikely]] return {};
	// 			return ClosingBracket;
	// 		}
	// 		return INDEX_NONE;
	// 	};
	// 	return Make(MoveTemp(Handler), Type);
	// }

	template <TCHAR OpeningBracket, TCHAR ClosingBracket>
	auto Subscope(ESegmentType Type = ST_Required)
	{
		auto Handler = [](const FStringView& Code, int Position)-> TOptional<int>
		{
			if (Code[Position] != OpeningBracket) return INDEX_NONE;
			const int PairedBracket = FindPairedBracket(Code, Position, OpeningBracket, ClosingBracket);
			if (ClosingBracket == INDEX_NONE) [[unlikely]] return {};
			return PairedBracket;
		};
		return Make(MoveTemp(Handler), Type);
	}

	template <class T>
	concept CFilter = requires { std::declval<T>()(FStringView(), 0); };

	auto NoFilter = [](const FStringView&, int Pos) { return Pos; };

	template <TCHAR OpeningBracket = TCHAR('{'), TCHAR ClosingBracket = TCHAR('}')>
	auto SubscopeFilter = [](const FStringView& Code, int Pos)
	{
		if (Code[Pos] == OpeningBracket)
		{
			const int PairedBracket = FindPairedBracket(Code, Pos, OpeningBracket, ClosingBracket);
			return Code.IsValidIndex(PairedBracket + 1) ? PairedBracket + 1 : INDEX_NONE;
		}
		return Pos;
	};

	template <CSegment... T>
	TVariant<int, std::array<FStringView, sizeof...(T)>> Match(
		const FStringView& Code,
		const TTuple<T...>& Sequence,
		const CFilter auto& CustomFilter,
		int InitialPosition
	)
	{
		auto Call = [&](int i, int WordBeginning)-> TOptional<int>
		{
			return [&]<size_t... Indices>(std::index_sequence<Indices...>)
			{
				check(i < sizeof...(T));
				TOptional<int> Result;
				((i == Indices ? void(Result = Sequence.template Get<Indices>().Matcher(Code, WordBeginning)) : void()), ...);
				return Result;
			}(std::make_index_sequence<sizeof...(T)>{});
		};
		auto GetType = [&](int i)-> ESegmentType
		{
			return [&]<size_t... Indices>(std::index_sequence<Indices...>)
			{
				check(i < sizeof...(T));
				ESegmentType Result;
				((i == Indices ? void(Result = Sequence.template Get<Indices>().Type) : void()), ...);
				return Result;
			}(std::make_index_sequence<sizeof...(T)>{});
		};

		using FResult = TVariant<int, std::array<FStringView, sizeof...(T)>>;
		auto Int = [](int Value) { return FResult{ TInPlaceType<int>{}, Value }; };
		std::array<FStringView, sizeof...(T)> Result;
		auto Array = [&Result] { return FResult{ TInPlaceType<std::array<FStringView, sizeof...(T)>>{}, MoveTemp(Result) }; };

		if (Code.IsEmpty()) [[unlikely]] return Int(INDEX_NONE);

		int LookingForComponent = 0;
		bool bHandledAnyRequirements = false;
		bool bFullStop = false;
		for (int i = InitialPosition; i < Code.Len();)
		{
			// Skip white spaces and comments
			const int WordBeginning = SkipNoneCode(Code, i);
			if (WordBeginning == INDEX_NONE) [[unlikely]]
			{
				bFullStop = true;
				break;
			}

			// Apply custom filter
			{
				const int AfterFilter = CustomFilter(Code, WordBeginning);
				if (AfterFilter == INDEX_NONE) [[unlikely]]
				{
					bFullStop = true;
					break;
				}
				if (AfterFilter != WordBeginning)
				{
					i = AfterFilter;
					continue;
				}
			}

			// Examine word
			// Match it against each pending segment
			// Optional segments are allowed to mismatch
			// Undesired segments are expected to mismatch
			uint8 bMatch = 123;
			FStringView Word;
			int Component = LookingForComponent;
			for (; Component < sizeof...(T); ++Component)
			{
				const TOptional<int> MayBeWordEnding = Call(Component, WordBeginning);
				if (!MayBeWordEnding) [[unlikely]] break;
				int WordEnding = *MayBeWordEnding;
				if (WordEnding != INDEX_NONE)
				{
					bMatch = true;
					Word = Code.Mid(WordBeginning, WordEnding - WordBeginning + 1);
				}
				else
				{
					bMatch = false;
				}

				// Stop on any result except:
				// - Mismatch but optional
				// - Mismatch but undesired
				const bool bException = !bMatch && GetType(Component) != ST_Required;
				if (!bException) break;
			}
			check(bMatch == 0 || bMatch == 1)

			// Return first untested character index on any undesired events
			if (!bMatch)
			{
				// Special case for the last optional
				if ((Component == sizeof...(T)) && Sequence.template Get<sizeof...(T) - 1>().Type != ST_Required) [[unlikely]]
					return Array();

				return Int(InvalidatePos(Code, WordBeginning + int(!bHandledAnyRequirements)));
			}
			if (GetType(Component) == ST_MustNotBePresent)
			{
				const int LastCharIndex = &Word[Word.Len() - 1] - &Code[0];
				return Int(InvalidatePos(Code, LastCharIndex + 1));
			}

			// Cache match and advance
			Result[Component] = Word;
			LookingForComponent = Component + 1;
			if (LookingForComponent >= sizeof...(T))
			{
				return Array();
			}
			i = &Word[Word.Len() - 1] - &Code[0] + 1;
			bHandledAnyRequirements |= GetType(Component) == ST_Required;
		}

		// Make sure all requirements were fulfilled
		for (int i = LookingForComponent; i < sizeof...(T); ++i)
		{
			if (GetType(i) == ST_Required)
			{
				int LastMatchedComponent = INDEX_NONE;
				for (int j = LookingForComponent - 1; j >= 0; --j)
				{
					if (!Result[j].IsEmpty())
					{
						LastMatchedComponent = j;
						break;
					}
				}
				if (LastMatchedComponent == INDEX_NONE)
				{
					check(bFullStop);
					return Int(INDEX_NONE);
				}
				const FStringView& LastMatch = Result[LastMatchedComponent];
				const int LastMatchedChar = &LastMatch[LastMatch.Len() - 1] - &Code[0];
				return Int(InvalidatePos(Code, LastMatchedChar + 1));
			}
		}
		return Array();
	}

	template <CSegment... T>
	TOptional<std::array<FStringView, sizeof...(T)>> Find(
		const FStringView& Code,
		const TTuple<T...>& Sequence,
		const CFilter auto& CustomFilter,
		int InitialPosition
	)
	{
		TVariant<int, std::array<FStringView, sizeof...(T)>> Result;
		for (int Position = InitialPosition; Position != INDEX_NONE; Position = Result.template Get<int>())
		{
			Result = Match(Code, Sequence, CustomFilter, Position);
			if (auto* Array = Result.template TryGet<std::array<FStringView, sizeof...(T)>>())
				return MoveTemp(*Array);
		}
		return {};
	}

	template <CSegment... T>
	FORCEINLINE TOptional<std::array<FStringView, sizeof...(T)>> Find(
		const FStringView& Code,
		T&&... Segments
	)
	{
		return Find(Code, MakeTuple(MoveTemp(Segments)...), NoFilter, 0);
	}

	template <CSegment... T>
	FORCEINLINE TOptional<std::array<FStringView, sizeof...(T)>> Find(
		const FStringView& Code,
		int InitialPosition,
		T&&... Segments
	)
	{
		return Find(Code, MakeTuple(MoveTemp(Segments)...), NoFilter, InitialPosition);
	}

	template <CSegment... T>
	FORCEINLINE TOptional<std::array<FStringView, sizeof...(T)>> Find(
		const FStringView& Code,
		const CFilter auto& CustomFilter,
		T&&... Segments
	)
	{
		return Find(Code, MakeTuple(MoveTemp(Segments)...), CustomFilter, 0);
	}

	template <CSegment... T>
	FORCEINLINE TOptional<std::array<FStringView, sizeof...(T)>> Find(
		const FStringView& Code,
		int InitialPosition,
		const CFilter auto& CustomFilter,
		T&&... Segments
	)
	{
		return Find(Code, MakeTuple(MoveTemp(Segments)...), CustomFilter, InitialPosition);
	}

	template <CSegment... T>
	auto OneOf(ESegmentType Type, T&&... Segments)
	{
		auto Handler = [Segments...](const FStringView& Code, int Position)-> TOptional<int>
		{
			bool bContinue = true;
			FStringView Match;
			auto Next = [&](auto& Segment)
			{
				TOptional<int> Result = Segment.Matcher(Code, Position);
				if (Result && *Result != INDEX_NONE)
				{
					bContinue = false;
					Match = Code.Mid(Position, *Result - Position + 1);
				}
			};
			((bContinue ? Next(Segments) : void()), ...);
			return Match.IsEmpty() ? INDEX_NONE : &Match[Match.Len() - 1] - &Code[0];
		};
		return Make(MoveTemp(Handler), Type);
	}

	template <CSegment... T>
	auto OneOf(T&&... Segments) { return OneOf(ST_Required, MoveTemp(Segments)...); };

	template <CFilter FilterType = decltype(NoFilter), int Size = 1>
	struct TSubsequenceOptions
	{
		std::array<FStringView, Size>* Result = nullptr;
		FilterType Filter;
		ESegmentType Type = ST_Required;
	};

	template <CSegment... T, CFilter FilterType>
	auto Subsequence(TSubsequenceOptions<FilterType, sizeof...(T)> Options, T&&... Segments)
	{
		auto Handler = [Tup = MakeTuple(MoveTemp(Segments)...), F = MoveTemp(Options.Filter), Out = Options.Result](const FStringView& Code, int Position)-> TOptional<int>
		{
			TVariant<int, std::array<FStringView, sizeof...(T)>> Result = Match(Code, Tup, F, Position);
			if (const auto* ArrayPtr = Result.template TryGet<std::array<FStringView, sizeof...(T)>>())
			{
				const auto& Array = *ArrayPtr;
				for (int i = Array.size() - 1; i >= 0; --i)
				{
					if (!Array[i].IsEmpty())
					{
						if (Out) *Out = Array;
						return &Array[i][Array[i].Len() - 1] - &Code[0];
					}
				}
				checkNoEntry();
			}
			return INDEX_NONE;
		};
		return Make(MoveTemp(Handler), Options.Type);
	}

	template <CSegment... T>
	auto Subsequence(ESegmentType Type, CFilter auto&& Filter, T&&... Segments)
	{
		auto Handler = [Tup = MakeTuple(MoveTemp(Segments)...), F = MoveTemp(Filter)](const FStringView& Code, int Position)-> TOptional<int>
		{
			TVariant<int, std::array<FStringView, sizeof...(T)>> Result = Match(Code, Tup, F, Position);
			if (const auto* ArrayPtr = Result.template TryGet<std::array<FStringView, sizeof...(T)>>())
			{
				const auto& Array = *ArrayPtr;
				for (int i = Array.size() - 1; i >= 0; --i)
				{
					if (!Array[i].IsEmpty())
					{
						return &Array[i][Array[i].Len() - 1] - &Code[0];
					}
				}
				checkNoEntry();
			}
			return INDEX_NONE;
		};
		return Make(MoveTemp(Handler), Type);
	}

	template <CSegment... T>
	auto Subsequence(T&&... Segments) { return Subsequence(ST_Required, NoFilter, MoveTemp(Segments)...); }

	template <CSegment... T>
	auto Subsequence(ESegmentType Type, T&&... Segments) { return Subsequence(Type, NoFilter, MoveTemp(Segments)...); }

	template <CSegment... T>
	auto Subsequence(CFilter auto&& Filter, T&&... Segments) { return Subsequence(ST_Required, MoveTemp(Filter), MoveTemp(Segments)...); }
};



bool ContainsSlotConstructor(const FStringView& ClassBody, const FStringView& SlotName)
{
	// Locate "static [MODULE_API] SlotName::FSlotArguments Slot()"
	// Where [] are optional
	std::array Components = {
		FStringView{ TEXT("static") },
		SlotName,
		FStringView(TEXT("::")),
		FStringView(TEXT("FSlotArguments")),
		FStringView(TEXT("Slot")),
		FStringView(TEXT("(")),
		FStringView(TEXT(")")),
	};
	int LookingForComponent = 0;
	for (int i = 0; i < ClassBody.Len();)
	{
		// Skip white spaces and comments
		const int WordBeginning = SkipNoneCode(ClassBody, i);
		if (WordBeginning == INDEX_NONE) [[unlikely]] return false;

		// Do not enter subscopes
		if (ClassBody[WordBeginning] == TCHAR('{'))
		{
			int PairedBracket = FindPairedBracket(ClassBody, WordBeginning, TCHAR('{'), TCHAR('}'));
			if (PairedBracket == INDEX_NONE) [[unlikely]] return false;
			i = PairedBracket + 1;
			continue;
		}

		// Find word ending
		const int WordEnding = SkipAnyWord(ClassBody, WordBeginning);
		if (WordEnding == INDEX_NONE) [[unlikely]] return false;

		// Skip API macro
		const FStringView Word = ClassBody.Mid(WordBeginning, WordEnding - WordBeginning);
		if (!Word.EndsWith(TEXT("_API"), ESearchCase::CaseSensitive))[[likely]]
		{
			// Check if this is another word we are looking for
			if (Word.Equals(Components[LookingForComponent], ESearchCase::CaseSensitive))
			{
				++LookingForComponent;
				if (LookingForComponent == Components.size())
					return true;
			}
			else
			{
				LookingForComponent = 0;
			}
		}
		i = WordEnding;
	}
	return false;
}

#pragma endregion ~ Navigation

/*************************************************************************************************/
// Utility //
/*************************************************************************************************/

#pragma region Utility

/** Removes any comments, line brakes, trims whitespaces */
sketch::HeaderTool::FProcessedString CleanCode(const FStringView& Code)
{
	// Locate all comments
	TArray<FInt32Interval, TInlineAllocator<8>> Comments;
	FInt32Interval Comment = FindComment(Code, 0);
	if (Comment.Min == INDEX_NONE) [[likely]]
		return sketch::HeaderTool::FProcessedString{ Code.TrimStartAndEnd() };
	for (;;)
	{
		Comment.Min = FindFirstCharacterBackwards(Code, Comment.Min - 1);
		Comment.Max = FindFirstCharacter(Code, Comment.Max + 1);
		Comment.Min = Comment.Min == INDEX_NONE ? 0 : Comment.Min + 1;
		Comment.Max = Comment.Max == INDEX_NONE ? Code.Len() - 1 : Comment.Max - 1;
		Comments.Emplace(Comment);

		Comment = FindComment(Code, Comments.Last().Max + 1);
		if (Comment.Min == INDEX_NONE)
			break;
	}

	// Search for non-empty views between comments
	// If there's a single view - it's the result
	// If there are multiple views - copy all of them into a new string
	// Yes, result may turn out to be empty
	FStringView ValuableView(&Code[0], Comments[0].Min);
	for (int i = 1; i < Comments.Num(); ++i)
	{
		FStringView CandidateView = Code.Mid(Comments[i - 1].Max + 1, Comments[i].Min - Comments[i - 1].Max - 1);
		if (ValuableView.IsEmpty())
		{
			ValuableView = CandidateView;
			continue;
		}
		if (CandidateView.Len() > 0)
		{
			sketch::HeaderTool::FProcessedString Result;
			Result.Container.Reserve(Code.Len());
			Result.Container.Append(ValuableView);
			Result.Container.Append(CandidateView);
			for (++i; i < Comments.Num(); ++i)
			{
				const int PayloadLength = Comments[i].Min - Comments[i - 1].Max - 1;
				if (PayloadLength > 0)
				{
					ValuableView = FStringView(&Code[Comments[i - 1].Max] + 1, PayloadLength);
					Result.Container.Append(ValuableView);
				}
			}
			Result.String = Result.Container;
			return Result;
		}
	}
	return { ValuableView };
}

TArray<sketch::HeaderTool::FProperty> ParseArguments(const FStringView& Arguments)
{
	TArray<sketch::HeaderTool::FProperty> Result;
	int NextArgStart = 0;
	for (;;)
	{
		// Match next argument
		using namespace Sequence;
		std::array<FStringView, 4> DefaultValue;
		auto MatchResult = Match(
			Arguments,
			MakeTuple(
				// Argument type
				AlnumWord(),
				Subscope<TCHAR('<'), TCHAR('>')>(ST_Optional),
				Subscope<TCHAR('('), TCHAR(')')>(ST_Optional),
				// Argument name
				AlnumWord(),
				// Default argument value
				Subsequence(
					TSubsequenceOptions{ .Result = &DefaultValue, .Type = ST_Optional },
					String<ST_Punct>(TEXT("=")),
					AlnumWord(),
					Subscope<TCHAR('<'), TCHAR('>')>(ST_Optional),
					Subscope<TCHAR('('), TCHAR(')')>(ST_Optional)
				),
				// Potential comma before next argument
				String<ST_Punct>(TEXT(","), ST_Optional)
			),
			NoFilter,
			NextArgStart
		);
		if (MatchResult.IsType<int>()) break;

		// Gather argument data
		const auto& Data = MatchResult.Get<std::array<FStringView, 6>>();
		sketch::HeaderTool::FProperty& Arg = Result.Emplace_GetRef();
		Arg.Name = Data[3];
		if (Data[1].IsEmpty() && Data[2].IsEmpty())
		{
			Arg.Type.String = Data[0];
		}
		else
		{
			const TCHAR* FirstCharPtr = &Data[0][0];
			const int LastCharOwner = Data[2].IsEmpty() ? 1 : 2;
			const TCHAR* LastCharPtr = &Data[LastCharOwner][Data[LastCharOwner].Len() - 1];
			FStringView Type = FStringView(FirstCharPtr, LastCharPtr - FirstCharPtr + 1);
			Arg.Type = CleanCode(Type);
		}
		if (!Data[4].IsEmpty())
		{
			if (DefaultValue[2].IsEmpty() && DefaultValue[3].IsEmpty())
			{
				Arg.DefaultValue = sketch::HeaderTool::FProcessedString{ .String = DefaultValue[1] };
			}
			else
			{
				const TCHAR* FirstCharPtr = &DefaultValue[1][0];
				const int LastCharOwner = DefaultValue[3].IsEmpty() ? 2 : 3;
				const TCHAR* LastCharPtr = &DefaultValue[LastCharOwner][DefaultValue[LastCharOwner].Len() - 1];
				FStringView Value = FStringView(FirstCharPtr, LastCharPtr - FirstCharPtr + 1);
				Arg.DefaultValue = CleanCode(Value);
			}
		}

		// Break if there are no more arguments
		if (Data.back().IsEmpty()) break;

		// Update argument position
		NextArgStart = &Data.back()[0] - &Arguments[0];
	}
	return Result;
}

#pragma endregion ~ Utility

/*************************************************************************************************/
//  //
/*************************************************************************************************/

using namespace sketch::HeaderTool;

TArray<FFile> FHeaderTool::Scan(const FString& Path, bool bRecursive)
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

	TSet<FName> SupportedTypes = {
		TEXT("const FSlateBrush*"),
		TEXT("FLinearColor"),
		TEXT("FSlateColor"),
		TEXT("FSlateFontInfo"),
		TEXT("bool"),
		TEXT("float"),
		TEXT("double"),
		TEXT("int"),
		TEXT("int32"),
		TEXT("FMargin"),
		TEXT("FOptionalSize"),
		TEXT("FText")
	};
	TArray<FFile> Result;
	Result.Reserve(Files.Num());
	for (auto& File : Files)
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
		TArray<FClass> Classes;
		if (ProcessCode(Code, SupportedTypes, Classes) && !Classes.IsEmpty())
		{
			Result.Emplace(MoveTemp(File), MoveTemp(Code), MoveTemp(Classes));
		}
	}
	return Result;
}

FFile FHeaderTool::Scan(const FString& FilePath)
{
	TSet<FName> SupportedTypes = {
		TEXT("const FSlateBrush*"),
		TEXT("FLinearColor"),
		TEXT("FSlateColor"),
		TEXT("FSlateFontInfo"),
		TEXT("bool"),
		TEXT("float"),
		TEXT("double"),
		TEXT("int"),
		TEXT("int32"),
		TEXT("FMargin"),
		TEXT("FOptionalSize"),
		TEXT("FText")
	};

	FString Code;
	FFileHelper::LoadFileToString(Code, *FilePath);
	TArray<FClass> Classes;
	if (ProcessCode(Code, SupportedTypes, Classes) && !Classes.IsEmpty())
	{
		return { FilePath, MoveTemp(Code), MoveTemp(Classes) };
	}
	return {};
}

FString& operator<<(FString& A, const TCHAR* B) { return A.Append(B); }
FString& operator<<(FString& A, const FStringView& B) { return A.Append(B); }
FString& operator<<(FString& A, const FProcessedString& B) { return A << B.String; }
FString& operator<<(FString& A, const TOptional<FProcessedString>& B) { return B.IsSet() ? A << B.GetValue().String : A; }
FString& operator<<(FString& A, int B) { return A.AppendInt(B), A; }

struct FDefaultValueHint
{
	const sketch::HeaderTool::FProperty& Property;
};

FDefaultValueHint MakeDefaultValue(const sketch::HeaderTool::FProperty& Property) { return { Property }; }

FString& operator<<(FString& A, const FDefaultValueHint& B)
{
	const bool bPointerType = B.Property.Type.String.EndsWith(TCHAR('*'));
	if (bPointerType)
		A << TEXT("(");
	A << B.Property.Type;
	if (bPointerType)
		A << TEXT(")");

	if (bPointerType && (!B.Property.DefaultValue.IsSet() || B.Property.DefaultValue->String.IsEmpty())) [[unlikely]]
		A << TEXT(" nullptr ");
	else
		A << TEXT("( ") << B.Property.DefaultValue << TEXT(" )");
	return A;
}

FString FHeaderTool::GenerateReflectionPrologue()
{
	FString Prologue;
	Prologue << TEXT("// Generated by Sketch Header Tool on ") << FDateTime::Now().ToString() << TEXT("\r\n");
	Prologue << TEXT("#include \"Sketch.h\"\r\n");
	Prologue << TEXT("#include \"Sketch/Public/Widgets/SSketchWidget.h\"\r\n");
	Prologue << TEXT("#include \"SketchHeaderToolTypes.h\"\r\n");
	Prologue << TEXT("\r\n");
	return Prologue;
}

FString FHeaderTool::GenerateReflectionEpilogue()
{
	FString Epilogue;
	Epilogue
		<< TEXT("void RegisterAll()\r\n")
		<< TEXT("{\r\n")
		<< TEXT("\tFSketchModule& Host = FSketchModule::Get();\r\n");
	return Epilogue;
}

void FHeaderTool::GenerateReflection(
	const FString& FilePath,
	const FString& InclusionRoot,
	const FClass& Class,
	FString& Prologue,
	FString& Code,
	FString& Epilogue
)
{
	// Prologue
	FStringView Include(&FilePath[InclusionRoot.Len() + 1], FilePath.Len() - InclusionRoot.Len() - 1);
	Prologue << TEXT("#include \"") << Include << TEXT("\"\r\n");

	// Epilogue
	FStringView ClassName = Class.Name.RightChop(1);
	Epilogue << TEXT("\tRegister") << ClassName << TEXT("(Host);\r\n");

	// Registrar beginning
	Code << TEXT("void Register") << ClassName << TEXT("(FSketchModule& Host)\r\n{\r\n");
	Code << TEXT("\tusing namespace sketch;\r\n");

	// Widget factory, including named slots
	{
		Code
			<< TEXT("\tFFactory Factory;\r\n")
			<< TEXT("\tFactory.") << GET_MEMBER_NAME_STRING_VIEW_CHECKED(sketch::FFactory, Name) << TEXT(" = TEXT(\"") << Class.Name.RightChop(1) << TEXT("\");\r\n")
			<< TEXT("\tFactory.") << GET_MEMBER_NAME_STRING_VIEW_CHECKED(sketch::FFactory, ConstructWidget) << TEXT(" = []\r\n")
			<< TEXT("\t{\r\n");
		if (!Class.NamedSlots.IsEmpty())
		{
			using namespace sketch;
			const TCHAR* MetaClassName = GET_TYPE_NAME_STRING_CHECKED(, HeaderTool::FUniqueSlotMeta);
			Code << TEXT("\t\tTSharedRef<") << MetaClassName << TEXT("> Meta = MakeShared<") << MetaClassName << TEXT(">();\r\n");
			Code << TEXT("\t\tMeta->Slots.Reserve(") << Class.NamedSlots.Num() << TEXT(");\r\n");
		}
		Code
			<< TEXT("\t\t") << Class.Name << TEXT("::FArguments Arguments;\r\n")
			<< TEXT("\t\tArguments\r\n");
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.WithCustomInitialization()) [[unlikely]]
				continue;

			Code << TEXT("\t\t\t");
			if (!Property.bSupported)
				Code << TEXT("// ");
			Code << TEXT(".") << Property.Name << TEXT("(Sketch(\"") << Property.Name << TEXT("\", ") << MakeDefaultValue(Property) << TEXT("))\r\n");
		}
		for (const FSlot& UniqueSlot : Class.NamedSlots)
		{
			// .SlotName()[ SAssignNew(Meta->Slots.Emplace(FName(TEXT("SlotName"))), SSketchWidget).SetupAsUniqueSlotContainer(TEXT("SlotName"))
			Code
				<< TEXT("\t\t\t.")
				<< UniqueSlot.Name
				<< TEXT("()[ SAssignNew(Meta->Slots.Emplace(FName(TEXT(\"")
				<< UniqueSlot.Name
				<< TEXT("\"))), SSketchWidget).")
				<< GET_FUNCTION_NAME_STRING_VIEW_CHECKED(SSketchWidget::FArguments, SetupAsUniqueSlotContainer)
				<< TEXT("(TEXT(\"") << UniqueSlot.Name << TEXT("\")) ]\r\n");
		}
		if (!Class.NamedSlots.IsEmpty())
			Code << TEXT("\t\t\t.AddMetaData(MoveTemp(Meta))\r\n");
		Code
			<< TEXT("\t\t\t.SKETCH_WIDGET_FACTORY_BOILERPLATE();\r\n");
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.CustomArgumentInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << TEXT("\t\t\t") << Property.CustomArgumentInitializationCode << TEXT("\r\n");
		}
		Code
			<< TEXT("\t\tTSharedRef<") << Class.Name << TEXT("> Widget = SArgumentNew(Arguments, ") << Class.Name << TEXT(");\r\n");
		for (const FProperty& Property : Class.Properties)
		{
			if (Property.CustomEntityInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << TEXT("\t\t\t") << Property.CustomEntityInitializationCode << TEXT("\r\n");
		}
		Code
			<< TEXT("\t\treturn MoveTemp(Widget);\r\n")
			<< TEXT("\t};\r\n");
		auto ll = SNew(SHorizontalBox);
	}

	// Unique slot enumerator
	if (!Class.NamedSlots.IsEmpty())
	{
		Code << TEXT("\tFactory.EnumerateUniqueSlots = &HeaderTool::FUniqueSlotMeta::GetSlots;\r\n");
	}

	// Dynamic slots
	const bool bHasDynamicSlots = [&]
	{
		if (Class.DynamicSlots.Num() > 1) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Error, TEXT("Multiple types of dynamic slots (%s) aren't currently supported"), *FString(Class.Name));
		}
		return Class.DynamicSlots.Num() == 1;
	}();
	if (bHasDynamicSlots)
	{
		// Type enumerator
		Code
			<< TEXT("\tFactory.")
			<< GET_MEMBER_NAME_STRING_VIEW_CHECKED(FFactory, EnumerateDynamicSlotTypes)
			<< TEXT("= [](const SWidget& Widget) -> ")
			<< GET_TYPE_NAME_VIEW_CHECKED(sketch::, FFactory)
			<< TEXT("::")
			<< GET_TYPE_NAME_VIEW_CHECKED(sketch::FFactory::, FDynamicSlotTypes)
			<< TEXT("\r\n");
		Code
			<< TEXT("\t{\r\n");
		Code
			<< TEXT("\t\t")
			<< GET_TYPE_NAME_VIEW_CHECKED(sketch::, FFactory)
			<< TEXT("::")
			<< GET_TYPE_NAME_VIEW_CHECKED(sketch::FFactory::, FDynamicSlotTypes)
			<< TEXT(" Result;\r\n");
		Code
			<< TEXT("\t\tResult.Reserve(") << Class.DynamicSlots.Num() << TEXT(");\r\n")
			<< TEXT("\t\tResult = { ");
		for (const FSlot& Slot : Class.DynamicSlots)
		{
			Code << TEXT("TEXT(\"") << Slot.Name << TEXT("\"), ");
		}
		Code
			<< TEXT(" };\r\n")
			<< TEXT("\t\treturn Result;\r\n")
			<< TEXT("\t};\r\n");

		// Constructor
		Code
			<< TEXT("\tFactory.")
			<< GET_MEMBER_NAME_STRING_VIEW_CHECKED(FFactory, ConstructDynamicSlot)
			<< TEXT(" = [](SWidget& Widget, const FName& SlotType) -> ")
			<< TEXT("::FSlotBase&\r\n");
		Code
			<< TEXT("\t{\r\n")
			<< TEXT("\t\tcheck(SlotType == TEXT(\"") << Class.DynamicSlots[0].Name << TEXT("\"));\r\n")
			<< TEXT("\t\t") << Class.Name << TEXT("::FSlot* Slot;\r\n")
			<< TEXT("\t\t{\r\n")
			<< TEXT("\t\t\tdecltype(auto) SlotArgs = static_cast<") << Class.Name << TEXT("&>(Widget).AddSlot();\r\n")
			<< TEXT("\t\t\tSlotArgs\r\n");
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.WithCustomInitialization()) [[unlikely]]
				continue;

			Code << TEXT("\t\t\t\t");
			if (!Property.bSupported)
				Code << TEXT("// ");
			Code << TEXT(".") << Property.Name << TEXT("(Sketch(\"") << Property.Name << TEXT("\", ") << MakeDefaultValue(Property) << TEXT("))\r\n");
		}
		Code
			<< TEXT("\t\t\t;\r\n");
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.CustomArgumentInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << TEXT("\t\t\t") << Property.CustomArgumentInitializationCode << TEXT("\r\n");
		}
		Code
			<< TEXT("\t\t\tSlot = SlotArgs.GetSlot();\r\n")
			<< TEXT("\t\t}\r\n");
		for (const auto& Property : Class.DynamicSlots[0].Properties)
		{
			if (Property.CustomEntityInitializationCode.String.IsEmpty()) [[likely]]
				continue;
			Code << TEXT("\t\t") << Property.CustomEntityInitializationCode << TEXT("\r\n");
		}
		Code
			<< TEXT("\t\treturn *Slot;\r\n")
			<< TEXT("\t};\r\n");
	}

	// Registrar ending
	FStringView CategoryView = TEXT("Unknown");
	{
		FStringView MayBeCategoryView = FilePath;
		int LastSlash = INDEX_NONE;
		if (MayBeCategoryView.FindLastChar(TCHAR('/'), LastSlash))[[likely]]
		{
			MayBeCategoryView.LeftChopInline(MayBeCategoryView.Len() - LastSlash);
			if (MayBeCategoryView.FindLastChar(TCHAR('/'), LastSlash))[[likely]]
			{
				MayBeCategoryView.RightChopInline(LastSlash + 1);
				CategoryView = MayBeCategoryView;
			}
		}
	}
	// Host.RegisterFactory(TEXT("Containers"), MoveTemp(Factory));
	Code << TEXT("\tHost.RegisterFactory(TEXT(\"") << CategoryView << TEXT("\"), MoveTemp(Factory));\r\n");
	Code << TEXT("}\r\n");
	Code << TEXT("\r\n");
}

FString FHeaderTool::CombineReflection(const FString& Prologue, const FString& Code, const FString& Epilogue)
{
	FString Result;
	Result.Reserve(Prologue.Len() + Code.Len() + Epilogue.Len() + 128);
	Result
		<< Prologue
		<< TEXT("\r\n")
		<< TEXT("#define LOCTEXT_NAMESPACE \"") << TEXT("Sketch") << TEXT("\"\r\n")
		<< TEXT("\r\n")
		<< TEXT("PRAGMA_DISABLE_DEPRECATION_WARNINGS\r\n")
		<< TEXT("\r\n")
		<< Code
		<< TEXT("\r\n")
		<< Epilogue << TEXT("}\r\n")
		<< TEXT("\r\n")
		<< TEXT("PRAGMA_ENABLE_DEPRECATION_WARNINGS\r\n")
		<< TEXT("\r\n")
		<< TEXT("#undef LOCTEXT_NAMESPACE\r\n");
	return Result;
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

FIndex::FPosition FIndex::LocatePosition(int Position) const
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
			return { Mid, Position - Lines[Mid] };
		if (Position > Lines[Mid])
			Min = Mid + 1;
		else
			Max = Mid - 1;
		check(i < 2048);
	}
}

FIndex::FPosition FIndex::LocatePosition(const FString& Code, const FStringView& View) const
{
	const ptrdiff_t Diff = &View[0] - &Code[0];
	return LocatePosition(Diff);
}

int FIndex::FindScope(int Position) const
{
	if (Scopes.IsEmpty() || Position < Scopes[0].Start)
		return INDEX_NONE;

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

TOptional<FIndex> FHeaderTool::IndexCode(const FString& Code)
{
	TArray<FScope> Scopes;
	Scopes.Reserve(128);
	TArray<int, TInlineAllocator<32>> Stack;
	Stack.Reserve(32);
	TArray<int> Lines;
	Lines.Reserve(Code.Len() / 16);
	Lines.Push(0);
	for (int i = 0; i < Code.Len() - 1; ++i)
	{
		i = SkipComments(Code, i, [&](int NewLine) { Lines.Push(NewLine); });
		if (i == INDEX_NONE) [[unlikely]] break;

		if (Code[i] == TCHAR('{'))
		{
			Scopes.Emplace(i);
			Stack.Push(Scopes.Num() - 1);
		}
		else if (Code[i] == TCHAR('}'))
		{
			if (Stack.IsEmpty()) [[unlikely]] return {};
			const int Pair = Stack.Pop(EAllowShrinking::No);
			Scopes[Pair].End = i;
		}
		else if (Code[i] == TCHAR('\n'))
		{
			Lines.Push(i);
		}
	}
	if (!Stack.IsEmpty()) [[unlikely]] return {};
	FIndex Result;
	Result.Scopes = MoveTemp(Scopes);
	Result.Lines = MoveTemp(Lines);
	return MoveTemp(Result);
}

bool FHeaderTool::ProcessCode(
	const FString& Code,
	const TSet<FName>& SupportedAttributes,
	TArray<FClass>& OutClasses
)
{
	const TOptional<FIndex> MayBeIndex = IndexCode(Code);
	if (!MayBeIndex) [[unlikely]]
	{
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Failed to index code"));
		return false;
	}
	const FIndex& Index = *MayBeIndex;

	int Position = 0;
	for (;;)
	{
		// Locate next widget args
		const int ArgsStart = FindString(Code, Position, TEXT("SLATE_BEGIN_ARGS"));
		if (ArgsStart == INDEX_NONE) { return true; }

		const int ArgsEnd = FindString(Code, ArgsStart, TEXT("SLATE_END_ARGS"));
		if (ArgsEnd == INDEX_NONE) [[unlikely]]
		{
			const FIndex::FPosition Pos = Index.LocatePosition(ArgsStart);
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("No 'SLATE_END_ARGS' for 'SLATE_BEGIN_ARGS' at %i::%i"), Pos.Line, Pos.Column);

			// Update position so we can continue at least somehow
			Position = ArgsStart + 1;
			continue;
		}

		// Locate class beginning
		// Args are always in main class scope
		// Locate first scope that ends after given widget attributes (and make sure it contains them)
		// Then find first inner scope that still contains given widget attributes
		int ScopeIndex = Index.FindScope(ArgsStart);
		if (ScopeIndex == INDEX_NONE) [[unlikely]]
		{
			const FIndex::FPosition Pos = Index.LocatePosition(ArgsStart);
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't determine class scope for 'SLATE_BEGIN_ARGS' at %i::%i"), Pos.Line, Pos.Column);
			continue;
		}

		// Once class extents are determined - set next search position
		Position = Index.Scopes[ScopeIndex].End + 1;

		// Locate class name
		const auto& [ClassName, bTemplate] = FindClassName(Code, Index, ScopeIndex);
		if (ClassName.IsEmpty()) [[unlikely]]
		{
			const FIndex::FPosition Pos = Index.LocatePosition(ArgsStart);
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't determine class name for 'SLATE_BEGIN_ARGS' at %i::%i"), Pos.Line, Pos.Column);
			continue;
		}
		if (bTemplate) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Display, TEXT("Skipping class %s as it's a template"), *FString(ClassName));
			continue;
		}

		// Make sure this is not an internal data structure
		if (ClassName[0] != TCHAR('S')) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Display, TEXT("Skipping class %s as doesn't seem to be a widget"), *FString(ClassName));
			continue;
		}

		// Now find "Construct" and make sure it has only one argument
		// Nothing to do with widgets with multiple "Construct" arguments
		// It's somewhere inside class body
		// And there must be exactly 0 "," inside it brackets
		// Do not look for "Construct" before slate properties though
		// There may be "Construct" for a slot, but not the one we are looking for 'cause FArguments are not
		// defined before "SLATE_END_ARGS"
		FStringView ClassBody(&Code[ArgsEnd], Index.Scopes[ScopeIndex].Length() - 1);
		const int ConstructPosition = FindString(ClassBody, 0, TEXT("Construct"));
		if (ConstructPosition == INDEX_NONE) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find 'Construct' for class %s"), *FString(ClassName));
			continue;
		}
		const int FirstBracketPosition = FindString(ClassBody, ConstructPosition, TEXT("("));
		if (FirstBracketPosition == INDEX_NONE) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find '(' after 'Construct' for class %s"), *FString(ClassName));
			continue;
		}
		const int SecondBracketPosition = FindString(ClassBody, FirstBracketPosition, TEXT(")"));
		if (SecondBracketPosition == INDEX_NONE) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find ')' after 'Construct' for class %s"), *FString(ClassName));
			continue;
		}
		FStringView ConstructArguments(&ClassBody[FirstBracketPosition + 1], SecondBracketPosition - FirstBracketPosition - 1);
		const int CommaPosition = FindString(ConstructArguments, 0, TEXT(","));
		if (CommaPosition != INDEX_NONE) [[unlikely]]
		{
			UE_LOG(LogSketchHeaderTool, Display, TEXT("Skipping class %s due to multiple arguments in 'Construct'"), *FString(ClassName));
			continue;
		};

		// Locate slate properties defaults
		// They begin after "SLATE_BEGIN_ARGS(...):" and ends before first "SLATE_"
		// Can't rely on figure bracket since they can be used inside initialization list
		FStringView SlateProperties(&Code[ArgsStart], ArgsEnd - ArgsStart);
		constexpr FStringView Prefix(TEXT("SLATE_"));
		int PropertyPosition = FindString(SlateProperties, 1, Prefix);
		FStringView PropertiesInitializers(&SlateProperties[0], PropertyPosition - 1);

		// Parse all SLATE_ATTRIBUTE-s and SLATE_ARGUMENT-s
		// Preserve order, so search only "SLATE_" and then determine exact type depending on continuation
		FClass& Class = OutClasses.Emplace_GetRef();
		Class.Name = ClassName;
		for (; PropertyPosition != INDEX_NONE; PropertyPosition = FindString(SlateProperties, PropertyPosition + 1, Prefix))
		{
			// Parse property
			FStringView PropertyAndFurther(&SlateProperties[PropertyPosition] + Prefix.Len(), SlateProperties.Len() - PropertyPosition - Prefix.Len());
			TOptional<FAnchoredProperty> Property;
			TOptional<FSlot> NamedSlot;
			TOptional<FSlot> DynamicSlot;
			if (PropertyAndFurther.StartsWith(TEXT("ARGUMENT")))
			{
				Property = ProcessSlateProperty(Index, Code, ClassName, PropertiesInitializers, PropertyAndFurther, SupportedAttributes);
			}
			else if (PropertyAndFurther.StartsWith(TEXT("ARGUMENT_DEFAULT")))
			{
				Property = ProcessSlateProperty(Index, Code, ClassName, PropertiesInitializers, PropertyAndFurther, SupportedAttributes);

				// Only use default value if default constructor doesn't have one, 'cause default constructor has a higher priority
				if (Property && !Property->Property.DefaultValue)
				{
					TOptional<FProcessedString> DefaultValue = ProcessSlateArgumentDefaultValue(Index, Code, PropertyAndFurther, Property->Anchors);
					if (DefaultValue) [[likely]] Property->Property.DefaultValue = MoveTemp(DefaultValue);
				}
			}
			else if (PropertyAndFurther.StartsWith(TEXT("ATTRIBUTE")))
			{
				Property = ProcessSlateProperty(Index, Code, ClassName, PropertiesInitializers, PropertyAndFurther, SupportedAttributes);
			}
			else if (PropertyAndFurther.StartsWith(TEXT("DEFAULT_SLOT")))
			{
				NamedSlot = ProcessUniqueSlateSlot(Index, Code, PropertyAndFurther).Get({}).Key;
			}
			else if (PropertyAndFurther.StartsWith(TEXT("NAMED_SLOT")))
			{
				NamedSlot = ProcessUniqueSlateSlot(Index, Code, PropertyAndFurther).Get({}).Key;
			}
			else if (PropertyAndFurther.StartsWith(TEXT("SLOT_ARGUMENT")))
			{
				DynamicSlot = ProcessDynamicSlot(Index, Code, ScopeIndex, ClassName, PropertyAndFurther, SupportedAttributes);
			}
			else if (PropertyAndFurther.StartsWith(TEXT("EVENT")))
			{
				// Nothing to do with them right now
			}
			else if (PropertyAndFurther.StartsWith(TEXT("STYLE_ARGUMENT")))
			{
				// Nothing to do with them right now
			}
			else [[unlikely]]
			{
				const FIndex::FPosition Pos = Index.LocatePosition(Code, PropertyAndFurther);
				UE_LOG(LogSketchHeaderTool, Warning, TEXT("Unknown slate property at %i::%i"), Pos.Line, Pos.Column);
			}
			check(((int)Property.IsSet() + (int)NamedSlot.IsSet() + (int)DynamicSlot.IsSet()) <= 1); // It's one or another

			if (Property) [[likely]] Class.Properties.Emplace(MoveTemp(Property->Property));
			if (NamedSlot) Class.NamedSlots.Emplace(MoveTemp(*NamedSlot));
			if (DynamicSlot) Class.DynamicSlots.Emplace(MoveTemp(*DynamicSlot));
		}
	}
}

FPropertyAnchors::FPropertyAnchors(
	const FStringView& PropertyAndFurther,
	const auto& LogErr,
	auto&&... Args
)
{
	// Find both bracket
	FirstBracket = FindString(PropertyAndFurther, 0, TEXT("("));
	if (FirstBracket == INDEX_NONE) [[unlikely]]
	{
		LogErr(TCHAR('('), Args...);
		return;
	}
	SecondBracket = FindPairedBracket(PropertyAndFurther, FirstBracket, TCHAR('('), TCHAR(')'));
	if (SecondBracket == INDEX_NONE) [[unlikely]]
	{
		LogErr(TCHAR(')'), Args...);
		return;
	}

	// Locate last comma within bracket - everything before it is attribute type, everything after - its name
	FStringView PropertyData(&PropertyAndFurther[FirstBracket + 1], SecondBracket - FirstBracket - 1);
	Comma = FindLastString(PropertyData, 0, TEXT(","));
	if (Comma == INDEX_NONE) [[unlikely]]
	{
		LogErr(TCHAR(','), Args...);
		return;
	}
	Comma += FirstBracket + 1;
}

TOptional<FAnchoredProperty> FHeaderTool::ProcessSlateProperty(
	const FIndex& Index,
	const FString& Code,
	const FStringView& ClassName,
	const FStringView& PropertiesDefaults,
	const FStringView& PropertyAndFurther,
	const TSet<FName>& SupportedAttributes
)
{
	auto LogErr = [&](TCHAR Char, const TCHAR* Entity = TEXT(""))
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find '%c' for slate property%s at %i::%i"), Char, Entity, Position.Line, Position.Column);
	};
	FPropertyAnchors Anchors(PropertyAndFurther, LogErr);
	if (!Anchors) [[unlikely]] return {};

	// Gather attribute name and type
	FProperty Result;
	FProcessedString Name = CleanCode(FStringView(&PropertyAndFurther[Anchors.Comma + 1], Anchors.SecondBracket - Anchors.Comma - 1));
	if (!Name.Container.IsEmpty())[[unlikely]]
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't determine attribute name for slate property at %i::%i"), Position.Line, Position.Column);
		return {};
	}
	Result.Name = Name.String;
	Result.Type = CleanCode(FStringView(&PropertyAndFurther[Anchors.FirstBracket + 1], Anchors.Comma - Anchors.FirstBracket - 1));
	Result.bSupported = false;
	FName PropertyTypeName = FName(Result.Type.String, FNAME_Find);
	if (!PropertyTypeName.IsNone())
	{
		Result.bSupported = SupportedAttributes.Contains(PropertyTypeName);
	}

	// Consider predefined override
	if (TArray<FOverride>* Overrides = ClassOverrides.FindByHash(GetTypeHash(ClassName), ClassName))
	[[unlikely]]
	{
		const FOverride* Override = Overrides->FindByPredicate([&](const FOverride& SomeOverride) { return SomeOverride.Property == Result.Name; });
		if (Override)[[unlikely]]
		{
			Result.DefaultValue.Emplace();
			Result.DefaultValue->Container = Override->ValueOverride;
			Result.DefaultValue->String = Result.DefaultValue->Container;
			return FAnchoredProperty{ MoveTemp(Result), MoveTemp(Anchors) };
		}
	}

	// Locate default value
	TStringBuilder<128> InitializerName;
	InitializerName += TCHAR('_');
	InitializerName += Result.Name;
	int InitializerPosition = 0;
	for (;;)
	{
		InitializerPosition = FindString(PropertiesDefaults, InitializerPosition, InitializerName.ToView());
		if (InitializerPosition == INDEX_NONE)
			break;

		// Make sure it's not a partial match like "_Text" in "_TextStyle"
		// Initializer name must be followed by anything except a digit or a letter
		if (IsAlnum(PropertiesDefaults[InitializerPosition + InitializerName.Len()])) [[unlikely]]
		{
			InitializerPosition += InitializerName.Len() - 1;
			continue;
		}

		const int First = FindString(PropertiesDefaults, InitializerPosition, TEXT("("));
		if (First == INDEX_NONE) [[unlikely]]
		{
			LogErr(TCHAR('('), TEXT(" initializer"));
			break;
		}
		const int Second = FindPairedBracket(PropertiesDefaults, First, TCHAR('('), TCHAR(')'));
		if (Second == INDEX_NONE) [[unlikely]]
		{
			LogErr(TCHAR(')'), TEXT(" initializer"));
			break;
		}
		Result.DefaultValue = CleanCode(PropertiesDefaults.Mid(First + 1, Second - First - 1));
		break;
	}

	return FAnchoredProperty{ MoveTemp(Result), MoveTemp(Anchors) };
}

TOptional<TTuple<FSlot, FPropertyAnchors>> FHeaderTool::ProcessUniqueSlateSlot(
	const FIndex& Index,
	const FString& Code,
	const FStringView& PropertyAndFurther
)
{
	auto LogErr = [&](TCHAR Char)
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find '%c' for slate slot at %i::%i"), Char, Position.Line, Position.Column);
	};
	FPropertyAnchors Anchors(PropertyAndFurther, LogErr);
	if (!Anchors) [[unlikely]] return {};

	FProcessedString Name = CleanCode(FStringView(&PropertyAndFurther[Anchors.Comma + 1], Anchors.SecondBracket - Anchors.Comma - 1));
	if (!Name.Container.IsEmpty())[[unlikely]]
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't determine slot name for slate property at %i::%i"), Position.Line, Position.Column);
		return {};
	}
	TOptional<TTuple<FSlot, FPropertyAnchors>> Result;
	Result.Emplace();
	Result->Key.Name = Name.String;
	Result->Value = MoveTemp(Anchors);
	return MoveTemp(Result);
}

TOptional<FSlot> FHeaderTool::ProcessDynamicSlot(
	const FIndex& Index,
	const FString& Code,
	int ClassScopeIndex,
	const FStringView& ClassName,
	const FStringView& PropertyAndFurther,
	const TSet<FName>& SupportedAttributes
)
{
	TOptional UniqueSlot = ProcessUniqueSlateSlot(Index, Code, PropertyAndFurther);
	if (!UniqueSlot) [[unlikely]] return {};

	auto None = [&](const TCHAR* Message)
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't %s for dynamic slot at %i::%i"), Message, Position.Line, Position.Column);
		return TOptional<FSlot>{};
	};

	const FPropertyAnchors& Anchors = UniqueSlot->Value;
	FProcessedString Type = CleanCode(FStringView(&PropertyAndFurther[Anchors.FirstBracket + 1], Anchors.Comma - Anchors.FirstBracket - 1));
	if (!Type.Container.IsEmpty()) [[unlikely]] return None(TEXT("determine slot type"));
	const int Namespace = UE::String::FindLast(Type.String, TEXT("::"), ESearchCase::CaseSensitive);
	if (Namespace != INDEX_NONE)[[unlikely]]
	{
		Type.String = Type.String.RightChop(Namespace + 2);
	}
	FSlot Slot;
	Slot.Name = UniqueSlot->Key.Name;
	Slot.TypeName = Type.String;

	// Make sure class contains "static FSlot::FSlotArguments Slot()"
	FStringView ClassBody(&Code[Index.Scopes[ClassScopeIndex].Start + 1], Index.Scopes[ClassScopeIndex].End - Index.Scopes[ClassScopeIndex].Start - 2);
	// Or don't 'cause we are not going to use it either way
	// {
	// 	auto FindSlot = [&](int Position)
	// 	{
	// 		return Sequence::Find(
	// 			ClassBody,
	// 			Position,
	// 			Sequence::SubscopeFilter,
	// 			Sequence::TemplateDeclaration(Sequence::ST_MustNotBePresent),
	// 			Sequence::String(TEXT("static")),
	// 			Sequence::ModuleApi(Sequence::ST_Optional),
	// 			Sequence::String(Slot.TypeName),
	// 			Sequence::String(TEXT("::")),
	// 			Sequence::String(GET_TYPE_NAME_STRING_CHECKED(SHorizontalBox::FSlot::, FSlotArguments)),
	// 			Sequence::String(Slot.TypeName.RightChop(1)),
	// 			Sequence::String(TEXT("(")),
	// 			Sequence::String(TEXT(")"))
	// 		);
	// 	};
	// 	auto SlotConstructor = FindSlot(0);
	// 	if (!SlotConstructor) [[unlikely]]
	// 	{
	// 		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
	// 		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find slot constructor for dynamic slot '%s' at %i::%i"), *FString(Slot.TypeName), Position.Line, Position.Column);
	// 		return {};
	// 	}
	// }

	// Make sure class contains "ReturnType AddSlot(...)"
	{
		FString MethodName = TEXT("Add");
		MethodName.Append(Slot.TypeName.RightChop(1));
		auto Match = Sequence::Find(
			ClassBody,
			Sequence::SubscopeFilter<>,
			Sequence::ModuleApi(),
			Sequence::AlnumWord(),
			Sequence::String(MethodName),
			Sequence::Subscope<TCHAR('('), TCHAR(')')>()
		);
		if (!Match) [[unlikely]] return None(TEXT("find 'AddSlot'"));

		// Collect constructor arguments
		// For now make sure there are none, or all of them has default values
		// Their support will be implemented later
		FStringView ArgsView = Match->back();
		ArgsView.MidInline(1, ArgsView.Len() - 2);
		ArgsView.TrimStartAndEndInline();
		TArray<FProperty> Args = ParseArguments(ArgsView);
		if (Args.ContainsByPredicate([](const FProperty& Property) { return !Property.DefaultValue.IsSet(); }))[[unlikely]]
			return None(TEXT("find AddSlot without explicit arguments"));
	}

	// Make sure class contains "ReturnType RemoveSlot(...)"
	{
		FString MethodName = TEXT("Remove");
		MethodName.Append(Slot.TypeName.RightChop(1));
		auto Match = Sequence::Find(
			ClassBody,
			Sequence::SubscopeFilter<>,
			Sequence::ModuleApi(),
			Sequence::OneOf({ TEXT("void"), TEXT("bool"), TEXT("int32"), TEXT("int") }),
			Sequence::String(MethodName),
			Sequence::String(TEXT("(")),
			Sequence::String(TEXT("const"), Sequence::ST_Optional),
			Sequence::OneOf(
				Sequence::String(TEXT("int32")),
				Sequence::String(TEXT("int")),
				Sequence::Subsequence(
					Sequence::String(TEXT("TSharedRef")),
					Sequence::Subscope<TCHAR('<'), TCHAR('>')>(),
					Sequence::String(TEXT("&"), Sequence::ST_Optional)
				)
			),
			Sequence::AlnumWord(Sequence::ST_Optional),
			Sequence::String(TEXT(")"))
		);
		if (!Match) [[unlikely]] return None(TEXT("find 'RemoveSlot'"));
	}

	// Locate slot class within class body. Note that "SLATE_SLOT_ARGUMENT" depends on slot public interface,
	// so full class definition have to be present before widget's properties
	int SlotScopeIndex = INDEX_NONE;
	{
		const FStringView SlotDeclarationScope = FStringView(Code).Mid(
			Index.Scopes[ClassScopeIndex].Start + 1,
			&PropertyAndFurther[0] - &Code[Index.Scopes[ClassScopeIndex].Start] - 2
		);
		using namespace Sequence;
		auto Match = Find(
			SlotDeclarationScope,
			SubscopeFilter<>,
			// Subsequence(
			// 	ST_Optional,
			// 	String(TEXT("template")),
			// 	Subscope<TCHAR('<'), TCHAR('>')>()
			// ),
			String(TEXT("class")),
			ModuleApi(),
			Subsequence(
				ST_Optional,
				String(TEXT("alignas")),
				Subscope<TCHAR('('), TCHAR(')')>()
			),
			ModuleApi(), // Module API may come before or after alignment specifier, so look for it twice
			String<ST_Alnum>(Slot.TypeName)
			// String<ST_Punct>(TEXT(":"))
		);
		if (!Match) [[unlikely]] return None(TEXT("find slot class"));

		const FStringView SlotBodyScope = SlotDeclarationScope.RightChop(&Match->back()[Match->back().Len() - 1] - &SlotDeclarationScope[0] + 1);
		const int ScopeBeginning = FindString(SlotBodyScope, 0, TEXT("{"), Sequence::SubscopeFilter<TCHAR('<'), TCHAR('>')>);
		if (ScopeBeginning != INDEX_NONE) [[likely]]
		{
			const int AbsoluteScopeBeginning = &SlotBodyScope[ScopeBeginning] - &Code[0];
			for (int i = ClassScopeIndex + 1; i < Index.Scopes.Num(); ++i)
			{
				if (Index.Scopes[i].Start == AbsoluteScopeBeginning)
				{
					SlotScopeIndex = i;
					break;
				}
			}
		}
		if (SlotScopeIndex == INDEX_NONE) [[unlikely]] return None(TEXT("find body beginning"));
	}

	FStringView SlotProperties;
	{
		// Locate "SLATE_SLOT_BEGIN_ARGS" and "SLATE_SLOT_END_ARGS" within slot
		const FStringView SlotBody = FStringView(Code).Mid(
			Index.Scopes[SlotScopeIndex].Start + 1,
			Index.Scopes[SlotScopeIndex].Length() - 2
		);
		constexpr FStringView SlateSlotBeginArgs(TEXT("SLATE_SLOT_BEGIN_ARGS"));
		const int ArgsBeginning = FindString(SlotBody, 0, SlateSlotBeginArgs);
		if (ArgsBeginning == INDEX_NONE) [[unlikely]] return None(TEXT("find 'SLATE_SLOT_BEGIN_ARGS'"));
		const int ArgsEnd = FindString(SlotBody, ArgsBeginning, TEXT("SLATE_SLOT_END_ARGS"));
		if (ArgsEnd == INDEX_NONE) [[unlikely]] return None(TEXT("find 'SLATE_SLOT_END_ARGS'"));
		SlotProperties = SlotBody.Mid(ArgsBeginning + SlateSlotBeginArgs.Len(), ArgsEnd - ArgsBeginning - SlateSlotBeginArgs.Len());

		// Check if slot uses any mixins
		FStringView SuffixAndFurther = SlotBody.RightChop(ArgsBeginning + SlateSlotBeginArgs.Len());
		if (SuffixAndFurther.StartsWith(FStringView(TEXT("_")), ESearchCase::CaseSensitive))
		{
			// Detect number of slot mixins
			int NumMixins;
			if (SuffixAndFurther.StartsWith(FStringView(TEXT("_OneMixin")), ESearchCase::CaseSensitive))
				NumMixins = 1;
			else if (SuffixAndFurther.StartsWith(FStringView(TEXT("_TwoMixins")), ESearchCase::CaseSensitive))
				NumMixins = 2;
			else if (SuffixAndFurther.StartsWith(FStringView(TEXT("_ThreeMixins")), ESearchCase::CaseSensitive))
				NumMixins = 3;
			else if (SuffixAndFurther.StartsWith(FStringView(TEXT("_FourMixins")), ESearchCase::CaseSensitive))
				NumMixins = 4;
			else return None(TEXT("detect number of mixins"));

			// Get next "()" scope
			const int OpeningBracketPosition = FindString(SuffixAndFurther, 1, FStringView(TEXT("(")));
			if (OpeningBracketPosition == INDEX_NONE) [[unlikely]] return None(TEXT("find '('"));
			const int ClosingBracketPosition = FindPairedBracket(SuffixAndFurther, OpeningBracketPosition, TCHAR('('), TCHAR(')'));
			if (ClosingBracketPosition == INDEX_NONE) [[unlikely]] return None(TEXT("find ')'"));

			// Skip first argument - it's class slot name
			const FStringView Arguments = SuffixAndFurther.Mid(OpeningBracketPosition + 1, ClosingBracketPosition - OpeningBracketPosition - 1);
			const int FirstComma = FindString(Arguments, 0, FStringView(TEXT(",")), Sequence::SubscopeFilter<TCHAR('<'), TCHAR('>')>);
			if (FirstComma == INDEX_NONE) [[unlikely]] return None(TEXT("find first ','"));

			// Consider second argument - it might be BasicLayoutWidgetSlot which already contains 2 mixins
			const int SecondComma = FindString(Arguments, FirstComma + 1, FStringView(TEXT(",")), Sequence::SubscopeFilter<TCHAR('<'), TCHAR('>')>);
			if (SecondComma == INDEX_NONE) [[unlikely]] return None(TEXT("find second ','"));
			FStringView ParentClass = Arguments.Mid(FirstComma + 1, SecondComma - FirstComma - 2);
			ParentClass.TrimStartInline();
			ParentClass.RightChopInline(1);
			std::array<FStringView, 4> Mixins;
			int NumPredefinedMixins = 0;
			if (ParentClass.StartsWith(TEXT("BasicLayoutWidgetSlot")))
			{
				NumPredefinedMixins = 2;
				Mixins[0] = FStringView(TEXT("TPadding"));
				Mixins[1] = FStringView(TEXT("TAlignment"));
			}

			// Gather all mixins
			int LastComma = SecondComma;
			for (int i = 1; i < NumMixins; ++i)
			{
				const int NextComma = FindString(Arguments, LastComma + 1, FStringView(TEXT(",")), Sequence::SubscopeFilter<TCHAR('<'), TCHAR('>')>);
				if (NextComma == INDEX_NONE) [[unlikely]] return None(TEXT("locate another slot mixin"));
				Mixins[NumPredefinedMixins + i] = Arguments.Mid(LastComma + 1, NextComma - LastComma - 2);
				LastComma = NextComma;
			}
			Mixins[NumPredefinedMixins + NumMixins - 1] = Arguments.RightChop(LastComma + 1);

			// Add each mixin attributes
			for (int i = 0; i < NumMixins + NumPredefinedMixins; ++i)
			{
				if (Mixins[i].Contains(FStringView(TEXT("TAlignment")), ESearchCase::CaseSensitive))
				{
					// TAlignmentWidgetSlotMixin
					constexpr FStringView HAlignmentName = GET_TYPE_NAME_VIEW_CHECKED(, EHorizontalAlignment);
					constexpr FStringView VAlignmentName = GET_TYPE_NAME_VIEW_CHECKED(, EVerticalAlignment);
					Slot.Properties.Emplace(
						FProperty{
							.Type = { .String = HAlignmentName },
							.Name = TEXT("HAlign"),
							.DefaultValue = FProcessedString{ .String = GET_ENUMERATOR_NAME_STRING_VIEW_CHECKED(EHorizontalAlignment, HAlign_Fill) },
							.bSupported = SupportedAttributes.ContainsByHash(GetTypeHash(HAlignmentName), HAlignmentName)
						});
					Slot.Properties.Emplace(
						FProperty{
							.Type = { .String = VAlignmentName },
							.Name = TEXT("VAlign"),
							.DefaultValue = FProcessedString{ .String = GET_ENUMERATOR_NAME_STRING_VIEW_CHECKED(EVerticalAlignment, VAlign_Fill) },
							.bSupported = SupportedAttributes.ContainsByHash(GetTypeHash(VAlignmentName), VAlignmentName)
						});
				}
				else if (Mixins[i].Contains(FStringView(TEXT("TPadding")), ESearchCase::CaseSensitive))
				{
					// TPaddingWidgetSlotMixin
					constexpr FStringView MarginName = GET_TYPE_NAME_VIEW_CHECKED(, FMargin);
					Slot.Properties.Emplace(
						FProperty{
							.Type = { .String = MarginName },
							.Name = TEXT("Padding"),
							.bSupported = SupportedAttributes.ContainsByHash(GetTypeHash(MarginName), MarginName)
						}
					);
				}
				else if (Mixins[i].Contains(FStringView(TEXT("TResizing")), ESearchCase::CaseSensitive))
				{
					// As of 5.6 epic uses slate attributes for this mixin, but not every widget registers its slot attributes
					// So this mixin's attributes can't be binds, only plain values
					// If widget explicitly registers - we can assume it registers its slot's attributes as well
					const int WidgetDeclaration = FindString(ClassBody, 0, TEXT("SLATE_DECLARE_WIDGET"), Sequence::SubscopeFilter<>);
					Slot.Properties.Emplace(
						FProperty{
							.CustomEntityInitializationCode = {
								.String = WidgetDeclaration == INDEX_NONE
									          ? FStringView(TEXT("HeaderTool::InitResizingMixinProperties(*Slot, false);"))
									          : FStringView(TEXT("HeaderTool::InitResizingMixinProperties(*Slot, true);"))
							}
						}
					);
				}
				else
				{
					UE_LOG(LogSketchHeaderTool, Warning, TEXT("Unknown mixin: %s"), *FString(Mixins[i]));
				}
			}
		}
	}

	// Gather slot attributes and arguments
	// Parse all SLATE_ATTRIBUTE-s and SLATE_ARGUMENT-s
	// Preserve order, so search only "SLATE_" and then determine exact type depending on continuation
	constexpr FStringView Prefix(TEXT("SLATE_"));
	for (
		int PropertyPosition = FindString(SlotProperties, 0, Prefix);
		PropertyPosition != INDEX_NONE;
		PropertyPosition = FindString(SlotProperties, PropertyPosition + 1, Prefix)
	)
	{
		// Parse property
		FStringView SlotPropertyAndFurther(&SlotProperties[PropertyPosition] + Prefix.Len(), SlotProperties.Len() - PropertyPosition - Prefix.Len());
		TOptional<FAnchoredProperty> Property;
		if (SlotPropertyAndFurther.StartsWith(TEXT("ARGUMENT")))
		{
			Property = ProcessSlateProperty(Index, Code, ClassName, {}, SlotPropertyAndFurther, SupportedAttributes);
		}
		else if (SlotPropertyAndFurther.StartsWith(TEXT("ATTRIBUTE")))
		{
			Property = ProcessSlateProperty(Index, Code, ClassName, {}, SlotPropertyAndFurther, SupportedAttributes);
		}
		else [[unlikely]]
		{
			const FIndex::FPosition Pos = Index.LocatePosition(Code, PropertyAndFurther);
			UE_LOG(LogSketchHeaderTool, Warning, TEXT("Unknown slate property at %i::%i"), Pos.Line, Pos.Column);
		}

		if (Property)[[likely]]
		{
			Slot.Properties.Emplace(MoveTemp(Property->Property));
		}
	}

	return MoveTemp(Slot);
}

TOptional<FProcessedString> FHeaderTool::ProcessSlateArgumentDefaultValue(
	const FIndex& Index,
	const FString& Code,
	const FStringView& PropertyAndFurther,
	FPropertyAnchors& Anchors
)
{
	// SLATE_ARGUMENT_DEFAULT has two initialization options:
	// - = ...;
	// - { ... };
	// Either way find ";" first
	auto LogErr = [&](const TCHAR* What)
	{
		const FIndex::FPosition Position = Index.LocatePosition(Code, PropertyAndFurther);
		UE_LOG(LogSketchHeaderTool, Warning, TEXT("Couldn't find %s for slate argument default value at %i::%i"), What, Position.Line, Position.Column);
	};
	const int Semicolon = FindString(PropertyAndFurther, Anchors.SecondBracket, TEXT(";"));
	if (Semicolon == INDEX_NONE) [[unlikely]]
	{
		LogErr(TEXT("';'"));
		return {};
	}

	// Find first service symbol after property - either "=" or "{"
	int i = SkipNoneCode(PropertyAndFurther, Anchors.SecondBracket + 1);
	if (i != INDEX_NONE && i != Semicolon) [[likely]]
	{
		if (PropertyAndFurther[i] == TCHAR('='))
		{
			FProcessedString DefaultValue = CleanCode(FStringView(&PropertyAndFurther[i + 1], Semicolon - i - 1));
			return MoveTemp(DefaultValue);
		}
		if (PropertyAndFurther[i] == TCHAR('{'))
		{
			const int ClosingBrace = FindPairedBracket(PropertyAndFurther, i, TCHAR('{'), TCHAR('}'));
			if (ClosingBrace == INDEX_NONE) [[unlikely]]
			{
				LogErr(TEXT("'}'"));
				return {};
			}
			FProcessedString DefaultValue = CleanCode(FStringView(&PropertyAndFurther[i + 1], ClosingBrace - i - 1));
			return MoveTemp(DefaultValue);
		}
	}
	LogErr(TEXT("'=' or '{'"));
	return {};
}

template <class T>
class alignas(0) alignas(123) SKETCH_API qwe /*: TArray<class lal>*/
{
	SLATE_BEGIN_ARGS(qwe);
		SLATE_ARGUMENT(int, lal)
		// SLATE_SLOT_ARGUMENT()
		// SLATE_ARGUMENT_DEFAULT(FString, kek){ TEXT("qew") };
		// SLATE_DEFAULT_SLOT(FArguments, Content)
		// SLATE_NAMED_SLOT(FArguments, Slot)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}
};

#undef GET_TYPE_NAME_STRING_CHECKED
