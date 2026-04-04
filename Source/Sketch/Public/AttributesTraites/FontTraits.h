#pragma once
#include "AttributesTraits.h"
#include "Fonts/SlateFontInfo.h"

namespace sketch
{
	struct FFontAttribute : public IAttributeImplementation
	{
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;
		SKETCH_API virtual bool Equals(const IAttributeImplementation& Other) const override;
		SKETCH_API virtual void Reinitialize(const IAttributeImplementation& From) override;

		SKETCH_API FFontAttribute(FSlateFontInfo&& Info = FSlateFontInfo());
		FFontAttribute(const FFontAttribute&) = default;
		FFontAttribute(FFontAttribute&&) = default;
		const FSlateFontInfo& GetValue() const { return FontInfo; }

		FSlateFontInfo FontInfo;
		FName StyleName;
		FName FontName;
	};

	template <>
	struct TAttributeTraits<FSlateFontInfo> : public TCommonAttributeTraits<FFontAttribute> {};
}
