#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FFontAttribute : public IAttributeImplementation
	{
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
		virtual void Reinitialize(const IAttributeImplementation& From) override;

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
