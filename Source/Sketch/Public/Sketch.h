#pragma once
#ifndef WITH_SKETCH
#error "WITH_SKETCH is not defined, fix Sketch.Build.cs"
#endif
#if !WITH_SKETCH
#include "NoSketch.h"
#else
#include "SketchCore.h"
#include "AttributesTraites/AllTraits.h"


// Template generator is in the end of the file
inline sketch::TAttributeInitializer<> Sketch(FName AttributeName, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<>{ AttributeName, MakeTuple(), S, S.line(), S.column() }; }

template <class AT0>
sketch::TAttributeInitializer<std::decay_t<AT0>> Sketch(FName AttributeName, AT0&& A0, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>>{ AttributeName, MakeTuple(std::forward<AT0>(A0)), S, S.line(), S.column() }; }

template <class AT0, class AT1>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2, class AT3>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, AT3&& A3, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2), std::forward<AT3>(A3)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2, class AT3, class AT4>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, AT3&& A3, AT4&& A4, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2), std::forward<AT3>(A3), std::forward<AT4>(A4)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2, class AT3, class AT4, class AT5>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, AT3&& A3, AT4&& A4, AT5&& A5, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2), std::forward<AT3>(A3), std::forward<AT4>(A4), std::forward<AT5>(A5)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2, class AT3, class AT4, class AT5, class AT6>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>, std::decay_t<AT6>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, AT3&& A3, AT4&& A4, AT5&& A5, AT6&& A6, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>, std::decay_t<AT6>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2), std::forward<AT3>(A3), std::forward<AT4>(A4), std::forward<AT5>(A5), std::forward<AT6>(A6)), S, S.line(), S.column() }; }

template <class AT0, class AT1, class AT2, class AT3, class AT4, class AT5, class AT6, class AT7>
sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>, std::decay_t<AT6>, std::decay_t<AT7>> Sketch(FName AttributeName, AT0&& A0, AT1&& A1, AT2&& A2, AT3&& A3, AT4&& A4, AT5&& A5, AT6&& A6, AT7&& A7, const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<std::decay_t<AT0>, std::decay_t<AT1>, std::decay_t<AT2>, std::decay_t<AT3>, std::decay_t<AT4>, std::decay_t<AT5>, std::decay_t<AT6>, std::decay_t<AT7>>{ AttributeName, MakeTuple(std::forward<AT0>(A0), std::forward<AT1>(A1), std::forward<AT2>(A2), std::forward<AT3>(A3), std::forward<AT4>(A4), std::forward<AT5>(A5), std::forward<AT6>(A6), std::forward<AT7>(A7)), S, S.line(), S.column() }; }



namespace sketch
{
	namespace Private
	{
		TAttribute<FSlateFontInfo> DefaultFont();
	}
}

template <class T, class... ConstructorArgTypes>
TAttribute<T> FSketchCore::MakeSlateAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	using namespace sketch;

	// Retrieve context
	auto AttributeAndIndex = FindOrCreateSketchAttribute<T, ConstructorArgTypes...>(Initializer);
	FAttribute& Attribute = AttributeAndIndex.template Get<FAttribute&>();

	// Construct attribute
	return TAttribute<T>::CreateLambda(TAttributeReader<T>(Attribute));
}

template <class T, class... ConstructorArgTypes>
T FSketchCore::ReadAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	auto Context = FindOrCreateSketchAttribute<T, ConstructorArgTypes...>(Initializer); // TTuple arg deduction crashes compiler, hence auto
	sketch::FAttribute& Attribute = Context.template Get<sketch::FAttribute&>();
	Attribute.StaleCountdown += Attribute.StaleCountdown == 0 ? 2 : 1;

	decltype(auto) Value = sketch::TAttributeTraits<T>::GetValue(*Attribute.GetValue()); // GetValue returns a value rather than a reference facing an unsupported type, hence decltype(auto)
	static_assert(std::is_same_v<T, std::decay_t<decltype(Value)>>);
	return Value;
}

template <class T, class... ConstructorArgTypes>
TTuple<sketch::FAttribute&, int> FSketchCore::FindOrCreateSketchAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer)
{
	// Consider custom attributes
	if (!CustomAttributes.IsEmpty())
	{
		return FindOrCreateSketchAttributeImplementation<T>(*CustomAttributes.Last(), Initializer);
	}

	// Use global attributes
	using namespace sketch;
	FAttributeCollection* Context = Attributes.Find(Initializer.SourceLocation);
	if (!Context)
	{
		Context = &Attributes.Emplace(Initializer.SourceLocation, InPlace);
	}
	return FindOrCreateSketchAttributeImplementation<T>(**Context, Initializer);
}

template <class T, class... ConstructorArgTypes>
TTuple<sketch::FAttribute&, int> FSketchCore::FindOrCreateSketchAttributeImplementation(
	TArray<TSharedPtr<sketch::FAttribute>>& Container,
	sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer
)
{
	using namespace sketch;

	// Find existing attribute
	int Index = Container.IndexOfByPredicate(
		[&](const TSharedPtr<FAttribute>& Attribute)
		{
			return Attribute->GetLine() == Initializer.Line
				&& Attribute->GetColumn() == Initializer.Column
				// Surprisingly multiple attributes CAN share the same line and the same column when they come from a macro
				&& Attribute->GetName() == Initializer.AttributeName;
		}
	);

	// Or create a new one
	if (Index == INDEX_NONE)
	{
		auto MakeValue = [&](ConstructorArgTypes&... Args)
		{
			using FValue = decltype(TAttributeTraits<T>::ConstructValue(MoveTemp(Args)...));
			FValue* Value = new FValue(TAttributeTraits<T>::ConstructValue(MoveTemp(Args)...));
			return Value;
		};
		Index = Container.Emplace(
			MakeShared<FAttribute>(
				Initializer.SourceLocation,
				Initializer.Line,
				Initializer.Column,
				Initializer.AttributeName,
				Initializer.ConstructorArgs.ApplyAfter(MakeValue)
			)
		);
		Container[Index]->Meta = MoveTemp(Initializer.Meta);
		OnAttributesChanged.Broadcast();
	}
	return { *Container[Index], Index };
}

inline bool sketch::FAttribute::IsSetToDefault() const
{
	return Value->Equals(*DefaultValue.Get());
}

template <class... ConstructorArgTypes>
template <class T>
	requires std::is_constructible_v<sketch::TInitializedType<T>, ConstructorArgTypes...>
sketch::TAttributeInitializer<ConstructorArgTypes...>::operator T()
{
	FSketchCore& Core = FSketchCore::Get();
	if constexpr (TIsSlateAttribute<T>)
	{
		return Core.MakeSlateAttribute<TInitializedType<T>, ConstructorArgTypes...>(*this);
	}
	else
	{
		return Core.ReadAttribute<T, ConstructorArgTypes...>(*this);
	}
}

template <class T>
sketch::TAttributeReader<T>::TAttributeReader(FAttribute& Attribute)
	: WeakAttribute(Attribute.AsWeak())
{
	++Attribute.NumUsers;
}

template <class T>
sketch::TAttributeReader<T>::TAttributeReader(const TAttributeReader& Other)
	: WeakAttribute(Other.WeakAttribute)
{
	if (TSharedPtr<FAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
	{
		Attribute->NumUsers++;
	}
}

template <class T>
sketch::TAttributeReader<T>::TAttributeReader(TAttributeReader&& Other)
	: WeakAttribute(MoveTemp(Other.WeakAttribute))
{
}

template <class T>
T sketch::TAttributeReader<T>::operator()() const
{
	if (TSharedPtr<FAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
	{
		return TAttributeTraits<T>::GetValue(*Attribute->GetValue());
	}
	return T{};
}

template <class T>
sketch::TAttributeReader<T>::~TAttributeReader()
{
	if (TSharedPtr<FAttribute> Attribute = WeakAttribute.Pin()) [[likely]]
	{
		Attribute->NumUsers--;
	}
}

inline sketch::FFactory* sketch::FFactoryHandle::Resolve() const
{
	if (IsValid())[[likely]]
	{
		FSketchCore& Host = FSketchCore::Get();
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
// 		std::cout << prefix << "AT" << j << suffix << ", ";
// 	}
// 	std::cout << prefix << "AT" << i << suffix;
// }
//
// void printArgs(int i)
// {
// 	for (int j = 0; j < i; ++j)
// 	{
// 		std::cout << "AT" << j << "&& A" << j << ", ";
// 	}
// 	std::cout << "AT" << i << "&& A" << i << ", ";
// }
//
// void printTupleArgs(int i)
// {
// 	for (int j = 0; j < i; ++j)
// 	{
// 		std::cout << "std::forward<AT" << j << ">(" << "A" << j << ")" << ", ";
// 	}
// 	std::cout << "std::forward<AT" << i << ">(" << "A" << i << ")";
// }
//
// int main()
// {
//  std::cout << "inline ";
// 	std::cout << "sketch::TAttributeInitializer<";
// 	std::cout << "> Sketch(FName AttributeName, ";
// 	std::cout << "const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<";
// 	std::cout << ">{ AttributeName, MakeTuple(";
// 	std::cout << "), S, S.line(), S.column() }; }\n";
// 	for (int i = 0; i < 8; ++i)
// 	{
// 		std::cout << "template <";
// 		printArgTypes("class ", i, "");
// 		std::cout << ">\n";
// 		std::cout << "sketch::TAttributeInitializer<";
// 		printArgTypes("std::decay_t<", i, ">");
// 		std::cout << "> Sketch(FName AttributeName, ";
// 		printArgs(i);
// 		std::cout << "const std::source_location& S = std::source_location::current()) { return sketch::TAttributeInitializer<";
// 		printArgTypes("std::decay_t<", i, ">");
// 		std::cout << ">{ AttributeName, MakeTuple(";
// 		printTupleArgs(i);
// 		std::cout << "), S, S.line(), S.column() }; }\n";
// 	}
// 	return 0;
// }
#endif
