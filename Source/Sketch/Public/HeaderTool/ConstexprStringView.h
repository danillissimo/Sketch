#pragma once
#include <string>

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

	struct FStringView
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
		constexpr FStringView() = default;
		constexpr FStringView(const FStringView& Other) = default;
		constexpr FStringView(FStringView&& Other) = default;
		constexpr FStringView& operator=(const FStringView& Other) = default;
		constexpr FStringView& operator=(FStringView&& Other) = default;

		constexpr FStringView(const TCHAR* InData)
			: DataPtr(InData)
			, Size(std::char_traits<TCHAR>::length(InData))
		{}

		constexpr FStringView(const TCHAR* InData, int InLength)
			: DataPtr(InData)
			, Size(InLength)
		{}

		FStringView(const FString& String)
			: DataPtr(String.GetCharArray().GetData())
			, Size(String.Len())
		{}



		///
		/// Comparison
		///
		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		constexpr bool Equals(const FStringView& Other) const
		{
			if (Size != Other.Size) return false;
			for (int i = 0; i < Size; ++i)
			{
				const TCHAR MyChar = SearchCase == ESearchCase::CaseSensitive ? DataPtr[i] : ToLower(DataPtr[i]);
				const TCHAR OtherChar = SearchCase == ESearchCase::CaseSensitive ? Other[i] : ToLower(Other[i]);
				if (MyChar != OtherChar) return false;
			}
			return true;
		}

		bool operator==(const FStringView& Other) const { return Equals(Other); }

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		constexpr bool StartsWith(FStringView Prefix) const
		{
			return Prefix.Equals<SearchCase>(Left(Prefix.Len()));
		}

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		constexpr bool EndsWith(FStringView Suffix) const
		{
			return Suffix.Equals<SearchCase>(Right(Suffix.Len()));
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

		constexpr FStringView Left(int32 CharCount) const
		{
			return FStringView(DataPtr, FMath::Clamp(CharCount, 0, Size));
		}

		constexpr FStringView LeftChop(int32 CharCount) const
		{
			return FStringView(DataPtr, FMath::Clamp(Size - CharCount, 0, Size));
		}

		constexpr void LeftChopInline(int32 CharCount)
		{
			*this = LeftChop(CharCount);
		}

		constexpr FStringView Mid(int32 Position, int32 CharCount) const
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

			FStringView Result;
			Result.DataPtr = CurrentStart + Position;
			Result.Size = CharCount;
			return Result;
		}

		constexpr void MidInline(int32 Position, int32 CharCount)
		{
			*this = Mid(Position, CharCount);
		}

		constexpr FStringView Right(int32 CharCount) const
		{
			const int32 OutLen = FMath::Clamp(CharCount, 0, Size);
			return FStringView(DataPtr + Size - OutLen, OutLen);
		}

		constexpr FStringView RightChop(int32 CharCount) const
		{
			const int32 OutLen = FMath::Clamp(Size - CharCount, 0, Size);
			return FStringView(DataPtr + Size - OutLen, OutLen);
		}

		constexpr void RightChopInline(int32 CharCount)
		{
			*this = RightChop(CharCount);
		}

		constexpr FStringView TrimStart() const
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
			return FStringView(DataPtr + SpaceCount, Size - SpaceCount);
		}

		constexpr FStringView TrimEnd() const
		{
			int32 NewSize = Size;
			while (NewSize && TChar<TCHAR>::IsWhitespace(DataPtr[NewSize - 1]))
			{
				--NewSize;
			}
			return FStringView(DataPtr, NewSize);
		}

		constexpr FStringView TrimStartAndEnd() const
		{
			return TrimStart().TrimEnd();
		}

		constexpr FStringView& TrimStartAndEndInline()
		{
			*this = TrimStartAndEnd();
			return *this;
		}

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		constexpr FStringView RemovePrefix(const FStringView& Prefix) const
		{
			if (StartsWith<SearchCase>(Prefix))
			{
				return RightChop(Prefix.Len());
			}
			return *this;
		}

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		constexpr FStringView RemoveSuffix(const FStringView& Suffix) const
		{
			if (EndsWith<SearchCase>(Suffix))
			{
				return LeftChop(Suffix.Len());
			}
			return *this;
		}

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		const FStringView& RemovePrefixInline(const FStringView& Prefix) { return *this = RemovePrefix<SearchCase>(Prefix); }

		template <ESearchCase::Type SearchCase = ESearchCase::CaseSensitive>
		const FStringView& RemoveSuffixInline(const FStringView& Suffix) { return *this = RemoveSuffix<SearchCase>(Suffix); }

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
}

inline uint32 GetTypeHash(const sketch::FStringView& String)
{
	return GetTypeHash(String.ToCommonStringView());
}

inline FString& operator+=(FString& A, const sketch::FStringView& B)
{
	A += B.ToCommonStringView();
	return A;
}
