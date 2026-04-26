#pragma once
#include "SketchTypes.h"

struct FSlateFontInfo;

class FSketchCore : FNoncopyable
{
public:
	SKETCH_API static FSketchCore& Get();
	SKETCH_API static void Initialize();
	SKETCH_API static void Shutdown();

	SKETCH_API static TAttribute<FSlateFontInfo> GetDefaultFont();

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



	const TWeakPtr<SSketchWidget>& GetWidgetEditorTarget() const { return WidgetEditorTarget; }
	SKETCH_API void SetWidgetEditorTarget(SSketchWidget& NewTarget);
	SKETCH_API void ResetWidgetEditorTarget();
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWidgetEditorTargetChanged, SSketchWidget*);
	FOnWidgetEditorTargetChanged OnWidgetEditorTargetChanged;

private:
	FSketchCore() = default;

	template <class T, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	template <class T, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttributeImplementation(TArray<TSharedPtr<sketch::FAttribute>>& Container, sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);

	TMap<sketch::FSourceLocation, sketch::FAttributeCollection> Attributes;
	TArray<sketch::FAttributeCollection, TInlineAllocator<1>> CustomAttributes;
	TWeakPtr<SSketchWidget> WidgetEditorTarget;
};
