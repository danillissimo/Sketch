#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FBrush
	{
		FBrush() = default;
		SKETCH_API FBrush(const FSlateBrush* Brush);

		FBrush(const FSlateBrush* Brush, const FName& StyleName, const FName& BrushName)
			: Brush(Brush)
			  , StyleName(StyleName)
			  , BrushName(BrushName)
		{
		}

		const FSlateBrush* Brush = nullptr;
		FName StyleName;
		FName BrushName;
	};

	template <>
	struct TWrappedType<FBrush>
	{
		using Type = const FSlateBrush*;
	};

	template <>
	struct TAttributeTraits<const FSlateBrush*>
	{
		using FStorage = FBrush;

		static const FSlateBrush* GetValue(const FBrush& Storage) { return Storage.Brush; }
		SKETCH_API static TSharedRef<SWidget> MakeEditor(const FAttributeHandle& Handle);
		SKETCH_API static FString GenerateCode(const FBrush& Brush);
	};
}
