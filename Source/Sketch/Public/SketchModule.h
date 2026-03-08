#pragma once
#include "Modules/ModuleManager.h"
#include "SketchTypes.h"

class FSketchModule : public IModuleInterface
{
public:
	static FSketchModule& Get() { return FModuleManager::Get().GetModuleChecked<FSketchModule>("Sketch"); }

	// SKETCH_API TTuple<const TSparseArray<sketch::FAttribute>*, const sketch::FAttribute*> FindAttribute(const sketch::FAttributePath& Path) const;
	// SKETCH_API TTuple<const TSparseArray<sketch::FAttribute>*, sketch::FAttribute*> FindAttribute(const sketch::FAttributePath& Path);
	// const sketch::FAttribute* operator[](const sketch::FAttributePath& Path) const { return FindAttribute(Path).Value; }
	// sketch::FAttribute* operator[](const sketch::FAttributePath& Path) { return FindAttribute(Path).Value; }

	template <class T, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	template <class T, class... ConstructorArgTypes>
	TAttribute<T> MakeSlateAttribute(sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);
	SKETCH_API void ClearStaleAttributes();

	SKETCH_API void RegisterFactory(const FName& Type, sketch::FFactory&& Factory);
	void RedirectNewAttributesInto(const SWidget& Widget, TArray<sketch::FAttribute>& Container) { CustomAttributesOwner = Widget.AsWeak(), CustomAttributes = &Container; }
	void RedirectNewAttributesInto(const TSharedPtr<TArray<sketch::FAttribute>>& Container) { CustomAttributesOwner = Container, CustomAttributes = Container.Get(); }
	void StopRedirectingNewAttributes() { CustomAttributesOwner.Reset(), CustomAttributes = nullptr; }

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

private:
	template <class T, class ContainerType, class... ConstructorArgTypes>
	TTuple<sketch::FAttribute&, int> FindOrCreateSketchAttributeImplementation(ContainerType& Container, sketch::TAttributeInitializer<ConstructorArgTypes...>& Initializer);

	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<class FUICommandList> PluginCommands;



	TMap<sketch::FSourceLocation, TSharedPtr<TSparseArray<sketch::FAttribute>>> Attributes;
	friend class SSketchController;
	friend class SSketchControllerOLD;
	DECLARE_MULTICAST_DELEGATE(FOnAttributesChanged);
	FOnAttributesChanged OnAttributesChanged;

public:
	TMap<FName, TArray<sketch::FFactory>> Factories;
	TArray<sketch::FFactoryHandle> FactoryTypes;
	friend class SSketch;

	TArray<sketch::FAttribute>* CustomAttributes = nullptr;
	TWeakPtr<const void> CustomAttributesOwner;
};
