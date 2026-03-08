#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FUnsupported;

	template <class T>
	struct TAttributeTraits
	{
		using FStorage = FUnsupported;

		static T GetValue(const FUnsupported& Storage) { return T{}; }
		static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle) { return Private::MakeNoEditor(Handle); }
		static FString GenerateCode(const T& Value) { return Private::GenerateNoCode(); }
	};

	// /** Sketch doesn't deny any requests, but doesn't support all of them. This type handles unsupported requests. */
	// struct FUnsupported : public IAttribute
	// {
	// 	template <class... ArgTypes>
	// 	FUnsupported(ArgTypes&&...)
	// 	{
	// 	}
	//
	// 	uint8 MemoryInitializer = 0;
	// };
	//
	// template<class T>
	// struct TAttributeTraits
	// {
	// 	static T GetValue(const IAttribute& Attribute) { return T{}; }
	// 	static 
	// };
}
