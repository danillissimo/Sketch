#pragma once
#include <tuple>
#include <type_traits>

/**
 * @code
 * TBuildDependent<DefaultValue, Condition> A;          // incorrect usage - Type is implicit
 * TBuildDependent<Type { DefaultValue }, Condition> B; // correct usage
 * TBuildDependent<Type ( DefaultValue ), Condition> C; // correct usage
 * @endcode
 * @warning Many instances of this type may affect class size, since C++ must provide at least one
 *          byte for each field, and [[no_unique_address]] only allows marked fields overlap with
 *			unmarked fields, but not with each other.
 */
template <auto DefaultValue, bool Condition, class Type = decltype(DefaultValue)>
struct TBuildDependent
{
	constexpr TBuildDependent() = default;
	template <class... ArgTypes>
	constexpr TBuildDependent(ArgTypes... Args)
	{
		// Uncomment disabled code to make some conversions implicit
		// if constexpr (sizeof...(Args) == 1)
		// {
		// 	Value = static_cast<Type>((Args, ...));
		// }
		// else
		// {
		Value = Type{ std::forward<ArgTypes>(Args)... };
		// }
	}

	constexpr Type GetValue() const
	{
		if constexpr (Condition) { return Value; }
		else { return DefaultValue; }
	}

	static consteval Type GetDefaultValue() { return DefaultValue; }

	template <class T>
	constexpr bool Equals(const T& Other, bool DefaultResult = true) const
	{
		if constexpr (Condition) { return Value == Other; }
		else { return DefaultResult; }
	}

	template <class T>
	constexpr operator T() const { return static_cast<T>(GetValue()); }
	template <class T>
	constexpr TBuildDependent& operator=(const T& InValue) { return Value = InValue, *this; }
	template <class T>
	constexpr bool operator==(const T& InValue) const { return GetValue() == InValue; }
	constexpr bool operator!() const { return !static_cast<bool>(GetValue()); }

private:
	std::conditional_t<Condition, Type, decltype(std::ignore)> Value{ };
};

template <auto RuntimeValue, class Type = decltype(RuntimeValue)>
using TEditorOnly = TBuildDependent<RuntimeValue, WITH_EDITORONLY_DATA, Type>;

template <auto RuntimeValue, class Type = decltype(RuntimeValue)>
using TDevelopmentOnly = TBuildDependent<RuntimeValue, !UE_BUILD_SHIPPING, Type>;

#ifdef _MSC_VER
#define BUILD_DEPENDENT_NO_UNIQUE_ADDRESS msvc::no_unique_address
#else
#define BUILD_DEPENDENT_NO_UNIQUE_ADDRESS no_unique_address
#endif

#define TBuildDependent\
	[[BUILD_DEPENDENT_NO_UNIQUE_ADDRESS]]\
	::TBuildDependent

#define TEditorOnly\
	[[BUILD_DEPENDENT_NO_UNIQUE_ADDRESS]]\
	::TEditorOnly

#define TDevelopmentOnly\
	[[BUILD_DEPENDENT_NO_UNIQUE_ADDRESS]]\
	::TDevelopmentOnly
