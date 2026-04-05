#include "SketchSandbox.h"

auto& GetDeferredInitializers()
{
	static TArray<TFunction<void()>, TInlineAllocator<4>> DeferredInitializers;
	return DeferredInitializers;
}

sketch::FAttributeCollection sketch::Sandbox::GetMutable()
{
	static FAttributeCollection Sandbox{ MakeShared<TArray<TSharedPtr<FAttribute>>>() };

	static bool bRunning = false;
	if (!bRunning)
	{
		bRunning = true;
		auto& DeferredInitializers = GetDeferredInitializers();
		for (const TFunction<void()>& Initializer : DeferredInitializers)
		{
			Initializer();
		}
		DeferredInitializers.Empty();
		bRunning = false;
	}

	return Sandbox;
}

void sketch::Sandbox::DeferItemInitialization(TFunction<void()>&& Initializer)
{
	GetDeferredInitializers().Add(MoveTemp(Initializer));
}
