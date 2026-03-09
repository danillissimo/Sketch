#include "HeaderTool/SourceCodeUtility.h"

#include "HeaderTool/StringLiteral.h"

sketch::SourceCode::FProcessedString sketch::SourceCode::CleanCode(const sketch::FStringView& Code)
{
	// Collect all comments
	TArray<FInt32Interval, TInlineAllocator<8>> Comments;
	for (int i = 0; i < Code.Len();)
	{
		// Filter out string literals
		i = String::LiteralFilter(Code, i);
		if (!Code.IsValidIndex(i)) [[unlikely]] break;

		const int AfterComment = Comment::Filter(Code, i);
		if (AfterComment != i)
		{
			const int LastCommentCharIndex = AfterComment == INDEX_NONE ? Code.Len() - 1 : AfterComment - 1;
			Comments.Emplace(i, LastCommentCharIndex);
			i = AfterComment;
		}
		else
		{
			++i;
		}
	}

	// If there are no comments - just trim whitespaces
	if (Comments.IsEmpty()) return FProcessedString{ Code.TrimStartAndEnd() };

	// If there are comments - concat together everything between them
	FString Result;
	Result = Code.Mid(Comments[0].Min, Comments[0].Min).TrimStartAndEnd().ToCommonStringView();
	for (int i = 0; i < Comments.Num() - 1; ++i)
	{
		Result += Code.Mid(Comments[i].Max + 1, Comments[i + 1].Min - Comments[i].Max - 1).TrimStartAndEnd();
	}
	Result += Code.Mid(Comments.Last().Max + 1, Code.Len() - Comments.Last().Max - 1).TrimStartAndEnd();
	return { Result, MoveTemp(Result) };
}

TArray<sketch::SourceCode::FArgument> sketch::SourceCode::ParseArguments(const sketch::FStringView& Code)
{
	TArray<FArgument> Result;
	Result.Reserve(6);

	static constexpr auto EverythingUntilComma = [](const sketch::FStringView& Code, int Position) constexpr -> int
	{
		for (int i = Position; i < Code.Len(); ++i)
		{
			if (Code[i] != TCHAR(',')) return i;
		}
		return Code.Len();
	};
	enum { TypeTag, NameTag, AssignmentTag, CommaTag };
	auto Argument = Demangle(
			TMatcher(
				Code,

				// Argument type
				Matcher::TypeName<TypeTag>(),

				// Argument name
				Matcher::Name<NameTag>(),

				// Default argument value
				// It can be expression of any complexity
				// But it starts with a '=', and ends with provided string view or with first comma that is not within any subscope
				Matcher::Subsequence(
					ST_Optional,
					Matcher::String<AssignmentTag>(TEXT("=")),
					Matcher::Subsequence(
						ST_Optional,
						// Yes, this will fail if default value contains any kind of comparisons in its top-level scope
						// But templates are kinda undetectable without full code analysis
						CombinedFilter(&Bracket::ArgumentFilter, &Bracket::TemplateFilter, &Bracket::SubscopeFilter, CopyTemp(EverythingUntilComma)),
						Matcher::String<CommaTag>(SL",")
					)
				)
			)
		);
	bool bLastArgumentProcessed = false;
	for (; Argument; ++Argument)
	{
		checkf(!bLastArgumentProcessed, SL"Only the very last argument default value is not expected to be followed by comma");
		FArgument& Arg = Result.Emplace_GetRef();
		Arg.Type = Argument.Match.Get<TypeTag>().Value;
		Arg.Name = Argument.Match.Get<NameTag>().Value;
		if (Argument.Match.Get<AssignmentTag>().MatchResult == MR_Success)
		{
			if (Argument.Match.Get<CommaTag>().MatchResult == MR_Success)
			{
				Arg.DefaultValue = {
					Argument.Match.Get<AssignmentTag>().Value.FirstAfter,
					Argument.Match.Get<CommaTag>().Value.FirstOf
				};
			}
			else
			{
				Arg.DefaultValue = { Argument.Match.Get<AssignmentTag>().Value.FirstAfter, Code.Len() };
				bLastArgumentProcessed = true;
			}
		}
	}
	return Result;
}
