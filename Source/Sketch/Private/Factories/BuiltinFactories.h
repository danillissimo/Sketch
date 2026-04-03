#pragma once

void RegisterSlateFactories();
void RegisterSlateCoreFactories();
void RegisterHandWrittenFactories();
void RegisterTestWidgetFactory();

inline void RegisterBuiltinFactories()
{
	RegisterSlateCoreFactories();
	RegisterSlateFactories();

	// Must always be the last one so it can override any automatically generated factories
	RegisterHandWrittenFactories();

	// RegisterTestWidgetFactory();
}
