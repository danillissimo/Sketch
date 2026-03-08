#include "SketchSandbox.h"

sketch::FAttributeCollection sketch::Sandbox::GetMutable()
{
	static FAttributeCollection Sandbox{ MakeShared<TArray<TSharedPtr<FAttribute>>>() };
	return Sandbox;
}
