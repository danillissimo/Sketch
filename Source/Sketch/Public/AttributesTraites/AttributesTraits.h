#pragma once
#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"

class SWidget;

namespace sketch
{
	///
	/// Runtime attribute traits
	///
	/** @note Derivatives must provide both copy and move constructors */
	class IAttributeImplementation : public TSharedFromThis<IAttributeImplementation>
	{
	public:
		virtual ~IAttributeImplementation() = default;

		SKETCH_API virtual TSharedRef<SWidget> MakeEditor();
		SKETCH_API virtual FString GenerateCode() const;
		SKETCH_API TOptional<FString> TryGenerateCode() const;
		virtual bool Equals(const IAttributeImplementation& Other) const { return true; }
		/** @note You'll probably want to call OnValueChanged right after */
		virtual void Reinitialize(const IAttributeImplementation& From) {}

		FSimpleMulticastDelegate OnValueChanged;
	};

	template <class T>
	struct TCommonAttributeImplementation : public IAttributeImplementation
	{
		virtual bool Equals(const IAttributeImplementation& Other) const override
		{
			if constexpr (requires { std::declval<T>() == std::declval<T>(); })
			{
				return Value == static_cast<const TCommonAttributeImplementation<T>&>(Other).Value;
			}
			else
			{
				ensure(false);
				return true;
			}
		}

		virtual void Reinitialize(const IAttributeImplementation& From) override { Value = static_cast<const TCommonAttributeImplementation<T>&>(From).Value; }

		template <class... ArgTypes>
		TCommonAttributeImplementation(ArgTypes&&... Args)
			: Value(Forward<ArgTypes>(Args)...) {
		}

		TCommonAttributeImplementation(const TCommonAttributeImplementation&) = default;
		TCommonAttributeImplementation(TCommonAttributeImplementation&&) = default;

		const T& GetValue() const { return Value; }
		T CopyValue() const { return Value; }
		void SetValue(const T& InValue) { Value = InValue, OnValueChanged.Broadcast(); }
		void SetValue(T&& InValue) { Value = MoveTemp(InValue), OnValueChanged.Broadcast(); }
		void ReceiveValue(T InValue) { Value = InValue, OnValueChanged.Broadcast(); }

		T Value = T();
	};



	///
	/// Compile time attribute traits
	///
	/** Sketch doesn't deny any requests, but doesn't support all of them. This type handles unsupported requests. */
	struct FUnsupported : public IAttributeImplementation
	{
	};

	template <class T>
	struct TAttributeTraits
	{
		/** Note must be ready to receive no arguments at all */
		template <class... ArgTypes>
		static FUnsupported ConstructValue(ArgTypes&&...) { return FUnsupported{}; }

		static T GetValue(const IAttributeImplementation& Attribute) { return T{}; }
	};

	template <class ImplementationType>
	struct TCommonAttributeTraits
	{
		template <class... ArgTypes>
		static ImplementationType ConstructValue(ArgTypes&&... Args) { return ImplementationType(std::forward<ArgTypes>(Args)...); }

		static decltype(auto) GetValue(const IAttributeImplementation& Attribute) { return static_cast<const ImplementationType&>(Attribute).GetValue(); }
	};



	///
	/// Header tool runtime attribute traits
	///
	SKETCH_API TArray<TFunction<bool(::FStringView Type)>>& GetSupportedAttributeFilters();

	struct FHeaderToolAttributeFilter
	{
		FHeaderToolAttributeFilter(TFunction<bool(::FStringView Type)>&& Filter)
		{
			GetSupportedAttributeFilters().Add(MoveTemp(Filter));
		}
	};

	inline bool IsSupportedAttributeType(::FStringView Type)
	{
		return GetSupportedAttributeFilters().ContainsByPredicate(
			[&](const TFunction<bool(::FStringView Type)>& Filter)
			{
				return Filter(Type);
			}
		);
	}
}
