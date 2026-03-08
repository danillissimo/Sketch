#pragma once

/** Well, it turned out to be so much easier */
#define SL TEXT("")

// namespace sketch
// {
// 	template <int N = 0>
// 	struct TStringLiteral
// 	{
// 		template <class T>
// 			requires std::is_same_v<T, WIDECHAR> || std::is_same_v<T, ANSICHAR>
// 		consteval TStringLiteral(const T (&Initializer)[N])
// 		{
// 			for (int i = 0; i < N; ++i)
// 				Data[i] = Initializer[i];
// 		}
//
// 		constexpr operator const TCHAR*() const { return Data.data(); }
//
// 		std::array<TCHAR, N> Data;
// 	};
//
// 	constexpr struct FStringLiteralInitializer
// 	{
// 		template <class T, int N>
// 			requires std::is_same_v<T, WIDECHAR> || std::is_same_v<T, ANSICHAR>
// 		consteval TStringLiteral<N> operator*(const T (&Initializer)[N]) const { return TStringLiteral<N>(Initializer); }
// 	} GStringLiteralInitializer;
//
// 	/** 
// 	 * Resharper formatter highlighting gets disabled when it sees something it doesn't expect.
// 	 * So, for resharper - it's just a regular unreal-defined prefix.
// 	 */
// #ifdef __RESHARPER__
// #define SL TEXT()
// #else
// #define SL sketch::GStringLiteralInitializer *
// #endif
// }
