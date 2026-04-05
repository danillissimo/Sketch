#pragma once
#include "Templates/Tuple.h"

/*
 * Header included by other modules when Sketch is disabled
 */

namespace NoSketch
{
	template <class... ConstructorArgTypes>
	struct TAttributeInitializer
	{
		[[no_unique_address]] // 'Cause it may be empty
		TTuple<ConstructorArgTypes...> ConstructorArgs;

		TAttributeInitializer& AddMeta(const auto&...) { return *this; }

		template <class T>
		operator T()
		{
			auto InvokeConstructor = [](auto&&... Args)
			{
				return T{ MoveTempIfPossible(Args)... };
			};
			return ConstructorArgs.ApplyAfter(InvokeConstructor);
		}
	};
}

NoSketch::TAttributeInitializer<> Sketch(const auto& Name) { return {}; }

template <class... T>
NoSketch::TAttributeInitializer<std::decay_t<T>...> Sketch(const auto& Name, T&&... Args) { return { ::MakeTuple(::Forward<T...>(Args...)) }; }
