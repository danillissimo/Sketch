#pragma once
#include <source_location>
#include <array>

#include "UObject/NameTypes.h"
#include "Misc/TVariant.h"
#include "AttributesTraites/AllTraits.h"

class FSketchModule;
class SSketchWidget;

/*
 * Sketch data types
 */

namespace sketch
{
	/// 
	/// Types Sketch operates on
	/// 
	/** Sketch doesn't deny any requests, but doesn't support all of them. This type handles unsupported requests. */
	struct FUnsupported
	{
		template <class... ArgTypes>
		FUnsupported(ArgTypes&&...)
		{
		}

		uint8 MemoryInitializer = 0;
	};

	template <class T>
	using TOption = std::array<T, 2>;

	template <class... T>
	using TVariantBase = ::TVariant<TOption<T>...>;

	/**
	 * Main data container
	 * @note Default first type without any constructor
	 */
	using FVariant = TVariantBase<
		FUnsupported,

		FInteger,
		float,
		double,

		FOptionalSize,
		FMargin,

		FLinearColor,
		FSlateColor,

		FText,
		// FString,

		FFont,
		FBrush
	>;



	namespace Private
	{
		template <class T>
		struct TArgType;

		template <class ReturnType, class T, class ArgType>
		struct TArgType<ReturnType(T::*)(ArgType) const>
		{
			using Type = ArgType;
		};

		template <class ReturnType, class T, class ArgType>
		struct TArgType<ReturnType(T::*)(ArgType)>
		{
			using Type = ArgType;
		};



		template <class T>
		struct TReturnType;

		template <class ReturnType, class T, class... ArgTypes>
		struct TReturnType<ReturnType(T::*)(ArgTypes...) const>
		{
			using Type = ReturnType;
		};

		template <class ReturnType, class T, class... ArgTypes>
		struct TReturnType<ReturnType(T::*)(ArgTypes...)>
		{
			using Type = ReturnType;
		};



		template <class T>
		concept CInvokable = requires { &T::template operator()<FUnsupported>; };
	}



	///
	/// Types used by Sketch internally and by extensions
	/// 
	/** Partial Sketch attribute source path, that may be shared by multiple attributes */
	struct FSourceLocation
	{
		FSourceLocation(const std::source_location& SourceLocation = std::source_location::current())
			: FileName(SourceLocation.file_name())
			  , FunctionName(SourceLocation.function_name())
		// , Line(SourceLocation.line())
		// , Column(SourceLocation.column())
		{
		}

		FSourceLocation(ENoInit)
		{
		}

		FName FileName;
		FName FunctionName;
		// uint32 Line = 0;
		// uint32 Column = 0;

		friend uint32 GetTypeHash(const FSourceLocation& SourceLocation) { return HashCombineFast(GetTypeHash(SourceLocation.FileName), GetTypeHash(SourceLocation.FunctionName)); }
		bool operator==(const FSourceLocation& Other) const { return FileName == Other.FileName && FunctionName == Other.FunctionName; }
	};

	/** Contains operated data and all information necessary to manipulate Sketch attributes and patch code */
	struct FAttribute
	{
		template <class T>
		const T* GetValue() const { return Data.IsType<TOption<T>>() ? &Data.Get<TOption<T>>()[0] : nullptr; }

		template <class T>
		T* GetValue() { return Data.IsType<TOption<T>>() ? &Data.Get<TOption<T>>()[0] : nullptr; }

		template <class T>
		const T& GetValueSafe(T&& DefaultValue) const { return this && Data.IsType<TOption<T>>() ? Data.Get<TOption<T>>()[0] : DefaultValue; }

		template <class T>
		T& GetValueSafe(T&& DefaultValue) { return this && Data.IsType<TOption<T>>() ? Data.Get<TOption<T>>()[0] : DefaultValue; }

		template <class T>
		const T& GetValueChecked() const { return Data.Get<TOption<T>>()[0]; }

		template <class T>
		T& GetValueChecked() { return Data.Get<TOption<T>>()[0]; }

		template <class T>
		const T* GetDefaultValue() const { return Data.IsType<TOption<T>>() ? &Data.Get<TOption<T>>()[1] : nullptr; }

		template <class T>
		T* GetDefaultValue() { return Data.IsType<TOption<T>>() ? &Data.Get<TOption<T>>()[1] : nullptr; }

		template <class T>
		const T& GetDefaultValueChecked() const { return Data.Get<TOption<T>>()[1]; }

		template <class T>
		T& GetDefaultValueChecked() { return Data.Get<TOption<T>>()[1]; }

		template <class FunctorType>
		using TFunctionType = decltype(&FunctorType::template operator()<FUnsupported>);
		template <class FunctorType>
		using TReturnType = typename Private::TReturnType<TFunctionType<FunctorType>>::Type;
		template <class FunctorType>
		using TDefaultValueType = std::conditional_t<std::is_void_v<TReturnType<FunctorType>>, decltype(std::ignore), TReturnType<FunctorType>>;

		template <class FunctorType>
		decltype(auto) Apply(FunctorType&& Functor) const { return ApplyImpl(*this, std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) Apply(FunctorType&& Functor) { return ApplyImpl(*this, std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) ApplySafe(FunctorType&& Functor, TDefaultValueType<FunctorType> DefaultValue = {}) const { return ApplySafeImpl(this, std::forward<FunctorType>(Functor), std::forward<TDefaultValueType<FunctorType>>(DefaultValue)); }

		template <class FunctorType>
		decltype(auto) ApplySafe(FunctorType&& Functor, TDefaultValueType<FunctorType> DefaultValue = {}) { return ApplySafeImpl(this, std::forward<FunctorType>(Functor), std::forward<TDefaultValueType<FunctorType>>(DefaultValue)); }

		template <class FunctorType>
		friend decltype(auto) operator<<=(const FAttribute* This, FunctorType&& Functor) { return This->Apply(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		friend decltype(auto) operator<<=(FAttribute* This, FunctorType&& Functor) { return This->Apply(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		friend decltype(auto) operator<=(const FAttribute* This, FunctorType&& Functor) { return This->ApplySafe(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		friend decltype(auto) operator<=(FAttribute* This, FunctorType&& Functor) { return This->ApplySafe(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) operator<<=(FunctorType&& Functor) const { return Apply(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) operator<<=(FunctorType&& Functor) { return Apply(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) operator<=(FunctorType&& Functor) const { return ApplySafe(std::forward<FunctorType>(Functor)); }

		template <class FunctorType>
		decltype(auto) operator<=(FunctorType&& Functor) { return ApplySafe(std::forward<FunctorType>(Functor)); }

		template <class ThisType, class T>
		struct TDeferredSafeApplication
		{
			ThisType& This;
			T&& DefaultValue;

			template <class FunctorType>
			decltype(auto) operator <=(FunctorType&& Functor) const { return This.ApplySafe(std::forward<FunctorType>(Functor), std::forward<T>(DefaultValue)); }
		};

		template <class T> requires !Private::CInvokable<T>
		friend TDeferredSafeApplication<const FAttribute, T> operator<=(const FAttribute* This, T&& DefaultValue) { return {*This, std::forward<T>(DefaultValue)}; }

		template <class T> requires !Private::CInvokable<T>
		friend TDeferredSafeApplication<FAttribute, T> operator<=(FAttribute* This, T&& DefaultValue) { return {*This, std::forward<T>(DefaultValue)}; }

		template <class T> requires !Private::CInvokable<T>
		TDeferredSafeApplication<const FAttribute, T> operator<=(T&& DefaultValue) const { return {*this, std::forward<T>(DefaultValue)}; }

		template <class T> requires !Private::CInvokable<T>
		TDeferredSafeApplication<FAttribute, T> operator<=(T&& DefaultValue) { return {*this, std::forward<T>(DefaultValue)}; }


		const FSourceLocation& GetSourceLocation() const { return SourceLocation; }
		uint32 GetLine() const { return Line; }
		uint32 GetColumn() const { return Column; }
		FName GetName() const { return Name; }
		bool IsDynamic() const { return bDynamic; }
		uint8 GetStaleCountdown() const { return StaleCountdown; }
		uint16 GetNumUsers() const { return NumUsers; }

	private:
		template <class... ConstructorArgTypes>
		friend struct TAttributeInitializer;
		friend class ::FSketchModule;
		template <class T, class OwnerType, class ContainerType>
		friend struct TAttributeReader;

		FVariant Data;
		bool bDynamic = false;
		uint8 StaleCountdown = 0;
		uint16 NumUsers = 0;
		FSourceLocation SourceLocation;
		uint32 Line = 0;
		uint32 Column = 0;
		FName Name;

		template <class ThisType, class FunctorType>
		static decltype(auto) ApplyImpl(ThisType& This, FunctorType&& Functor);

		template <class ThisType, class FunctorType>
		static decltype(auto) ApplySafeImpl(ThisType* This, FunctorType&& Functor, TDefaultValueType<FunctorType> DefaultValue);
	};

	// /**
	//  * Soft Sketch attribute pointer.
	//  * Sketch attributes may get relocated or removed, making direct pointers dangerous to keep.
	//  * So, any long-term references must use this type to address them.
	//  * Must be resolved via FSketchModule to reduce implicit module lookups.
	//  */
	// struct FAttributePath
	// {
	// 	FSourceLocation Source;
	// 	int Index = INDEX_NONE;
	//
	// 	bool operator==(const FAttributePath& Other) const { return Index == Other.Index && Source == Other.Source; }
	// 	friend uint32 GetTypeHash(const FAttributePath& Path) { return HashCombineFast(GetTypeHash(Path.Index), GetTypeHash(Path.Source)); }
	// };

	struct FAttributeCollectionHandle
	{
		TWeakPtr<const void> Owner;
		TVariant<TArray<FAttribute>*, TSparseArray<FAttribute>*> Container;

		FAttributeCollectionHandle() = default;

		template <class T>
			requires std::is_same_v<std::remove_const_t<T>, TArray<FAttribute>> || std::is_same_v<std::remove_const_t<T>, TSparseArray<FAttribute>>
		FAttributeCollectionHandle(const TWeakPtr<const void>& Owner, T& Array)
			: Owner(Owner),
			  Container(TInPlaceType<std::remove_const_t<T>*>{}, &const_cast<std::remove_const_t<T>&>(Array)) {}

		bool IsValid() const { return Owner.IsValid() && (!!Container.Get<TArray<FAttribute>*>(nullptr) || !!Container.Get<TSparseArray<FAttribute>*>(nullptr)); }
		decltype(auto) operator<<(const auto& Callable) const;
	};

	struct FAttributeHandle
	{
		FAttributeCollectionHandle CollectionHandle;
		int Index = INDEX_NONE;

		FAttribute* Get() const;
		FAttribute* operator->() const { return Get(); }
		FAttribute* operator*() const { return Get(); }

		template <class T>
		T GetValue(const T& DefaultValue = {}) const;
		template <class T>
		bool SetValue(const T& Value) const;
	};



	template <class T>
	struct TIsSlateAttribute
	{
		enum { Value = false };
	};

	template <class T>
	struct TIsSlateAttribute<TAttribute<T>>
	{
		enum { Value = true };
	};

	template <class T>
	struct TSlateAttributeType
	{
		using Type = void;
	};

	template <class T>
	struct TSlateAttributeType<TAttribute<T>>
	{
		using Type = T;
	};

	/** Intermediate type that holds any required data between an attribute construction request and attribute construction */
	template <class... ConstructorArgTypes>
	struct TAttributeInitializer
	{
		FName AttributeName;
		[[no_unique_address]] // 'Cause it may be empty
		TTuple<ConstructorArgTypes...> ConstructorArgs;
		FSourceLocation SourceLocation = {NoInit};
		uint32 Line = 0;
		uint32 Column = 0;

		template <class T>
			requires std::is_constructible_v<
				std::conditional_t<sketch::TIsSlateAttribute<T>::Value, typename sketch::TSlateAttributeType<T>::Type, T>,
				ConstructorArgTypes...
			>
		operator T();
	};

	template <class T, class OwnerType, class ContainerType>
	struct TAttributeReader
	{
		ContainerType& Container;
		const int Index;
		const TWeakPtr<const OwnerType> Owner;

		TAttributeReader(const TWeakPtr<const OwnerType>& Owner, ContainerType& Container, int Index);
		TAttributeReader(const TAttributeReader& Other);
		TAttributeReader(TAttributeReader&& Other);
		T operator()() const;
		~TAttributeReader();
	};

	struct FFactory
	{
		FName Name;
		TFunction<TSharedRef<SWidget>()> ConstructWidget;
		using FUniqueSlots = TArray<SSketchWidget*, TInlineAllocator<4>>;
		TFunction<FUniqueSlots(SWidget& Widget)> EnumerateUniqueSlots;
		using FDynamicSlotTypes = TArray<FName, TInlineAllocator<1>>;
		TFunction<FDynamicSlotTypes(const SWidget& Widget)> EnumerateDynamicSlotTypes;
		TFunction<FSlotBase&(SWidget& Widget, const FName& SlotType)> ConstructDynamicSlot;
		TFunction<void(SWidget& Widget, const FName& SlotType, int Index, FSlotBase& Slot)> DestroyDynamicSlot;
	};

/** Every widget factory should end (or begin) from calling this macro - it sets up attributes inherited by each widget from SWidget */
#define SKETCH_WIDGET_FACTORY_BOILERPLATE()\
	 ToolTipText(Sketch("ToolTipText",                         FText                            {}))
	// .ToolTip(Sketch("ToolTip",                                 TSharedPtr<IToolTip>             {}))\
	// .Cursor(Sketch("Cursor",                                   TOptional<EMouseCursor::Type>    {}))\
	// .IsEnabled(Sketch("IsEnabled",                             bool                             { true }))\
	// .Visibility(Sketch("Visibility",                           EVisibility                      { EVisibility::Visible }))\
	// .ForceVolatile(Sketch("ForceVolatile",                     bool                             { false }))\
	// .Clipping(Sketch("Clipping",                               EWidgetClipping                  { EWidgetClipping::Inherit }))\
	// .PixelSnappingMethod(Sketch("PixelSnappingMethod",         EWidgetPixelSnapping             { EWidgetPixelSnapping::Inherit }))\
	// .FlowDirectionPreference(Sketch("FlowDirectionPreference", EFlowDirectionPreference         { EFlowDirectionPreference::Inherit }))\
	// .RenderOpacity(Sketch("RenderOpacity",                     float                            { 1.f }))\
	// .RenderTransform(Sketch("RenderTransform",                 TOptional<FSlateRenderTransform> { FVector2D::ZeroVector }))\
	// .RenderTransformPivot(Sketch("RenderTransformPivot",       FVector2D                        {}))\
	// .Tag(Sketch("Tag",                                         FName                            {}))\
	// .AccessibleParams(Sketch("AccessibleParams",               TOptional<FAccessibleWidgetData> {}))\
	// .AccessibleText(Sketch("AccessibleText",                   FText                            {}))

	struct FFactoryHandle
	{
		FFactory* Resolve() const;
		bool IsValid() const { return !Type.IsNone() && !Name.IsNone(); }

		FName Type;
		FName Name;
		bool operator==(const FFactoryHandle& Other) const { return Type == Other.Type && Name == Other.Name; }
	};
}
