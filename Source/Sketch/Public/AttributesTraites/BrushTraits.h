#pragma once
#include "AttributesTraits.h"
#include "Textures/SlateIcon.h"

namespace sketch
{
	struct FBrushAttribute : IAttributeImplementation
	{
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
		SKETCH_API virtual bool Equals(const IAttributeImplementation& Other) const override;
		SKETCH_API virtual void Reinitialize(const IAttributeImplementation& From) override;

		FBrushAttribute() = default;
		SKETCH_API FBrushAttribute(const FSlateBrush* Brush);
		FBrushAttribute(const FBrushAttribute&) = default;
		FBrushAttribute(FBrushAttribute&&) = default;
		const FSlateBrush* GetValue() const { return Brush ? Brush : FCoreStyle::Get().GetDefaultBrush(); }
		FString GenerateBaseCode() const;

		const FSlateBrush* Brush = nullptr;
		FName StyleName;
		FName BrushName;
	};

	template <>
	struct TAttributeTraits<const FSlateBrush*> : public TCommonAttributeTraits<FBrushAttribute> {};

	struct FIconAttribute : public FBrushAttribute
	{
		SKETCH_API virtual FString GenerateCode() const override;

		FIconAttribute() = default;
		FIconAttribute(FSlateIcon&& Icon) : FBrushAttribute(Icon.GetIcon()) {}
		FIconAttribute(const FIconAttribute& Icon) = default;
		FIconAttribute(FIconAttribute&& Icon) = default;
		FSlateIcon GetValue() const { return FSlateIcon(StyleName, BrushName); }
	};

	template <>
	struct TAttributeTraits<FSlateIcon> : public TCommonAttributeTraits<FIconAttribute> {};
}
