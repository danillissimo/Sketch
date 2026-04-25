#pragma once
#include "Sketch.h"

namespace sketch::Sandbox
{
	SKETCH_API FAttributeCollection GetMutable();
	inline FConstAttributeCollection Get() { return GetMutable(); }

	template <class T>
	void MakeAttribute(const TCHAR* Name, const std::source_location& SL = std::source_location::current())
	{
		FSketchCore& Core = FSketchCore::Get();
		Core.RedirectNewAttributesInto(GetMutable());
		Sketch(Name, SL).operator T();
		Core.StopRedirectingNewAttributes();
	}

	SKETCH_API void DeferItemInitialization(TFunction<void()>&& Initializer);

	template <class T>
	struct TItemInitializer
	{
		TItemInitializer(const TCHAR* Name, const std::source_location& SL = std::source_location::current())
		{
			Sandbox::DeferItemInitialization([=] { MakeAttribute<T>(Name, SL); });
		}
	};
}
