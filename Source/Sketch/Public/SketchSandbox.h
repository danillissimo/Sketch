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
		static_cast<T>(Sketch(Name, SL));
		Core.StopRedirectingNewAttributes();
	}

	template <class T>
	struct TItemInitializer
	{
		TItemInitializer(const TCHAR* Name, const std::source_location& SL = std::source_location::current()) { MakeAttribute<T>(Name, SL); }
	};
}
