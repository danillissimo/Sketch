// Copyright 2026 danillissimo

#pragma once
#include <string>

#include "Containers/StringView.h"
#include "Containers/UnrealString.h"

namespace sketch
{
	/** @note Copied from TChar */
	constexpr uint32 ToUnsigned(TCHAR Char)
	{
		return static_cast<TUnsignedIntType<sizeof(TCHAR)>::Type>(Char);
	}

	/** @note Copied from TChar */
	constexpr TCHAR ToLower(TCHAR Char)
	{
		return static_cast<TCHAR>(ToUnsigned(Char) + ((static_cast<uint32>(Char) - 'A' < 26u) << 5));
	}
}

/**
 * @note Initially sketch::FStringView, but it inevitably caused conflicts with regular FStringView.
 *       And, to prevent any further possible conflicts, it was renamed so it will never conflict with anything.
 */
struct FSketchStringView
{
	///
	/// Getters
	///
	constexpr const TCHAR* GetData() const { return DataPtr; }
	constexpr int Len() const { return Size; }
	constexpr bool IsEmpty() const { return Size == 0; }
	constexpr bool IsValidIndex(int Index) const { return Index >= 0 && Index < Len(); }
	constexpr const TCHAR& operator[](int Index) const { return DataPtr[Index]; }
	FString ToString() const { return FString::ConstructFromPtrSize(DataPtr, Size); }
	constexpr ::FStringView ToCommonStringView() const { return ::FStringView(DataPtr, Size); }
	constexpr operator ::FStringView() const { return ::FStringView(DataPtr, Size); }


	///
	/// Constructors
	///
	constexpr FSketchStringView() = default;
	constexpr FSketchStringView(const FSketchStringView& Other) = default;
	constexpr FSketchStringView(FSketchStringView&& Other) = default;
	constexpr FSketchStringView& operator=(const FSketchStringView& Other) = default;
	constexpr FSketchStringView& operator=(FSketchStringView&& Other) = default;

	constexpr FSketchStringView(const TCHAR* InData)
		: DataPtr(InData)
		, Size(std::char_traits<TCHAR>::length(InData))
	{}

	constexpr FSketchStringView(const TCHAR* InData, int InLength)
		: DataPtr(InData)
		, Size(InLength)
	{}

	FSketchStringView(const FString& String)
		: DataPtr(String.GetCharArray().GetData())
		, Size(String.Len())
	{}



	///
	/// Comparison
	///
	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr bool Equals(const FSketchStringView& Other) const
	{
		if (Size != Other.Size) return false;
		for (int i = 0; i < Size; ++i)
		{
			const TCHAR MyChar = SearchCase == ESearchCase::CaseSensitive ? DataPtr[i] : sketch::ToLower(DataPtr[i]);
			const TCHAR OtherChar = SearchCase == ESearchCase::CaseSensitive ? Other[i] : sketch::ToLower(Other[i]);
			if (MyChar != OtherChar) return false;
		}
		return true;
	}

	constexpr bool operator==(const FSketchStringView& Other) const { return Equals(Other); }

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr bool StartsWith(FSketchStringView Prefix) const
	{
		return Prefix.Equals<SearchCase>(Left(Prefix.Len()));
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr bool EndsWith(FSketchStringView Suffix) const
	{
		return Suffix.Equals<SearchCase>(Right(Suffix.Len()));
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr int Find(const FSketchStringView& Search, const int32 StartPosition = 0) const
	{
		const int32 Index = UE::String::FindFirst(RightChop(StartPosition), Search, SearchCase);
		return Index == INDEX_NONE ? INDEX_NONE : Index + StartPosition;
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr bool Contains(const FSketchStringView& Search) const
	{
		return Find<SearchCase>(Search, 0) != INDEX_NONE;
	}

	constexpr int FindLast(TCHAR Char)
	{
		for (int i = Len() - 1; i >= 0; --i)
		{
			if (DataPtr[i] == Char)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}


	///
	/// Manipulators
	///
	constexpr const TCHAR* begin() const { return DataPtr; }
	constexpr const TCHAR* end() const { return DataPtr + Size; }

	/** Returns the left-most part of the view by taking the given number of characters from the left. */
	constexpr FSketchStringView Left(int32 CharCount) const
	{
		return FSketchStringView(DataPtr, FMath::Clamp(CharCount, 0, Size));
	}

	constexpr void LeftInline(int32 CharCount)
	{
		*this = Left(CharCount);
	}

	/** Returns the left-most part of the view by chopping the given number of characters from the right. */
	constexpr FSketchStringView LeftChop(int32 CharCount) const
	{
		return FSketchStringView(DataPtr, FMath::Clamp(Size - CharCount, 0, Size));
	}

	/** Modifies the view by chopping the given number of characters from the right. */
	constexpr void LeftChopInline(int32 CharCount)
	{
		*this = LeftChop(CharCount);
	}

	/** Returns the middle part of the view by taking up to the given number of characters from the given position. */
	constexpr FSketchStringView Mid(int32 Position, int32 CharCount) const
	{
		const TCHAR* CurrentStart = GetData();
		const int32 CurrentLength = Len();

		// Clamp minimum position at the start of the string, adjusting the length down if necessary
		const int32 NegativePositionOffset = (Position < 0) ? Position : 0;
		CharCount += NegativePositionOffset;
		Position -= NegativePositionOffset;

		// Clamp maximum position at the end of the string
		Position = (Position > CurrentLength) ? CurrentLength : Position;

		// Clamp count between 0 and the distance to the end of the string
		CharCount = FMath::Clamp(CharCount, 0, (CurrentLength - Position));

		FSketchStringView Result;
		Result.DataPtr = CurrentStart + Position;
		Result.Size = CharCount;
		return Result;
	}

	constexpr void MidInline(int32 Position, int32 CharCount)
	{
		*this = Mid(Position, CharCount);
	}

	/** Returns the right-most part of the view by taking the given number of characters from the right. */
	constexpr FSketchStringView Right(int32 CharCount) const
	{
		const int32 OutLen = FMath::Clamp(CharCount, 0, Size);
		return FSketchStringView(DataPtr + Size - OutLen, OutLen);
	}

	constexpr void RightInline(int32 CharCount)
	{
		*this = Right(CharCount);
	}

	/** Returns the right-most part of the view by chopping the given number of characters from the left. */
	constexpr FSketchStringView RightChop(int32 CharCount) const
	{
		const int32 OutLen = FMath::Clamp(Size - CharCount, 0, Size);
		return FSketchStringView(DataPtr + Size - OutLen, OutLen);
	}

	/** Modifies the view by chopping the given number of characters from the left. */
	constexpr void RightChopInline(int32 CharCount)
	{
		*this = RightChop(CharCount);
	}

	constexpr FSketchStringView TrimStart() const
	{
		int32 SpaceCount = 0;
		for (TCHAR Char : *this)
		{
			if (!TChar<TCHAR>::IsWhitespace(Char))
			{
				break;
			}
			++SpaceCount;
		}
		return FSketchStringView(DataPtr + SpaceCount, Size - SpaceCount);
	}

	constexpr FSketchStringView TrimEnd() const
	{
		int32 NewSize = Size;
		while (NewSize && TChar<TCHAR>::IsWhitespace(DataPtr[NewSize - 1]))
		{
			--NewSize;
		}
		return FSketchStringView(DataPtr, NewSize);
	}

	constexpr FSketchStringView TrimStartAndEnd() const
	{
		return TrimStart().TrimEnd();
	}

	constexpr FSketchStringView& TrimStartAndEndInline()
	{
		*this = TrimStartAndEnd();
		return *this;
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr FSketchStringView RemovePrefix(const FSketchStringView& Prefix) const
	{
		if (StartsWith<SearchCase>(Prefix))
		{
			return RightChop(Prefix.Len());
		}
		return *this;
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	constexpr FSketchStringView RemoveSuffix(const FSketchStringView& Suffix) const
	{
		if (EndsWith<SearchCase>(Suffix))
		{
			return LeftChop(Suffix.Len());
		}
		return *this;
	}

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	const FSketchStringView& RemovePrefixInline(const FSketchStringView& Prefix) { return *this = RemovePrefix<SearchCase>(Prefix); }

	template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
	const FSketchStringView& RemoveSuffixInline(const FSketchStringView& Suffix) { return *this = RemoveSuffix<SearchCase>(Suffix); }

	constexpr void Reset()
	{
		DataPtr = nullptr;
		Size = 0;
	}

	// Public so can be template argument
	// private:
	const TCHAR* DataPtr = nullptr;
	int Size = 0;
};

inline uint32 GetTypeHash(const FSketchStringView& String)
{
	return GetTypeHash(String.ToCommonStringView());
}

inline FString& operator+=(FString& A, const FSketchStringView& B)
{
	A += B.ToCommonStringView();
	return A;
}
