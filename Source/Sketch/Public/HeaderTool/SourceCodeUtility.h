#pragma once
#include "ConstexprTuple.h"
#include "ConstexprStringView.h"
#include "SourceCodeTypes.h"
#include "BuildDependent.h"
#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/UnrealString.h"

/**
 * Mantras of this file:
 * - When looking for an ending of something - return index of the first character after it even if it will be an illegal index.
 *   Thus, you can state that thing is not malformed, and caller won't need to add +1 manually.
 * - When outputting new lines somewhere - always output indices of the last new line combinations characters, so they are always valid,
 *   and other characters positions can be compared by simple '<' and '>'
 */

/*************************************************************************************************/
// Elementary tests //
/*************************************************************************************************/

namespace sketch::SourceCode
{
	constexpr bool IsInRange(TCHAR Char, TCHAR Min, TCHAR Max) { return Char >= Min && Char <= Max; }

	constexpr bool IsWhitespace(TCHAR Char)
	{
		return IsInRange(Char, TCHAR('\t'), TCHAR('\r'))
			|| Char == TCHAR(' ');
	}

	constexpr bool IsNameChar(TCHAR Char)
	{
		return IsInRange(Char, TCHAR('0'), TCHAR('9'))
			|| IsInRange(Char, TCHAR('a'), TCHAR('z'))
			|| IsInRange(Char, TCHAR('A'), TCHAR('Z'))
			|| Char == TCHAR('_');
	}

	constexpr bool IsPunctChar(TCHAR Char)
	{
		return IsInRange(Char, TCHAR('!'), TCHAR('/'))
			|| IsInRange(Char, TCHAR(':'), TCHAR('@'))
			|| IsInRange(Char, TCHAR('['), TCHAR('^'))
			|| Char == TCHAR('`')
			|| IsInRange(Char, TCHAR('{'), TCHAR('~'));
	};

	constexpr bool IsControl(TCHAR Char) { return Char < TCHAR(' ') || Char == TCHAR(127); }

	constexpr bool IsGraph(TCHAR Char) { return Char >= TCHAR('!') && Char <= TCHAR('~'); }

	constexpr int ConsumeNewLine(const sketch::FStringView& Code, int Position)
	{
		if (Code.IsValidIndex(Position + 1))[[likely]]
		{
			Position += Code[Position] == TCHAR('\r');
		}
		return Code[Position] == TCHAR('\n') ? Position + 1 : INDEX_NONE;
	}

	constexpr int ConsumeLineBreak(const sketch::FStringView& Code, int Position)
	{
		return Code[Position] == TCHAR('\\') ? ConsumeNewLine(Code, Position + 1) : INDEX_NONE;
	}
}

/*************************************************************************************************/
// Comment //
/*************************************************************************************************/

namespace sketch::SourceCode::Comment
{
	enum ECommentType
	{
		CT_None,
		CT_SingleLine,
		CT_MultiLine
	};

	constexpr ECommentType DetectType(const sketch::FStringView& Code, int Position)
	{
		if (Code[Position] == TCHAR('/') && Code.IsValidIndex(Position + 1)) [[unlikely]]
		{
			if (Code[Position + 1] == TCHAR('/')) return CT_SingleLine;
			if (Code[Position + 1] == TCHAR('*')) return CT_MultiLine;
		}
		return CT_None;
	}

	/** @return Index of the first character that doesn't belong to a comment, or Code.Len() if all remaining code is a comment */
	constexpr int SingleLineEnding(const sketch::FStringView& Code, int Position, const auto& OnNewLine)
	{
		check(DetectType(Code, Position) == CT_SingleLine);

		int i = Position + 2;
		for (; i < Code.Len() - 1; ++i)
		{
			{
				const int AfterLineBreak = ConsumeLineBreak(Code, i);
				if (AfterLineBreak != INDEX_NONE) [[unlikely]]
				{
					OnNewLine(AfterLineBreak - 1);
					i = AfterLineBreak - 1;
					continue;
				}
			}

			{
				const int AfterNewLine = ConsumeNewLine(Code, i);
				if (AfterNewLine != INDEX_NONE) [[unlikely]]
				{
					OnNewLine(AfterNewLine - 1);
					return AfterNewLine;
				}
			}
		}
		return Code.Len();
	}

	constexpr int SingleLineEnding(const sketch::FStringView& Code, int Position)
	{
		return SingleLineEnding(Code, Position, [](int) {});
	}

	/** @return Index of the first character that doesn't belong to a comment, Code.Len() if all remaining code is a comment, or INDEX_NONE  if comment is malformed */
	constexpr int MultiLineEnding(const sketch::FStringView& Code, int Position, const auto& OnNewLine)
	{
		check(DetectType(Code,Position) == CT_MultiLine);

		for (int i = Position + 2; i < Code.Len(); ++i)
		{
			// Ignore line breaks cause we are looking for an exact pattern rather than for an explicit line ending
			{
				const int AfterNewLine = ConsumeNewLine(Code, i);
				if (AfterNewLine != INDEX_NONE) [[unlikely]]
				{
					OnNewLine(AfterNewLine - 1);
					i = AfterNewLine - 1;
					continue;
				}
			}

			const bool bFarEnough = i > Position + 3; // Ensure "/*/" is not accidentally accepted
			if (Code[i] == TCHAR('/') && Code[i - 1] == TCHAR('*') && bFarEnough) [[unlikely]]
				return i + 1;
		}
		return INDEX_NONE;
	}

	constexpr int MultiLineEnding(const sketch::FStringView& Code, int Position)
	{
		return MultiLineEnding(Code, Position, [](int) {});
	}

	/** @return Index of the first character that doesn't belong to a comment, Code.Len() if all remaining code is a comment, or INDEX_NONE if comment is malformed */
	constexpr int Skip(const sketch::FStringView& Code, int Position, const auto& OnNewLine)
	{
		// Keep in mind one comment can be followed by another
		int i;
		for (i = Position; Code.IsValidIndex(i);)
		{
			const ECommentType CommentType = DetectType(Code, i);
			switch (CommentType)
			{
			case CT_None: return i;
			case CT_SingleLine:
				i = Comment::SingleLineEnding(Code, i, OnNewLine);
				break;
			case CT_MultiLine:
				i = Comment::MultiLineEnding(Code, i, OnNewLine);
				break;
			}
		}
		return i;
	}

	constexpr auto GSkip = [](const sketch::FStringView& Code, int Position, const auto& OnNewLine) { return Comment::Skip(Code, Position, OnNewLine); };

	constexpr int Filter(const sketch::FStringView& Code, int Position) { return Skip(Code, Position, [](int) {}); }
}

/*************************************************************************************************/
// Filtration basics //
/*************************************************************************************************/
namespace sketch::SourceCode::Private
{
	template <bool bRoot = true>
	constexpr int Combine(const sketch::FStringView& Code, int Position, const auto& Invoker, const auto& First, const auto&... Rest)
	{
		// Each time a filter filters something out - restart applying the whole filter stack from zero
		// Including the very first one
		// 'Cause we don't know if a particular filter filters everything or just the first match
		// Repeat filtration til a first clear run when all filters are satisfied of where we are right now
		for (int CurrentPosition = Position; CurrentPosition < Code.Len();)
		{
			// Apply filter
			const int AfterThis = Invoker(Code, CurrentPosition, First);

			// I'm any filter and I ran into a source code error
			if (AfterThis == INDEX_NONE) [[unlikely]] return INDEX_NONE;

			if (AfterThis != CurrentPosition)
			{
				// I'm the very first filter and I filtered out something - continue
				if constexpr (bRoot)
				{
					CurrentPosition = AfterThis;
					continue;
				}
				// I'm some filter in the stack and I filtered out something - tell the root about it
				else
				{
					return AfterThis;
				}
			}

			// I'm the very last filter and I'm satisfied
			if constexpr (sizeof...(Rest) == 0)
			{
				return AfterThis;
			}
			// I'm a satisfied filter, but there are more filters ahead - ask their opinion on where we are
			else
			{
				const int AfterRest = Private::Combine<false>(Code, CurrentPosition, Invoker, Rest...);

				// Some filter ran into a source code error
				if (AfterRest == INDEX_NONE) [[unlikely]] return INDEX_NONE;

				// Everyone agrees on where we are right now!
				if (AfterRest == CurrentPosition) return CurrentPosition;

				// Someone advanced further and told the root about it
				if constexpr (bRoot)
				{
					CurrentPosition = AfterRest;
					continue;
				}
				// Someone advanced further - tell root about it
				else
				{
					return AfterRest;
				}
			}
		}

		// We filtered EVERYTHING out
		return Code.Len();
	}
}

namespace sketch::SourceCode
{
	template <class T>
	concept CSkip = requires { std::declval<T>()(sketch::FStringView(), 0, [](int) {}); };

	constexpr int SkipAll(const sketch::FStringView& Code, int Position, const auto& OnNewLine, const CSkip auto& FirstSkip, const CSkip auto&... Skips)
	{
		return Private::Combine(
			Code,
			Position,
			[&OnNewLine](const sketch::FStringView& Code, int Position, const auto& Skip) constexpr { return Skip(Code, Position, OnNewLine); },
			FirstSkip,
			Skips...
		);
	}

	/**
	 * A filter must return:
	 * - Provided position if it's not currently applicable
	 * - Index of the first character that doesn't satisfy the filter
	 * - INDEX_NONE when filter is applicable but something is broken
	 */
	template <class T>
	concept CFilter = std::is_same_v<int, std::invoke_result_t<T, sketch::FStringView, int>>;

	constexpr int NoFilter(const sketch::FStringView&, int Pos) { return Pos; };

	constexpr int CombineFilters(const sketch::FStringView& Code, int Position, CFilter auto&& FirstFilter, CFilter auto&&... Filters)
	{
		return Private::Combine(
			Code,
			Position,
			[](const sketch::FStringView& InCode, int InPosition, const auto& Filter) constexpr { return Filter(InCode, InPosition); },
			FirstFilter,
			Filters...
		);
	}

	constexpr auto CombinedFilter(CFilter auto&& FirstFilter, CFilter auto&&... Filters)
	{
		return [First = MoveTemp(FirstFilter), ...Rest = MoveTemp(Filters)](const sketch::FStringView& Code, int Position) -> int
		{
			return Private::Combine(
				Code,
				Position,
				[](const sketch::FStringView& InCode, int InPosition, const auto& Filter) constexpr { return Filter(InCode, InPosition); },
				First,
				Rest...
			);
		};
	}
}

/*************************************************************************************************/
// String //
/*************************************************************************************************/

namespace sketch::SourceCode::String
{
	constexpr bool IsLiteralEdge(const sketch::FStringView& Code, int Position)
	{
		return Code[Position] == TCHAR('"') && !(Code.IsValidIndex(Position - 1) && Code[Position - 1] == TCHAR('\\'));
	}

	/** @return Index of first character that doesn't belong to a string literal and is not a whitespace, or INDEX_NONE if string literal is broken */
	constexpr int LiteralEnding(const sketch::FStringView& Code, int Position, const auto& OnNewLine)
	{
		check(IsLiteralEdge(Code, Position));

		// Keep in mind one literal can be followed by another and - if so - they form a single literal
		bool bIsInLiteral = true;
		int i;
		for (i = Position + 1; i < Code.Len(); ++i)
		{
			if (bIsInLiteral)[[likely]]
			{
				// Consider line breaks
				const int AfterLineBreak = ConsumeLineBreak(Code, i);
				if (Code.IsValidIndex(AfterLineBreak))[[unlikely]]
				{
					OnNewLine(AfterLineBreak - 1);
					i = AfterLineBreak - 1;
					continue;
				}

				// New lines are illegal inside strings, only line breaks
				if (Code.IsValidIndex(ConsumeNewLine(Code, i)))[[unlikely]]
				{
					return INDEX_NONE;
				}
			}
			else
			{
				// Report new lines ignoring if they are actually line brakes
				const int AfterNewLine = ConsumeNewLine(Code, i);
				if (Code.IsValidIndex(AfterNewLine))[[unlikely]]
				{
					OnNewLine(AfterNewLine - 1);
					i = AfterNewLine - 1;
					continue;
				}

				// String literal in not over until last quote not followed by another
				if (!IsWhitespace(Code[i]))
				{
					break;
				}
			}


			if (IsLiteralEdge(Code, i))
			{
				bIsInLiteral = !bIsInLiteral;
			}
		}
		return bIsInLiteral ? INDEX_NONE : i;
	}

	constexpr int SkipLiteral(const sketch::FStringView& Code, int Position, const auto& OnNewLine)
	{
		if (IsLiteralEdge(Code, Position))
			return LiteralEnding(Code, Position, OnNewLine);
		return Position;
	}

	constexpr auto GSkipLiteral = [](const sketch::FStringView& Code, int Position, const auto& OnNewLine) { return String::SkipLiteral(Code, Position, OnNewLine); };

	constexpr int LiteralFilter(const sketch::FStringView& Code, int Position) { return SkipLiteral(Code, Position, [](int) {}); }

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive, CFilter FilterType = decltype(&NoFilter)>
	constexpr int Find(
		const sketch::FStringView& Code,
		int InitialPosition,
		const sketch::FStringView& String,
		const FilterType& CustomFilter = &NoFilter
	)
	{
		const sketch::FStringView SearchedCode = Code.LeftChop(String.Len());
		for (int i = InitialPosition; i <= Code.Len() - String.Len(); ++i)
		{
			i = SourceCode::CombineFilters(SearchedCode, i, &Comment::Filter, CustomFilter);
			if (!SearchedCode.IsValidIndex(i)) [[unlikely]] return INDEX_NONE;

			if (Code.Mid(i, String.Len()).Equals<SearchCase>(String))
				return i;
		}
		return INDEX_NONE;
	}
}

/*************************************************************************************************/
// Bracket //
/*************************************************************************************************/

namespace sketch::SourceCode::Bracket
{
	constexpr int FindPaired(
		const sketch::FStringView& Code,
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
		for (int i = OpeningBracketPosition + 1; i < Code.Len(); ++i)
		{
			i = SourceCode::SkipAll(Code, i, OnNewLine, Comment::GSkip, String::GSkipLiteral);
			if (!Code.IsValidIndex(i)) [[unlikely]] return INDEX_NONE;

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

	constexpr int FindPaired(
		const sketch::FStringView& Code,
		int OpeningBracketPosition,
		TCHAR OpeningBracket,
		TCHAR ClosingBracket)
	{
		return FindPaired(Code, OpeningBracketPosition, OpeningBracket, ClosingBracket, [](int) {}, [](int) {}, [](int) {});
	}

	template <TCHAR OpeningBracket, TCHAR ClosingBracket>
	constexpr int Filter(const sketch::FStringView& Code, int Position)
	{
		if (Code[Position] == OpeningBracket)
		{
			const int Result = FindPaired(Code, Position, OpeningBracket, ClosingBracket);
			return Result == INDEX_NONE ? INDEX_NONE : Result + 1;
		}
		return Position;
	}

	constexpr int SubscopeFilter(const sketch::FStringView& Code, int Position) { return Filter<TCHAR('{'), TCHAR('}')>(Code, Position); }
	constexpr int ArgumentFilter(const sketch::FStringView& Code, int Position) { return Filter<TCHAR('('), TCHAR(')')>(Code, Position); }
	constexpr int TemplateFilter(const sketch::FStringView& Code, int Position) { return Filter<TCHAR('<'), TCHAR('>')>(Code, Position); }
	constexpr int SubscriptFilter(const sketch::FStringView& Code, int Position) { return Filter<TCHAR('['), TCHAR(']')>(Code, Position); }
	constexpr int AnySubscopeFilter(const sketch::FStringView& Code, int Position) { return CombineFilters(Code, Position, &SubscopeFilter, &ArgumentFilter, &TemplateFilter, &SubscriptFilter); }
}

/*************************************************************************************************/
// Filter library //
/*************************************************************************************************/
namespace sketch::SourceCode
{
	/**
	 * Finds first character that is not an empty space and is not a part of a comment
	 * @return INDEX_NONE on source code errors
	 */
	constexpr int NoneCodeFilter(const sketch::FStringView& Code, int InitialPosition = 0)
	{
		for (int i = InitialPosition; i < Code.Len(); ++i)
		{
			if (IsWhitespace(Code[i])) continue;
			if (IsControl(Code[i])) continue;

			const int AfterComments = Comment::Filter(Code, i);
			if (AfterComments == INDEX_NONE) [[unlikely]] return INDEX_NONE;
			if (AfterComments != i)
			{
				i = AfterComments - 1;
				continue;
			}

			return i;
		}
		return Code.Len();
	}

	constexpr int NameFilter(const sketch::FStringView& Code, int InitialPosition = 0)
	{
		for (int i = InitialPosition; i < Code.Len(); ++i)
		{
			if (!IsNameChar(Code[i])) return i;
		}
		return Code.Len();
	}

	/** 
	 * Returns index of the first character after a complete punctuational expression.
	 * That is, it doesn't concat characters that aren't normally combined, e.g '.' or '?'
	 */
	constexpr int OperatorFilter(const sketch::FStringView& Code, int InitialPosition = 0)
	{
		auto IsAlwaysAlone = [](TCHAR Char)
		{
			auto AlwaysAlone = {
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
			return InitialPosition + 1;
		}
		for (int i = InitialPosition; i < Code.Len(); ++i)
		{
			if (!IsPunctChar(Code[i]) || IsAlwaysAlone(Code[i])) return i + 1;
		}
		return Code.Len();
	}

	constexpr int WordFilter(const sketch::FStringView& Code, int InitialPosition = 0)
	{
		return IsNameChar(Code[InitialPosition]) ? NameFilter(Code, InitialPosition) : OperatorFilter(Code, InitialPosition);
	}
}

/*************************************************************************************************/
// Pattern matching //
/*************************************************************************************************/
namespace sketch::SourceCode
{
	struct FLocalStringView
	{
		int FirstOf = INDEX_NONE;
		int FirstAfter = INDEX_NONE;
		TDevelopmentOnly<sketch::FStringView{}> DebugView;

		static constexpr FLocalStringView Make(int FirstOf, int FirstAfter, const sketch::FStringView& Code) { return { FirstOf, FirstAfter, Code.Mid(FirstOf, FirstAfter - FirstOf) }; }
		constexpr bool IsValid() const { return FirstOf != INDEX_NONE && FirstAfter != INDEX_NONE && FirstAfter > FirstOf; }
		constexpr sketch::FStringView ToView(const sketch::FStringView& Code) const { return IsValid() ? sketch::FStringView{ &Code[0] + FirstOf, FirstAfter - FirstOf } : sketch::FStringView{}; }
	};

	template <int InTag = -1, class... T>
	struct TMatch;

	template <class T>
	struct TIsMatch
	{
		enum { Value = false };
	};

	template <int Tag, class... T>
	struct TIsMatch<TMatch<Tag, T...>>
	{
		enum { Value = true };
	};

	template <class T>
	concept CMatch = TIsMatch<T>::Value == true;

	enum EMatchResult
	{
		MR_Skipped,
		MR_Success,
		MR_Failure,
		MT_UndesiredMatch,
	};

	struct FMatchHeader
	{
		FLocalStringView Value;
		EMatchResult MatchResult = MR_Skipped;
		TDevelopmentOnly<(const ANSICHAR*)nullptr> DebugSource;
		TDevelopmentOnly<sketch::FStringView{}> DebugName;

		constexpr sketch::FStringView View(const sketch::FStringView& Code) const { return Value.ToView(Code); }
	};

	template <int InTag, CMatch... T>
	struct TMatch<InTag, T...> : FMatchHeader
	{
		static constexpr int Tag = InTag;
		[[no_unique_address]]
		sketch::TTuple<T...> Components;

		template <int Tag, bool bRoot = true>
		constexpr auto& Get()
		{
			if constexpr (sizeof...(T) > 0)
			{
				auto& Result = Components.template TryGet<[]<class ItemType>() constexpr { return ItemType::Tag == Tag; }>();
				using FFailureType = sketch::TTuple<>&;
				if constexpr (!std::is_same_v<decltype(Result), FFailureType>)
				{
					return Result;
				}
				else
				{
					auto Filter = [&](auto&... Component) -> auto&
					{
						auto Results = sketch::Tie(Component.template Get<Tag, false>()...);
						auto& ChildResult = Results.template TryGet<[]<class ItemType>() constexpr { return !std::is_same_v<ItemType, FFailureType>; }>();
						constexpr bool bFailure = std::is_same_v<decltype(ChildResult), FFailureType>;
						static_assert(!bRoot || !bFailure, "Unknown tag");
						if constexpr (!bFailure)
						{
							return ChildResult;
						}
						else
						{
							return Results.template Get<0>();
						}
					};
					return Components.ApplyAfter(Filter);
				}
			}
			else
			{
				static_assert(!bRoot, "How did you even get there? Query doesn't make sense since result is empty.");
				return Components;
			}
		}

		template <int Tag>
		constexpr const auto& Get() const { return const_cast<TMatch*>(this)->Get<Tag>(); }

		template <CMatch... ComponentTypes>
		static constexpr auto Make(FLocalStringView Value, sketch::TTuple<ComponentTypes...>&& Components) { return TMatch<InTag, ComponentTypes...>{ Value, MR_Skipped, nullptr, {}, ::MoveTemp(Components) }; }
	};



	enum ESegmentType
	{
		ST_Required,
		ST_Optional,
		ST_MustNotBePresent
	};

	template <class T>
	concept CMatcher = CMatch<std::invoke_result_t<T, sketch::FStringView, int>>;

	template <int InTag, CMatcher T>
	struct TSegment
	{
		/**
		 * @returns
		 * - IndexOfFirstUnmatchedCharacter or ProvidedCode.Len() if there's no such
		 * - InitialProvidedPosition on mismatch
		 * - INDEX_NONE on source code errors
		 */
		[[no_unique_address]]
		T Matcher;
		ESegmentType Type = ST_Required;
		static constexpr int Tag = InTag;

		TDevelopmentOnly<(const ANSICHAR*)nullptr> DebugSource;
		TDevelopmentOnly<sketch::FStringView{}> DebugName;
		TDevelopmentOnly<bool{ false }> bBreakOnEnter;
		TDevelopmentOnly<bool{ false }> bBreakOnMatch;
		TDevelopmentOnly<bool{ false }> bBreakOnFailure;

		constexpr auto SetDebugSource(const ANSICHAR* InDebugSource) -> decltype(*this) { return DebugSource = InDebugSource, *this; }
		constexpr auto SetDebugName(const sketch::FStringView& InDebugName) -> decltype(*this) { return DebugName = InDebugName, *this; }
		constexpr auto BreakOnEnter(bool Value = true) -> decltype(*this) { return bBreakOnEnter = Value, *this; }
		constexpr auto BreakOnMatch(bool Value = true) -> decltype(*this) { return bBreakOnMatch = Value, *this; }
		constexpr auto BreakOnFailure(bool Value = true) -> decltype(*this) { return bBreakOnFailure = Value, *this; }

		FORCEINLINE constexpr void ConsiderBreakOnEnter() const
		{
#if !UE_BUILD_SHIPPING
			if (!std::is_constant_evaluated() && bBreakOnEnter && FPlatformMisc::IsDebuggerPresent())
			{
				PLATFORM_BREAK();
			}
#endif
		}

		FORCEINLINE constexpr void ConsiderBreakOnMatch() const
		{
#if !UE_BUILD_SHIPPING
			if (!std::is_constant_evaluated() && bBreakOnMatch && FPlatformMisc::IsDebuggerPresent())
			{
				PLATFORM_BREAK();
			}
#endif
		}

		FORCEINLINE constexpr void ConsiderBreakOnFailure() const
		{
#if !UE_BUILD_SHIPPING
			if (!std::is_constant_evaluated() && bBreakOnFailure && FPlatformMisc::IsDebuggerPresent())
			{
				PLATFORM_BREAK();
			}
#endif
		}
	};

	template <class>
	struct TIsSegment
	{
		enum { Value = true };
	};

	template <int Tag, class T>
	struct TIsSegment<TSegment<Tag, T>>
	{
		enum { Value = true };
	};

	template <class T>
	concept CSegment = TIsSegment<T>::Value == true;
}

namespace sketch::SourceCode::Private
{
	template <class... T>
	using TMatchOutput = sketch::TTuple<decltype(std::declval<T>().Matcher(sketch::FStringView(), 0))...>;

	template <CSegment FFirstSegment, CSegment... T>
	constexpr FMatchHeader& Match(
		const sketch::FStringView& Code,
		int InitialPosition,
		bool bAlreadyMatchedAnything, // Must always be "false" for initial calls
		TMatchOutput<FFirstSegment, T...>& Output,
		const CFilter auto& CustomFilter,
		const FFirstSegment& FirstSegment,
		const T&... Segments
	)
	{
		// Setup helpers
		auto& MyOutput = Output.template Get<0>();
		MyOutput.Value.FirstOf = InitialPosition;
		auto Result = [&](EMatchResult MatchResult, int NextMatchablePosition) constexpr -> FMatchHeader&
		{
			MyOutput.MatchResult = MatchResult;
			MyOutput.Value.FirstAfter = NextMatchablePosition;
			return MyOutput;
		};

		// Sanitize
		if (Code.IsEmpty()) [[unlikely]] return Result(MR_Failure, INDEX_NONE);

		// Skip all undesired stuff
		const int Position = SourceCode::CombineFilters(Code, InitialPosition, &NoneCodeFilter, CustomFilter);

		// Evaluate current matcher depending on availability of anything to evaluate
		// 'Cause running out of code doesn't necessarily means failure
		bool bMatchedAnything = false;
		if (!Code.IsValidIndex(Position))[[unlikely]]
		{
			switch (FirstSegment.Type)
			{
			case ESegmentType::ST_Required:
				FirstSegment.ConsiderBreakOnFailure();
				return Result(MR_Failure, InitialPosition + int(!bAlreadyMatchedAnything));
			case ESegmentType::ST_Optional:
				Result(MR_Skipped, InitialPosition);
				break;
			case ESegmentType::ST_MustNotBePresent:
				Result(MR_Success, InitialPosition);
				break;
			default:
				unimplemented();
			}
		}
		else
		{
			FirstSegment.ConsiderBreakOnEnter();
			MyOutput = FirstSegment.Matcher(Code, Position);
			MyOutput.DebugSource = FirstSegment.DebugSource;
			MyOutput.DebugName = FirstSegment.DebugName;
			MyOutput.MatchResult = MR_Success;
			switch (FirstSegment.Type)
			{
			case ESegmentType::ST_Required:
				if (!MyOutput.Value.IsValid())
				{
					FirstSegment.ConsiderBreakOnFailure();
					return Result(MR_Failure, Position + int(!bAlreadyMatchedAnything));
				}
				else
				{
					bMatchedAnything = true;
					FirstSegment.ConsiderBreakOnMatch();
				}
				break;
			case ESegmentType::ST_Optional:
				if (!MyOutput.Value.IsValid())
					Result(MR_Skipped, Position);
				else
					bMatchedAnything = true;
				break;
			case ESegmentType::ST_MustNotBePresent:
				// Return position of undesired match, not after it
				// Because full pattern is likely to have other items before undesired one
				// Which can find this particular code segment quite handy
				// In case there are none - problem is easier to solve for the caller then here
				if (MyOutput.Value.IsValid()) return Result(MT_UndesiredMatch, Position + int(!bAlreadyMatchedAnything));
				break;
			default:
				unimplemented();
			}
		}

		// Execute the rest of matchers
		if constexpr (sizeof...(T) > 0)
		{
			// Ensure this is returned as last successful match on optional tail mismatch
			FMatchHeader& Rest = Private::Match(Code, MyOutput.Value.FirstAfter, bAlreadyMatchedAnything || bMatchedAnything, Output.Next, CustomFilter, Segments...);
			return Rest.MatchResult != MR_Skipped ? Rest : MyOutput;
		}
		else
		{
			return MyOutput;
		}
	}
}

namespace sketch::SourceCode
{
	template <class... T>
	using TMatchTypeFromSegments = TMatch<-1, decltype(std::declval<T>().Matcher(sketch::FStringView(), 0))...>;

	template <CSegment FFirstSegment, CSegment... T>
	constexpr TMatchTypeFromSegments<FFirstSegment, T...> Match(
		const sketch::FStringView& Code,
		int InitialPosition,
		const CFilter auto& CustomFilter,
		const FFirstSegment& FirstSegment,
		const T&... Segments
	)
	{
		TMatchTypeFromSegments<FFirstSegment, T...> Result;
		const FMatchHeader& MatchResult = Private::Match(Code, InitialPosition, false, Result.Components, CustomFilter, FirstSegment, Segments...);
		// Mismatch of a trailing optional means that trailing optional could be successfully reached
		// Hence, trailing skip is a success
		Result.MatchResult = MatchResult.MatchResult == MR_Skipped ? MR_Success : MatchResult.MatchResult;
		Result.Value = FLocalStringView::Make(InitialPosition, MatchResult.Value.FirstAfter, Code);
		return Result;
	}

	template <CFilter AdvancementFilterType = decltype(&NoFilter), CFilter MatcherFilterType = decltype(&NoFilter), CSegment... SegmentTypes>
	struct TMatcher
	{
		constexpr TMatcher(
			FStringView InCode,
			int InPosition,
			AdvancementFilterType&& InAdvancementFilter,
			MatcherFilterType&& InMatcherFilter,
			SegmentTypes&&... InSegments
		)
			: Code{ InCode }
			, Position{ InPosition }
			, MatcherFilter{ ::MoveTemp(InMatcherFilter) }
			, AdvancementFilter{ ::MoveTemp(InAdvancementFilter) }
			, Segments{ ::MoveTemp(InSegments)... }
		{
			operator++();
		}

		constexpr TMatcher(FStringView InCode, MatcherFilterType&& InMatcherFilter, SegmentTypes&&... InSegments)
			: Code{ InCode }
			, Position{ 0 }
			, MatcherFilter{ MoveTemp(InMatcherFilter) }
			, AdvancementFilter{ &NoFilter }
			, Segments{ ::MoveTemp(InSegments)... }
		{
			operator++();
		}

		constexpr TMatcher(FStringView InCode, SegmentTypes&&... InSegments)
			: Code{ InCode }
			, Position{ 0 }
			, MatcherFilter{ &NoFilter }
			, AdvancementFilter{ &NoFilter }
			, Segments{ ::MoveTemp(InSegments)... }
		{
			operator++();
		}

		constexpr TMatcher& operator++()
		{
			auto CallMatch = [&]<class... T>(const T&... Segment) constexpr
			{
				Match = SourceCode::Match(Code, Position, MatcherFilter, Segment...);
			};
			for (;;)
			{
				Position = SourceCode::CombineFilters(Code, Position, &NoneCodeFilter, AdvancementFilter);
				if (!Code.IsValidIndex(Position))
				{
					Match.MatchResult = MR_Failure;
					break;
				}

				Segments.ApplyAfter(CallMatch);
				Position = Match.Value.FirstAfter;
				if (Match.MatchResult == MR_Success) break;
				if (!Code.IsValidIndex(Position)) break;
			}
			return *this;
		}

		constexpr operator bool() const
		{
			// Do not rely on position 'cause position may be already invalid after last successful match
			return Match.MatchResult == MR_Success;
		}



		template <int Tag>
		EMatchResult ResultOf() const { return Match.template Get<Tag>().MatchResult; }

		template <int Tag>
		bool Matched() const { return Match.template Get<Tag>().MatchResult == MR_Success; }

		template <int Tag>
		sketch::FStringView View() const { return Match.template Get<Tag>().View(Code); }

		template <int Tag>
		FString String() const { return Match.template Get<Tag>().View(Code).ToString(); }



		const sketch::FStringView Code;
		TMatchTypeFromSegments<SegmentTypes...> Match;
		int Position = -1;
		MatcherFilterType MatcherFilter;
		AdvancementFilterType AdvancementFilter;
		sketch::TTuple<SegmentTypes...> Segments;
	};
}

/*************************************************************************************************/
// Elementary matchers //
/*************************************************************************************************/
namespace sketch::SourceCode::Matcher
{
	template <int Tag>
	constexpr TMatch<Tag> MakeMatch(const sketch::FStringView& Code, int InitialPosition, int Position)
	{
		return Position != INDEX_NONE ? TMatch<Tag>{ FLocalStringView::Make(InitialPosition, Position, Code) } : TMatch<Tag>{};
	}

	template <int Tag>
	constexpr auto SegmentFromFilter(CFilter auto&& Filter, ESegmentType Type = ST_Required, const ANSICHAR* DebugSource = nullptr)
	{
		auto Handler = [F = ::MoveTempIfPossible(Filter)](const sketch::FStringView& Code, int Position) constexpr
		{
			return Matcher::MakeMatch<Tag>(Code, Position, F(Code, Position));
		};
		return TSegment<Tag, decltype(Handler)>(::MoveTemp(Handler), Type, DebugSource);
	}

	template <int Tag>
	constexpr TMatch<Tag> MakeMismatch(int InitialPosition) { return TMatch<Tag>{ FLocalStringView{ InitialPosition, InitialPosition } }; }

	template <int Tag = -1>
	constexpr auto OneOf(ESegmentType Type, CSegment auto&&... Segments)
	{
		auto Handler = [S = sketch::MakeTuple(MoveTemp(Segments)...)](const sketch::FStringView& Code, int Position) constexpr
		{
			Private::TMatchOutput<decltype(Segments)...> ResultComponents;
			FLocalStringView ResultView;
			[&]<int... Index>(std::integer_sequence<int, Index...>) constexpr
			{
				bool bContinue = true;
				auto Next = [&](const auto& Segment, auto& Result) constexpr
				{
					if (!bContinue) return;
					Segment.ConsiderBreakOnEnter();
					Result = Segment.Matcher(Code, Position);
					ResultView = Result.Value;
					bContinue = !Result.Value.IsValid();
					Result.MatchResult = bContinue ? MR_Failure : MR_Success;
					Result.Value.IsValid() ? Segment.ConsiderBreakOnMatch() : Segment.ConsiderBreakOnFailure();
				};
				(Next(S.template Get<Index>(), ResultComponents.template Get<Index>()), ...);
			}(std::make_integer_sequence<int, sizeof...(Segments)>{});
			return TMatch<Tag>::Make(ResultView, MoveTemp(ResultComponents));
		};
		return TSegment<Tag, decltype(Handler)>(::MoveTemp(Handler), Type, __func__);
	}

	template <int Tag = -1>
	constexpr auto OneOf(CSegment auto&&... Segments) { return Matcher::OneOf<Tag>(ST_Required, MoveTemp(Segments)...); }

	template <int Tag = -1>
	constexpr auto Subsequence(ESegmentType Type, CFilter auto&& CustomFilter, CSegment auto&&... Segments)
	{
		auto Handler = [F = ::MoveTemp(CustomFilter), ...S = ::MoveTemp(Segments)](const sketch::FStringView& Code, int Position) constexpr
		{
			// Use regular matcher, but don't allow it increment initial position on failing matching the very first item
			TMatchTypeFromSegments<decltype(S)...> Result;
			const FMatchHeader& MatchResult = Private::Match(Code, Position, true, Result.Components, F, S...);
			// Mismatch of a trailing optional means that trailing optional could be successfully reached
			// Hence, trailing skip is a success
			Result.MatchResult = MatchResult.MatchResult == MR_Skipped ? MR_Success : MatchResult.MatchResult;
			Result.Value = FLocalStringView::Make(Position, MatchResult.Value.FirstAfter, Code);
			return TMatch<Tag>::Make(
				FLocalStringView::Make(Position, Result.MatchResult == MR_Success ? Result.Value.FirstAfter : Position, Code),
				::MoveTemp(Result.Components)
			);
		};
		return TSegment<Tag, decltype(Handler)>(::MoveTemp(Handler), Type, __func__);
	}

	template <int Tag = -1>
	constexpr auto Subsequence(CSegment auto&&... Segments) { return Matcher::Subsequence<Tag>(ST_Required, &NoFilter, MoveTemp(Segments)...); }

	template <int Tag = -1>
	constexpr auto Subsequence(ESegmentType Type, CSegment auto&&... Segments) { return Matcher::Subsequence<Tag>(Type, &NoFilter, MoveTemp(Segments)...); }

	template <int Tag = -1, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr auto String(const sketch::FStringView& String, ESegmentType Type = ST_Required)
	{
		auto Handler = [String](const sketch::FStringView& Code, int Position) constexpr -> TMatch<Tag>
		{
			if (Code.RightChop(Position).StartsWith<SearchCase>(String))
			{
				return MakeMatch<Tag>(Code, Position, Position + String.Len());
			}
			return MakeMismatch<Tag>(Position);
		};
		return TSegment<Tag, decltype(Handler)>(::MoveTemp(Handler), Type, __func__).SetDebugName(String);
	}

	template <int Tag = -1>
	constexpr auto Digit(ESegmentType Type = ST_Required)
	{
		auto Handler = [](const sketch::FStringView& Code, int Position) constexpr -> TMatch<Tag>
		{
			return MakeMatch<Tag>(Code, Position, Code[Position] >= TCHAR('0') && Code[Position] <= TCHAR('9') ? Position + 1 : Position);
		};
		return TSegment<Tag, decltype(Handler)>(::MoveTemp(Handler), Type, __func__);
	}

	template <int Tag = -1>
	constexpr auto StringLiteral(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&String::LiteralFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto Name(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&NameFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto Operator(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&OperatorFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto Subscope(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&Bracket::SubscopeFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto Arguments(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&Bracket::ArgumentFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto TemplateArguments(ESegmentType Type = ST_Required) { return SegmentFromFilter<Tag>(&Bracket::TemplateFilter, Type, __func__); }

	template <int Tag = -1>
	constexpr auto ModuleApi(ESegmentType Type = ST_Optional)
	{
		auto Handler = [](const sketch::FStringView& Code, int Position)constexpr -> TMatch<Tag>
		{
			const int AfterWord = NameFilter(Code, Position);
			sketch::FStringView Word = Code.Mid(Position, AfterWord - Position);
			return MakeMatch<Tag>(Code, Position, Word.EndsWith(TEXT("_API")) ? AfterWord : Position);
		};
		return TSegment<Tag, decltype(Handler)>(MoveTemp(Handler), Type, __func__);
	}

	template <int Tag = -1>
	constexpr auto Alignment(ESegmentType Type = ST_Optional)
	{
		return Matcher::Subsequence(
			Type,
			&NoFilter,
			String(TEXT("alignas")),
			Arguments<Tag>()
		).SetDebugSource(__func__);
	}
}

#define Demangle(Expression)\
	[&]() constexpr\
	{\
		struct FDemangled##__LINE__ : std::decay_t<decltype(Expression)> {};\
		return FDemangled##__LINE__ { MoveTempIfPossible(Expression) };\
	}()

/*************************************************************************************************/
// Complex matchers //
/*************************************************************************************************/
namespace sketch::SourceCode::Matcher
{
	struct FClassNameTags
	{
		int TemplateArgumentsTag = -1;
		int ClassNameTag = -1;
		int TemplateSpecializationTag = -1;
		int FirstCharAfterClassNameTag = -1;
	};

	template <FClassNameTags Tags, CFilter FilterType = decltype(&NoFilter)>
	constexpr auto ClassDeclaration(FilterType&& Filter = &NoFilter, ESegmentType Type = ST_Required)
	{
		return Matcher::Subsequence(
			Type,
			::MoveTemp(Filter),
			Matcher::Subsequence(
				ST_Optional,
				&NoFilter,
				Matcher::String(TEXT("template")),
				Matcher::TemplateArguments<Tags.TemplateArgumentsTag>()
			),
			Matcher::ModuleApi(),
			Matcher::OneOf(
				Matcher::String(TEXT("class")),
				Matcher::String(TEXT("struct"))
			),
			Matcher::ModuleApi(),
			Matcher::Alignment(),
			Matcher::ModuleApi(),
			Matcher::Subsequence(
				ST_Optional,
				Matcher::String(TEXT("UE_DEPRECATED")),
				Matcher::Arguments()
			),
			Matcher::Name<Tags.ClassNameTag>().SetDebugName(TEXT("ClassName")),
			Matcher::TemplateArguments<Tags.TemplateSpecializationTag>(ST_Optional),
			Matcher::Subsequence( // Unfiltered subsequence always allows '{' matching
				Matcher::OneOf<Tags.FirstCharAfterClassNameTag>(
					Matcher::String(TEXT(":")),
					Matcher::String(TEXT("{")),
					Matcher::String(TEXT(";"))
				)
			).SetDebugName(TEXT("FirstCharAfter"))
		).SetDebugSource(__func__);
	}

	template <int Tag = -1>
	constexpr auto TypeName(ESegmentType Type = ST_Required)
	{
		// A rather simple implementation
		// But which covers all required cases
		return Matcher::Subsequence<Tag>(
			Matcher::String(TEXT("const"), ST_Optional),
			// Matcher::String(TEXT("volatile"), ST_Optional),
			Matcher::Name(),
			Matcher::Subsequence(
				ST_Optional,
				Matcher::String(TEXT("::")),
				Matcher::Name(),
				Matcher::Subsequence(
					ST_Optional,
					Matcher::String(TEXT("::")),
					Matcher::Name(),
					Matcher::Subsequence(
						ST_Optional,
						Matcher::String(TEXT("::")),
						Matcher::Name(),
						Matcher::Subsequence(
							ST_Optional,
							Matcher::String(TEXT("::")),
							Matcher::Name()
						)
					)
				)
			),
			Matcher::TemplateArguments(ST_Optional),
			Matcher::Arguments(ST_Optional),
			Matcher::String(TEXT("*"), ST_Optional),
			Matcher::String(TEXT("&"), ST_Optional)
		).SetDebugSource(__func__);
	}
}

/*************************************************************************************************/
// Utils //
/*************************************************************************************************/

namespace sketch::SourceCode
{
	SKETCH_API FProcessedString CleanCode(const sketch::FStringView& Code);



	struct FArgument
	{
		FLocalStringView Type;
		FLocalStringView Name;
		FLocalStringView DefaultValue;
	};

	SKETCH_API TArray<FArgument> ParseArguments(const sketch::FStringView& Code);
}
