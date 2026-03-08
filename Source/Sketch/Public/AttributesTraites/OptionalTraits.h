#pragma once
#include "AttributesTraits.h"

///
/// Declaration
///
namespace sketch
{
	namespace Private
	{
		struct FOptionalAttribute
		{
			bool IsSet() const { return bSet; }
			ECheckBoxState GetState() const { return bSet ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }

			SKETCH_API static TSharedRef<SWidget> MakeOptionalEditor(
				TSharedRef<SWidget>&& Content,
				TAttribute<ECheckBoxState>&& IsChecked,
				FOnCheckStateChanged&& OnStateChanged,
				TAttribute<bool>&& IsEnabled
			);

			FOptionalAttribute(bool IsSet) : bSet(IsSet) {}
			bool bSet;
		};
	}

	template <class T>
	struct TOptionalAttribute : decltype(TAttributeTraits<T>::ConstructValue(std::declval<T>())), Private::FOptionalAttribute
	{
		virtual TSharedRef<SWidget> MakeEditor() override;
		virtual FString GenerateCode() const override;
		virtual bool Equals(const IAttributeImplementation& Other) const override;
		virtual void Reinitialize(const IAttributeImplementation& From) override;

		using Traits = TAttributeTraits<T>;
		using Super = decltype(Traits::ConstructValue(std::declval<T>()));

		/** When something is passed - some value should be constructed */
		template <class... ArgTypes>
		TOptionalAttribute(ArgTypes&&... Args)
			: Super(Traits::ConstructValue(Forward<ArgTypes>(Args)...))
			, FOptionalAttribute(true) {}

		/** When nothing is passed - construct default value and consider it unset */
		TOptionalAttribute()
			: Super(Traits::ConstructValue())
			, FOptionalAttribute(false) {}

		/** When an explicit TOptional is passed - just use it */
		TOptionalAttribute(TOptional<T>&& Optional)
			: Super(Optional.IsSet() ? Traits::ConstructValue(Optional.GetValue()) : Traits::ConstructValue())
			, FOptionalAttribute(Optional.IsSet()) {}

		TOptionalAttribute(const TOptionalAttribute&) = default;
		TOptionalAttribute(TOptionalAttribute&&) = default;

		void OnStateChanged(ECheckBoxState NewState) { bSet = NewState == ECheckBoxState::Checked, Super::OnValueChanged.Broadcast(); }
	};

	template <class T>
	struct TAttributeTraits<TOptional<T>> : public TCommonAttributeTraits<TOptionalAttribute<T>>
	{
		static TOptional<T> GetValue(const IAttributeImplementation& Attribute);
	};
}

///
/// Implementation
///
namespace sketch
{
	template <class T>
	TSharedRef<SWidget> TOptionalAttribute<T>::MakeEditor()
	{
		return FOptionalAttribute::MakeOptionalEditor(
			Super::MakeEditor(),
			TAttribute<ECheckBoxState>::CreateSP(this, &FOptionalAttribute::GetState),
			FOnCheckStateChanged::CreateSP(this, &TOptionalAttribute::OnStateChanged),
			TAttribute<bool>::CreateSP(this, &FOptionalAttribute::IsSet)
		);
	}

	template <class T>
	FString TOptionalAttribute<T>::GenerateCode() const
	{
		return bSet ? Super::GenerateCode() : TEXT("{}");
	}

	template <class T>
	bool TOptionalAttribute<T>::Equals(const IAttributeImplementation& Other) const
	{
		const TOptionalAttribute<T>& OtherOptional = static_cast<const TOptionalAttribute<T>&>(Other);
		return bSet == OtherOptional.bSet && Super::Equals(static_cast<const Super&>(OtherOptional));
	}

	template <class T>
	void TOptionalAttribute<T>::Reinitialize(const IAttributeImplementation& From)
	{
		const TOptionalAttribute<T>& OtherOptional = static_cast<const TOptionalAttribute<T>&>(From);
		bSet = OtherOptional.bSet;
		Super::Reinitialize(static_cast<const Super&>(OtherOptional));
	}

	template <class T>
	TOptional<T> TAttributeTraits<TOptional<T>>::GetValue(const IAttributeImplementation& Attribute)
	{
		using FAttribute = TOptionalAttribute<T>;
		const FAttribute& Optional = static_cast<const FAttribute&>(Attribute);
		return Optional.bSet
			       ? TOptional<T>(TAttributeTraits<T>::GetValue(static_cast<const FAttribute::Super&>(Optional)))
			       : TOptional<T>{};
	}
}
