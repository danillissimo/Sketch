#pragma once
#include "AttributesTraits.h"

namespace sketch
{
	struct FBooleanAttribute : public TCommonAttributeImplementation<bool>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;

		ECheckBoxState GetState() const { return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }
		void SetState(ECheckBoxState State) { Value = State == ECheckBoxState::Checked, OnValueChanged.Broadcast(); }
	};

	template <>
	struct TAttributeTraits<bool> : public TCommonAttributeTraits<FBooleanAttribute> {};
}
