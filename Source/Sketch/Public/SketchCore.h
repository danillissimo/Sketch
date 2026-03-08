#pragma once
#include "SketchTypes.h"

class FSketchCore
{
public:
	SKETCH_API static FSketchCore& Get();
	SKETCH_API static void Initialize();
	SKETCH_API static void Shutdown();

	using FNomadAttributes = TMap<const sketch::FSourceLocation, const sketch::FConstAttributeCollection>;
	const FNomadAttributes& GetNomadAttributes() const { return (FNomadAttributes&)Attributes; }



	template <class T, class... ConstructorArgTypes>
	TAttribute<T> MakeSlateAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	template <class T, class... ConstructorArgTypes>
	T ReadAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	void RedirectNewAttributesInto(const sketch::FAttributeCollection& Container) { CustomAttributes.Push(Container); }
	void StopRedirectingNewAttributes() { CustomAttributes.Pop(); }
	SKETCH_API void ClearStaleAttributes();

	DECLARE_MULTICAST_DELEGATE(FOnAttributesChanged);
	FOnAttributesChanged OnAttributesChanged;



	TMap<FName, TArray<sketch::FFactory>> Factories;
	/** QOL helper */
	SKETCH_API void RegisterFactory(const FName& Type, sketch::FFactory&& Factory);

private:
	FSketchCore() = default;

	template <class T, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	template <class T, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttributeImplementation(TArray<TSharedPtr<sketch::FAttribute>>& Container, sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);

	TMap<sketch::FSourceLocation, sketch::FAttributeCollection> Attributes;
	TArray<sketch::FAttributeCollection, TInlineAllocator<1>> CustomAttributes;
};
