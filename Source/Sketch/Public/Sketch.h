#pragma once
#include "SketchTypes.h"

namespace sketch
{
	template <class... ConstructorArgTypes>
	struct TAttributeInitializer;
}

/*
 * Sketch logic and modules
 */

// Template generator is in the end of the file

template <class ArgType0>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>> Sketch(const FName AttributeName, ArgType0&& Arg0, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2, class ArgType3>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, ArgType3&& Arg3, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2), std::forward<ArgType3>(Arg3)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2, class ArgType3, class ArgType4>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, ArgType3&& Arg3, ArgType4&& Arg4, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2), std::forward<ArgType3>(Arg3), std::forward<ArgType4>(Arg4)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2, class ArgType3, class ArgType4, class ArgType5>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, ArgType3&& Arg3, ArgType4&& Arg4, ArgType5&& Arg5, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2), std::forward<ArgType3>(Arg3), std::forward<ArgType4>(Arg4), std::forward<ArgType5>(Arg5)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2, class ArgType3, class ArgType4, class ArgType5, class ArgType6>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>, std::remove_const_t<std::remove_reference_t<ArgType6>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, ArgType3&& Arg3, ArgType4&& Arg4, ArgType5&& Arg5, ArgType6&& Arg6, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>, std::remove_const_t<std::remove_reference_t<ArgType6>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2), std::forward<ArgType3>(Arg3), std::forward<ArgType4>(Arg4), std::forward<ArgType5>(Arg5), std::forward<ArgType6>(Arg6)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

template <class ArgType0, class ArgType1, class ArgType2, class ArgType3, class ArgType4, class ArgType5, class ArgType6, class ArgType7>
sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>, std::remove_const_t<std::remove_reference_t<ArgType6>>, std::remove_const_t<std::remove_reference_t<ArgType7>>> Sketch(const FName AttributeName, ArgType0&& Arg0, ArgType1&& Arg1, ArgType2&& Arg2, ArgType3&& Arg3, ArgType4&& Arg4, ArgType5&& Arg5, ArgType6&& Arg6, ArgType7&& Arg7, const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<std::remove_const_t<std::remove_reference_t<ArgType0>>, std::remove_const_t<std::remove_reference_t<ArgType1>>, std::remove_const_t<std::remove_reference_t<ArgType2>>, std::remove_const_t<std::remove_reference_t<ArgType3>>, std::remove_const_t<std::remove_reference_t<ArgType4>>, std::remove_const_t<std::remove_reference_t<ArgType5>>, std::remove_const_t<std::remove_reference_t<ArgType6>>, std::remove_const_t<std::remove_reference_t<ArgType7>>>{AttributeName, MakeTuple(std::forward<ArgType0>(Arg0), std::forward<ArgType1>(Arg1), std::forward<ArgType2>(Arg2), std::forward<ArgType3>(Arg3), std::forward<ArgType4>(Arg4), std::forward<ArgType5>(Arg5), std::forward<ArgType6>(Arg6), std::forward<ArgType7>(Arg7)), SourceLocation, SourceLocation.line(), SourceLocation.column()}; }

/**
 * Primary module function - Sketch - is normally more important for a dev than namespace with internal stuff.
 * So "Sketch" is reserved for it, and namespace name begins from a lower case letter to avoid collision.
 * No, I didn't like any kinds of underscores and couldn't come up with a prefix/suffix I'd like.
 */
namespace sketch
{
	namespace Private
	{
		TAttribute<FSlateFontInfo> DefaultFont();
	}
}

#include "SketchModule.h"

template <class T, class... ConstructorArgTypes>
TTuple<sketch::FAttribute&, int> FSketchModule::FindOrCreateSketchAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	// Consider custom attributes
	if (CustomAttributesOwner.IsValid() && CustomAttributes)
	{
		return FindOrCreateSketchAttributeImplementation<T>(*CustomAttributes, Initializer);
	}

	// Use global attributes
	using namespace sketch;
	TSharedPtr<TSparseArray<FAttribute>>* Context = Attributes.Find(Initializer.SourceLocation);
	if (!Context)
	{
		Context = &Attributes.Add(Initializer.SourceLocation, MakeShared<TSparseArray<FAttribute>>());
	}
	return FindOrCreateSketchAttributeImplementation<T>(*Context->Get(), Initializer);
}

template <class T, class ContainerType, class... ConstructorArgTypes>
TTuple<sketch::FAttribute&, int> FSketchModule::FindOrCreateSketchAttributeImplementation(ContainerType& Container, sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	using namespace sketch;
	using StorageType = typename TAttributeTraits<T>::FStorage;

	// Find existing attribute or create a new one
	int Index = Container.IndexOfByPredicate(
		[&](const FAttribute& Attribute)
		{
			return Attribute.Line == Initializer.Line
				&& Attribute.Column == Initializer.Column;
		}
	);
	if (Index == INDEX_NONE)
	{
		auto MakeValue = []<class... ArgTypes>(ArgTypes&&... Args)
		{
			return StorageType(std::forward<ArgTypes>(Args)...);
		};
		FAttribute Attribute;
		Attribute.SourceLocation = Initializer.SourceLocation;
		Attribute.Line = Initializer.Line;
		Attribute.Column = Initializer.Column;
		Attribute.Data.Emplace<TOption<StorageType>>();
		Attribute.GetValueChecked<StorageType>() = Initializer.ConstructorArgs.ApplyAfter(MakeValue);
		Attribute.GetDefaultValueChecked<StorageType>() = Attribute.GetValueChecked<StorageType>();
		Attribute.Name = Initializer.AttributeName;
		Attribute.StaleCountdown = 2;
		Index = Container.Add(MoveTemp(Attribute));
		OnAttributesChanged.Broadcast();
	}
	return {Container[Index], Index};
}

template <class T, class... ConstructorArgTypes>
TAttribute<T> FSketchModule::MakeSlateAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	using namespace sketch;

	// Retrieve context
	auto [Attribute, Index] = FindOrCreateSketchAttribute<T, ConstructorArgTypes...>(Initializer);

	// Finalize attribute configuration
	++Attribute.NumUsers;
	Attribute.bDynamic = true;

	// Construct attribute
	if (CustomAttributesOwner.IsValid() && CustomAttributes)
	{
		return TAttribute<T>::CreateLambda(TAttributeReader<T, void, TArray<FAttribute>>(CustomAttributesOwner, *CustomAttributes, Index));
	}
	else
	{
		const TSharedPtr<TSparseArray<FAttribute>>& Context = Attributes.FindChecked(Initializer.SourceLocation);
		return TAttribute<T>::CreateLambda(TAttributeReader<T, TSparseArray<FAttribute>, TSparseArray<FAttribute>>(Context.ToWeakPtr(), *Context.Get(), Index));
	}
}

template <class ThisType, class FunctorType>
decltype(auto) sketch::FAttribute::ApplyImpl(ThisType& This, FunctorType&& Functor)
{
	return [&]<class... Type>(const TVariantBase<Type...>&)
	{
		auto SelectArg = [&](auto& Value) -> decltype(auto)
		{
			if constexpr (
				std::is_same_v<
					FUnsupported,
					std::remove_const_t<
						std::remove_reference_t<
							typename Private::TArgType<TFunctionType<FunctorType>>::Type
						>
					>
				>
			)
			{
				return Value;
			}
			else
			{
				return This;
			}
		};
		using ReturnType = TReturnType<FunctorType>;
		enum { bReturnsVoid = std::is_void_v<ReturnType> };
		using StorageType = std::conditional_t<bReturnsVoid, uint8, ReturnType>;
		TTypeCompatibleBytes<StorageType> Result;
		auto Invoke = [&]<class T>(T& Value)
		{
			// Argument may come by const reference, but lambda should always receive pure type
			using CleanType = std::remove_const_t<T>;
			if constexpr (bReturnsVoid)
			{
				Functor.template operator()<CleanType>(SelectArg(Value));
			}
			else
			{
				new((ReturnType*)&Result) ReturnType(Functor.template operator()<CleanType>(SelectArg(Value)));
			}
		};
		((This.Data.template IsType<TOption<Type>>() ? void(Invoke(This.template GetValueChecked<Type>())) : void()), ...);
		if constexpr (!bReturnsVoid)
		{
			return (ReturnType&)Result;
		}
	}(This.Data);
}

template <class ThisType, class FunctorType>
decltype(auto) sketch::FAttribute::ApplySafeImpl(ThisType* This, FunctorType&& Functor, TDefaultValueType<FunctorType> DefaultValue)
{
	if (!This)
	[[unlikely]]
	{
		if constexpr (std::is_void_v<TReturnType<FunctorType>>)
		{
			return;
		}
		else
		{
			return DefaultValue;
		}
	}
	return ApplyImpl(*This, std::forward<FunctorType>(Functor));
}

decltype(auto) sketch::FAttributeCollectionHandle::operator<<(const auto& Callable) const
{
	if (auto* Array = Container.Get<TArray<FAttribute>*>(nullptr))
	{
		for (auto It = Array->CreateIterator(); It; ++It)
			Callable(*It, It);
	}
	else
	{
		for (auto It = Container.Get<TSparseArray<FAttribute>*>()->CreateIterator(); It; ++It)
			Callable(*It, It);
	}
}

inline sketch::FAttribute* sketch::FAttributeHandle::Get() const
{
	if (!CollectionHandle.Owner.IsValid())
		[[unlikely]]
			return nullptr;

	if (auto* Array = CollectionHandle.Container.TryGet<TArray<FAttribute>*>())
	{
		if (*Array && (*Array)->IsValidIndex(Index))
			[[likely]]
				return &(**Array)[Index];
		return nullptr;
	}
	else
	{
		auto& SparseArray = CollectionHandle.Container.Get<TSparseArray<FAttribute>*>();
		return SparseArray && SparseArray->IsValidIndex(Index) ? &(*SparseArray)[Index] : nullptr;
	}
}

template <class T>
T sketch::FAttributeHandle::GetValue(const T& DefaultValue) const
{
	if (FAttribute* Attribute = Get())
	[[likely]]
	{
		return Attribute->GetValueChecked<T>();
	}
	return DefaultValue;
}

template <class T>
bool sketch::FAttributeHandle::SetValue(const T& Value) const
{
	if (FAttribute* Attribute = Get())
	[[likely]]
	{
		Attribute->GetValueChecked<T>() = Value;
		return true;
	}
	return false;
}

template <class... ConstructorArgTypes>
template <class T>
	requires std::is_constructible_v<
		std::conditional_t<sketch::TIsSlateAttribute<T>::Value, typename sketch::TSlateAttributeType<T>::Type, T>,
		ConstructorArgTypes...
	>
sketch::TAttributeInitializer<ConstructorArgTypes...>::operator T()
{
	constexpr bool bAttribute = TIsSlateAttribute<T>::Value;
	using Type = std::conditional_t<bAttribute, typename TSlateAttributeType<T>::Type, T>;

	FSketchModule& Host = FSketchModule::Get();
	if constexpr (bAttribute)
	{
		TAttribute<Type> Attribute = Host.MakeSlateAttribute<Type>(*this);
		return Attribute;
	}
	else
	{
		auto Context = Host.FindOrCreateSketchAttribute<T, ConstructorArgTypes...>(*this); // TTuple arg deduction crashes compiler, hence auto
		FAttribute& Attribute = Context.template Get<FAttribute&>();
		Attribute.StaleCountdown += Attribute.StaleCountdown == 0 ? 2 : 1;

		using StorageType = typename TAttributeTraits<T>::FStorage;
		const StorageType& Storage = Attribute.GetValueChecked<StorageType>();
		decltype(auto) Value = TAttributeTraits<T>::GetValue(Storage); // GetValue returns a value rather than a reference facing an unsupported type, hence decltype(auto)
		return Value;
	}
}

template <class T, class OwnerType, class ContainerType>
sketch::TAttributeReader<T, OwnerType, ContainerType>::TAttributeReader(const TWeakPtr<const OwnerType>& Owner, ContainerType& Container, int Index)
	: Container(Container)
	  , Index(Index)
	  , Owner(Owner)
{
}

template <class T, class OwnerType, class ContainerType>
sketch::TAttributeReader<T, OwnerType, ContainerType>::TAttributeReader(const TAttributeReader& Other)
	: Container(Other.Container)
	  , Index(Other.Index)
	  , Owner(Other.Owner)
{
	if (Owner.IsValid() && Container.IsValidIndex(Index))
	[[likely]]
	{
		FAttribute& Attribute = Container[Index];
		++Attribute.NumUsers;
	}
}

template <class T, class OwnerType, class ContainerType>
sketch::TAttributeReader<T, OwnerType, ContainerType>::TAttributeReader(TAttributeReader&& Other)
	: Container(Other.Container)
	  , Index(Other.Index)
	  , Owner(Other.Owner)
{
	const_cast<int&>(Other.Index) = INDEX_NONE;
}

template <class T, class OwnerType, class ContainerType>
T sketch::TAttributeReader<T, OwnerType, ContainerType>::operator()() const
{
	if (Owner.IsValid() && Container.IsValidIndex(Index))
	[[likely]]
	{
		FAttribute& Attribute = Container[Index];
		using StorageType = typename TAttributeTraits<T>::FStorage;
		if (const auto* Storage = Attribute.GetValue<StorageType>())
		[[likely]]
		{
			Attribute.StaleCountdown = 2;
			return TAttributeTraits<T>::GetValue(*Storage);
		}
	}
	return {};
}

template <class T, class OwnerType, class ContainerType>
sketch::TAttributeReader<T, OwnerType, ContainerType>::~TAttributeReader()
{
	if (Index == INDEX_NONE)
		return; // Already moved

	if (Owner.IsValid() && Container.IsValidIndex(Index))
	[[likely]]
	{
		FAttribute& Attribute = Container[Index];
		--Attribute.NumUsers;
		// if (Attribute->NumUsers == 0)
		// {
		// 	Context->RemoveAt(Index);
		// 	Host.OnAttributesChanged.Broadcast();
		// }
	}
}

inline sketch::FFactory* sketch::FFactoryHandle::Resolve() const
{
	if (IsValid())[[likely]]
	{
		FSketchModule& Host = FSketchModule::Get();
		if (TArray<FFactory>* Factories = Host.Factories.Find(Type))
		{
			return Factories->FindByPredicate([this](const FFactory& Factory) { return Factory.Name == Name; });
		}
	}
	return nullptr;
}

// #include <iostream>
//
// void printArgTypes(const char* prefix, int i, const char* suffix)
// {
// 	for (int j = 0; j < i; ++j)
// 	{
// 		std::cout << prefix << "ArgType" << j << suffix << ", ";
// 	}
// 	std::cout << prefix << "ArgType" << i << suffix;
// }
//
// void printArgs(int i)
// {
// 	for (int j = 0; j < i; ++j)
// 	{
// 		std::cout << "ArgType" << j << "&& Arg" << j << ", ";
// 	}
// 	std::cout << "ArgType" << i << "&& Arg" << i << ", ";
// }
//
// void printTupleArgs(int i)
// {
// 	for (int j = 0; j < i; ++j)
// 	{
// 		std::cout << "std::forward<ArgType" << j << ">(" << "Arg" << j << ")" << ", ";
// 	}
// 	std::cout << "std::forward<ArgType" << i << ">(" << "Arg" << i << ")";
// }
//
// int main()
// {
// 	for (int i = 0; i < 8; ++i)
// 	{
// 		std::cout << "template <";
// 		printArgTypes("class ", i, "");
// 		std::cout << ">\n";
// 		std::cout << "sketch::TAttributeInitializer<";
// 		printArgTypes("std::remove_const_t<std::remove_reference_t<", i, ">>");
// 		std::cout << "> Sketch(const FName AttributeName, ";
// 		printArgs(i);
// 		std::cout << "const std::source_location& SourceLocation = std::source_location::current()) { return sketch::TAttributeInitializer<";
// 		printArgTypes("std::remove_const_t<std::remove_reference_t<", i, ">>");
// 		std::cout << ">{ AttributeName, MakeTuple(";
// 		printTupleArgs(i);
// 		std::cout << "), SourceLocation, SourceLocation.line(), SourceLocation.column() }; }\n";
// 	}
// 	return 0;
// }
