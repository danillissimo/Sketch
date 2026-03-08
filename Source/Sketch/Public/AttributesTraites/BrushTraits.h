#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FBrushAttribute : IAttributeImplementation
	{
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
		virtual void Reinitialize(const IAttributeImplementation& From) override;

		FBrushAttribute() = default;
		SKETCH_API FBrushAttribute(const FSlateBrush* Brush);
		FBrushAttribute(const FBrushAttribute&) = default;
		FBrushAttribute(FBrushAttribute&&) = default;
		const FSlateBrush* GetValue() const { return Brush; }
		FString GenerateBaseCode() const;

		const FSlateBrush* Brush = nullptr;
		FName StyleName;
		FName BrushName;
	};

	template <>
	struct TAttributeTraits<const FSlateBrush*> : public TCommonAttributeTraits<FBrushAttribute> {};

	struct FIconAttribute : public FBrushAttribute
	{
		virtual FString GenerateCode() const override;

		FIconAttribute() = default;
		FIconAttribute(FSlateIcon&& Icon) : FBrushAttribute(Icon.GetIcon()) {}
		FIconAttribute(const FIconAttribute& Icon) = default;
		FIconAttribute(FIconAttribute&& Icon) = default;
		FSlateIcon GetValue() const { return FSlateIcon(StyleName, BrushName); }
	};

	template <>
	struct TAttributeTraits<FSlateIcon> : public TCommonAttributeTraits<FIconAttribute> {};
}
