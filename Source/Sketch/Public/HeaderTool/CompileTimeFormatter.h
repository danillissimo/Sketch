#pragma once
#include <array>

namespace sketch
{
	template <int N>
	struct TCompileTimeFormatterHelper
	{
		consteval TCompileTimeFormatterHelper() = default;
		consteval TCompileTimeFormatterHelper(const TCHAR(&Initializer)[N])
		{
			for (int i = 0; i < N; ++i)
				Data[i] = Initializer[i];
		}

		template <int M>
		consteval TCompileTimeFormatterHelper<N + M - 1> operator+(const TCHAR(&Other)[M]) const
		{
			TCompileTimeFormatterHelper<N + M - 1> Result;
			for (int i = 0; i < N - 1; ++i)
				Result.Data[i] = Data[i];
			for (int i = 0; i < M; ++i)
				Result.Data[N + i - 1] = Other[i];
			return Result;
		}

		operator const TCHAR* () const { return Data.data(); }

		std::array<TCHAR, N> Data;
	};

	constexpr struct
	{
		template <int N>
		consteval TCompileTimeFormatterHelper<N> operator+(const TCHAR(&Initializer)[N]) const
		{
			return TCompileTimeFormatterHelper<N>(Initializer);
		}
	} GFormatterHelperInitializer;
}

#define $F sketch::GFormatterHelperInitializer + TEXT("")
#define $T(Namespace, Type) + ((Namespace::Type*)nullptr, TEXT(#Type))
#define $V(Namespace, Variable) + ((decltype(Namespace::Variable)*)nullptr, TEXT(#Variable))

// struct AA {};
// enum { BB };
//
// int main()
// {
// 	std::cout << $F "compile time " $T(, AA) + " " $V(, BB) + TEXT(" qwe");
// 	return 0;
// }
