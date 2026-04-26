#pragma once
#include <string>
#include <array>

#include "AttributesTraits.h"
#include "Layout/Visibility.h"
#include "Misc/SourceLocation.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

namespace sketch
{
	namespace Private
	{
		struct FEnum
		{
			struct FMember
			{
				int FullNameStart = -1;
				int ShortNameStart = -1;
				int NameEnd = -1;
				uint64 Value = 0;
			};

			bool bClass = false;
			FStringView Name;
			const TCHAR* MembersNames = nullptr;
			const FMember* Members = nullptr;
			int NumMembers = 0;

			FStringView GetFullMemberName(int Index) const { return FStringView(&MembersNames[Members[Index].FullNameStart], Members[Index].NameEnd - Members[Index].FullNameStart); }
			FStringView GetShortMemberName(int Index) const { return FStringView(&MembersNames[Members[Index].ShortNameStart], Members[Index].NameEnd - Members[Index].ShortNameStart); }
			int GetMemberByValue(uint64 Value) const;
		};
	}

	struct FEnumerationAttribute : public TCommonAttributeImplementation<uint64>
	{
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;

		FEnumerationAttribute(uint64 InValue, const Private::FEnum& Enum)
			: TCommonAttributeImplementation<uint64>(InValue)
			, Enum(Enum) {}

		FEnumerationAttribute(const FEnumerationAttribute&) = default;
		FEnumerationAttribute(FEnumerationAttribute&&) = default;

		FText GetSelectedMemberName() const;
		TSharedRef<SWidget> MakeMenu();
		void OnMemberSelected(int Index);

		const Private::FEnum& Enum;
	};

	template <class T>
		requires std::is_enum_v<T>
	struct TAttributeTraits<T>
	{
	private:
		static constexpr char ClosingBrace = _MSC_VER ? '>' : ']';
		static constexpr char OpeningBrace = _MSC_VER ? '<' : ' ';

		static consteval int GetEnumNameSize(const UE::FSourceLocation& SourceLocation = UE::FSourceLocation::Current())
		{
			for (int i = std::char_traits<char>::length(SourceLocation.GetFunctionName()); i >= 0; --i)
			{
				if (SourceLocation.GetFunctionName()[i] == ClosingBrace)
				{
					for (int j = i - 1; j >= 0; --j)
					{
						const char CurrentChar = SourceLocation.GetFunctionName()[j];
						if (CurrentChar == OpeningBrace || CurrentChar == ' ') return i - j - 1;
					}
				}
			}
			return 0;
		}

		static consteval auto GetEnumName()
		{
			constexpr UE::FSourceLocation SourceLocation = UE::FSourceLocation::Current();
			constexpr int Size = GetEnumNameSize(SourceLocation);
			std::array<TCHAR, Size> Result;
			for (int i = std::char_traits<char>::length(SourceLocation.GetFunctionName()); i >= 0; --i)
			{
				if (SourceLocation.GetFunctionName()[i] == ClosingBrace)
				{
					for (int j = 0; j < Size; ++j)
					{
						Result[Size - j - 1] = SourceLocation.GetFunctionName()[i - j - 1];
					}
					break;
				}
			}
			return Result;
		}

		static consteval bool IsDigit(char Char) { return Char >= '0' && Char <= '9'; }

		static consteval bool IsAlnum(char Char)
		{
			const bool bSmallLetter = Char >= 'a' && Char <= 'z';
			const bool bCapitalLetter = Char >= 'A' && Char <= 'Z';
			const bool bDigit = IsDigit(Char);
			const bool bOther = Char == '_' || Char == ':';
			return bSmallLetter || bCapitalLetter || bDigit || bOther;
		}

		template <T Value>
		static consteval int GetMemberNameSize()
		{
			// void ses()[with auto v = aa]
			// void ses()[with auto v = (qwe)2]
			// void __cdecl ses<aa>(void)
			// void __cdecl ses<(enum qwe)0x2>(void)
			constexpr UE::FSourceLocation SourceLocation = UE::FSourceLocation::Current();
			for (int i = std::char_traits<char>::length(SourceLocation.GetFunctionName()); i >= 0; --i)
			{
				if (SourceLocation.GetFunctionName()[i] == ClosingBrace)
				{
					for (int j = i - 1; j >= 0; --j)
					{
						if (!IsAlnum(SourceLocation.GetFunctionName()[j]))
						{
							// Must be -1, but leave this extra char for string terminator
							return IsDigit(SourceLocation.GetFunctionName()[j + 1]) ? 0 : i - j;
						}
					}
				}
			}
			return 0;
		}

		template <T Value>
		static consteval std::array<TCHAR, GetMemberNameSize<Value>()> GetMemberName()
		{
			constexpr int Size = GetMemberNameSize<Value>();
			std::array<TCHAR, Size> Result;
			for (int i = std::char_traits<char>::length(UE::FSourceLocation::Current().GetFunctionName()); i >= 0; --i)
			{
				if (UE::FSourceLocation::Current().GetFunctionName()[i] == ClosingBrace)
				{
					for (int j = 0; j < Size; ++j)
					{
						Result[Size - j - 1] = UE::FSourceLocation::Current().GetFunctionName()[i - j];
					}
					break;
				}
			}
			if (Size > 0)
				Result[Size - 1] = '\0';
			return Result;
		}

		template <int... As, int... Bs>
		static consteval auto Concat(std::integer_sequence<int, As...> NegativeValues, std::integer_sequence<int, Bs...> PositiveValues)
		{
			constexpr int LastA = (As, ...);
			return std::integer_sequence<int, LastA - As - 1 ..., Bs...>{};
		}

		using TestedValues = decltype(Concat(std::make_integer_sequence<int, 1>(), std::make_integer_sequence<int, 31>()));

		static consteval int GetNumMembers()
		{
			return []<int... Values>(std::integer_sequence<int, Values...>)consteval -> int
			{
				return ((GetMemberNameSize<T(Values)>() > 0 ? 1 : 0) + ...);
			}(TestedValues());
		}

		static consteval int GetMembersSize()
		{
			return []<int... Values>(std::integer_sequence<int, Values...>)consteval -> int
			{
				return (GetMemberNameSize<T(Values)>() + ...);
			}(TestedValues());
		}

		static consteval std::array<TCHAR, GetMembersSize()> GetMembers()
		{
			std::array<TCHAR, GetMembersSize()> Result;
			int i = 0;
			[&] <int... Values>(std::integer_sequence<int, Values...>)
			{
				auto StoreMember = [&]<T Value>(std::integral_constant<T, Value>)
				{
					std::array<TCHAR, GetMemberNameSize<T(Value)>()> Name = GetMemberName<T(Value)>();
					for (int j = 0; j < Name.size(); ++j, ++i)
					{
						Result[i] = Name[j];
					}
				};
				(StoreMember(std::integral_constant<T, T(Values)>()), ...);
			}(TestedValues());
			return Result;
		}

		static consteval std::array<Private::FEnum::FMember, GetNumMembers()> GetLayout()
		{
			std::array<Private::FEnum::FMember, GetNumMembers()> Result;
			int i = 0;
			[&] <int... Values>(std::integer_sequence<int, Values...>) consteval
			{
				auto StoreMember = [&]<T Value>(std::integral_constant<T, Value>) consteval
				{
					constexpr int Size = GetMemberNameSize<Value>();
					if (Size > 0)
					{
						std::array<TCHAR, Size> Name = GetMemberName<Value>();
						Result[i].FullNameStart = i == 0 ? 0 : Result[i - 1].NameEnd + 1;
						Result[i].ShortNameStart = Result[i].FullNameStart;
						Result[i].NameEnd = Result[i].FullNameStart + Size - 1;
						Result[i].Value = static_cast<uint64>(Value);
						for (int j = Size - 1; j >= 0; --j)
						{
							if (Name[j] == ':')
							{
								Result[i].ShortNameStart = Result[i].FullNameStart + j + 1;
								break;
							}
						}
						++i;
					}
				};
				(StoreMember(std::integral_constant<T, T(Values)>()), ...);
			}(TestedValues());
			return Result;
		}

	public:
		static FEnumerationAttribute ConstructValue(T Value)
		{
			static const Private::FEnum Enum = []() -> Private::FEnum
			{
				Private::FEnum Result;
				Result.bClass = TIsEnumClass<T>::Value;
				static std::array EnumName = GetEnumName();
				Result.Name = FStringView(EnumName.data(), EnumName.size());
				static std::array<TCHAR, GetMembersSize()> Members = GetMembers();
				static std::array<Private::FEnum::FMember, GetNumMembers()> Layout = GetLayout();
				Result.MembersNames = Members.data();
				Result.Members = Layout.data();
				Result.NumMembers = Layout.size();
				return Result;
			}();
			return FEnumerationAttribute(static_cast<uint64>(Value), Enum);
		}

		static FEnumerationAttribute ConstructValue() { return ConstructValue(T(0)); }

		static T GetValue(const IAttributeImplementation& Attribute) { return static_cast<T>(static_cast<const FEnumerationAttribute&>(Attribute).GetValue()); }
	};

	/** EVisibility is a very special boi, but we still love him */
	template <>
	struct SKETCH_API TAttributeTraits<EVisibility>
	{
		static FEnumerationAttribute ConstructValue(EVisibility Value = EVisibility());
		static EVisibility GetValue(const IAttributeImplementation& Attribute);
	};
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
