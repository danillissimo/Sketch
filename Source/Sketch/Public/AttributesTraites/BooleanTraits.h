#pragma once
#include "AttributesTraits.h"
#include "SCheckBoxList.h"

namespace sketch
{
	struct FBooleanAttribute : public TCommonAttributeImplementation<bool>
	{
		using TCommonAttributeImplementation::TCommonAttributeImplementation;
		SKETCH_API virtual TSharedRef<SWidget> MakeEditor() override;
		SKETCH_API virtual FString GenerateCode() const override;

		ECheckBoxState GetState() const { return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }
		void SetState(ECheckBoxState State) { Value = State == ECheckBoxState::Checked, OnValueChanged.Broadcast(); }
	};

	template <>
	struct TAttributeTraits<bool> : public TCommonAttributeTraits<FBooleanAttribute> {};
}
