// Copyright 2026 danillissimo

#pragma once
#include "ConstexprStringView.h"

namespace sketch::SourceCode
{
	struct FProcessedString
	{
		/** May rely on source string or on Container if any processing took place */
		FSketchStringView View;
		/** Only contains data if source string needed modifications */
		FString Container;
	};
}
