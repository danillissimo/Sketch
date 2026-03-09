#pragma once
#include <source_location>

#include "UObject/NameTypes.h"

class SSketchWidget;

namespace sketch
{
	class IAttributeImplementation;
}



/**
 * Primary module function - Sketch - is normally more important for a dev than namespace with internal stuff.
 * So "Sketch" is reserved for it, and namespace name begins from a lower case letter to avoid collision.
 * No, I didn't like any kinds of underscores and couldn't come up with a prefix/suffix I'd like.
 */
namespace sketch
{
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
	struct FAttribute : public TSharedFromThis<FAttribute>
	{
		FAttribute() = default;

		template <class T>
			requires std::is_base_of_v<IAttributeImplementation, T>
		FAttribute(
			const FSourceLocation& SourceLocation,
			uint32 Line,
			uint32 Column,
			const FName& Name,
			T* InDefaultValue
		) : Value(MakeShared<T>(const_cast<const T&>(*InDefaultValue))) // Make it clear that it's expected to call copy constructor, not random template constructor
		  , DefaultValue(TUniquePtr<IAttributeImplementation>(InDefaultValue))
		  , SourceLocation(SourceLocation)
		  , Line(Line)
		  , Column(Column)
		  , Name(Name) {}

		const TSharedPtr<IAttributeImplementation>& GetValue() const { return Value; }
		IAttributeImplementation* GetDefaultValue() const { return DefaultValue.Get(); }
		bool IsSetToDefault() const;
		bool IsDynamic() const { return NumUsers > 0; }
		const FSourceLocation& GetSourceLocation() const { return SourceLocation; }
		uint32 GetLine() const { return Line; }
		uint32 GetColumn() const { return Column; }
		FName GetName() const { return Name; }

		uint8 StaleCountdown = 0;
		uint16 NumUsers = 0;

		using FCodeGenerator = FString(*)(const FSlotBase*);
		using FMetaValue = TVariant<FEmptyVariantState, int64, FStringView, FCodeGenerator>;
		TMap<FName, FMetaValue> Meta;

	private:
		TSharedPtr<IAttributeImplementation> Value;
		TUniquePtr<IAttributeImplementation> DefaultValue;
		FSourceLocation SourceLocation;
		uint32 Line = 0;
		uint32 Column = 0;
		FName Name;
	};

	struct FConstAttributeCollection
	{
		using FConstSharedPointer = TSharedPtr<const TArray<const TSharedPtr<FAttribute>>>;
		using FConstSharedRef = TSharedRef<const TArray<const TSharedPtr<FAttribute>>>;
		FConstAttributeCollection() = default;
		FConstAttributeCollection(const TSharedPtr<TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedPointer&)InAttributes) {}
		FConstAttributeCollection(TSharedPtr<TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedPointer&)InAttributes)) {}
		FConstAttributeCollection(const TSharedPtr<const TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedPointer&)InAttributes) {}
		FConstAttributeCollection(TSharedPtr<const TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedPointer&)InAttributes)) {}
		FConstAttributeCollection(const TSharedPtr<const TArray<const TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedPointer&)InAttributes) {}
		FConstAttributeCollection(TSharedPtr<const TArray<const TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedPointer&)InAttributes)) {}
		FConstAttributeCollection(const TSharedRef<TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedRef&)InAttributes) {}
		FConstAttributeCollection(TSharedRef<TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedRef&)InAttributes)) {}
		FConstAttributeCollection(const TSharedRef<const TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedRef&)InAttributes) {}
		FConstAttributeCollection(TSharedRef<const TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedRef&)InAttributes)) {}
		FConstAttributeCollection(const TSharedRef<const TArray<const TSharedPtr<FAttribute>>>& InAttributes) : Attributes((FConstSharedRef&)InAttributes) {}
		FConstAttributeCollection(TSharedRef<const TArray<const TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp((FConstSharedRef&)InAttributes)) {}

		auto* operator->() const { return Attributes.Get(); }
		auto& operator*() const { return *Attributes; }
		operator bool() const { return Attributes.IsValid(); }

		TSharedPtr<const TArray<const TSharedPtr<FAttribute>>> Attributes;
	};

	struct FAttributeCollection
	{
		FAttributeCollection() = default;
		FAttributeCollection(EInPlace) : Attributes(MakeShared<TArray<TSharedPtr<FAttribute>>>()) {}
		FAttributeCollection(const TSharedPtr<TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes(InAttributes) {}
		FAttributeCollection(TSharedPtr<TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp(InAttributes)) {}
		FAttributeCollection(const TSharedRef<TArray<TSharedPtr<FAttribute>>>& InAttributes) : Attributes(InAttributes) {}
		FAttributeCollection(TSharedRef<TArray<TSharedPtr<FAttribute>>>&& InAttributes) : Attributes(MoveTemp(InAttributes)) {}

		auto* operator->() const { return Attributes.Get(); }
		auto& operator*() const { return *Attributes; }
		operator bool() const { return Attributes.IsValid(); }
		operator FConstAttributeCollection&() const { return *(FConstAttributeCollection*)this; }

		TSharedPtr<TArray<TSharedPtr<FAttribute>>> Attributes;
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

	template <class T>
	constexpr bool TIsSlateAttribute = !std::is_void_v<typename TSlateAttributeType<T>::Type>;

	template <class T>
	using TInitializedType = std::conditional_t<TIsSlateAttribute<T>, typename TSlateAttributeType<T>::Type, T>;

	/** Intermediate type that holds any required data between an attribute construction request and attribute construction */
	template <class... ConstructorArgTypes>
	struct TAttributeInitializer
	{
		FName AttributeName;
		[[no_unique_address]] // 'Cause it may be empty
		::TTuple<ConstructorArgTypes...> ConstructorArgs;
		FSourceLocation SourceLocation = { NoInit };
		uint32 Line = 0;
		uint32 Column = 0;



		TMap<FName, FAttribute::FMetaValue> Meta;

		TAttributeInitializer& AddMeta(const FName& Key) { return Meta.Add(Key), *this; }
		TAttributeInitializer& AddMeta(const FName& Key, FStringView Value) { return Meta.Add(Key, FAttribute::FMetaValue(TInPlaceType<FStringView>{}, Value)), *this; }
		TAttributeInitializer& AddMeta(const FName& Key, FAttribute::FCodeGenerator CodeGenerator) { return Meta.Add(Key, FAttribute::FMetaValue(TInPlaceType<FAttribute::FCodeGenerator>{}, CodeGenerator)), *this; }

		template <class T>
			requires std::is_integral_v<T>
		TAttributeInitializer& AddMeta(const FName& Key, T&& Value) { return Meta.Add(Key, FAttribute::FMetaValue(TInPlaceType<int64>{}, static_cast<int64>(Value))), *this; }



		template <class T>
			requires std::is_constructible_v<sketch::TInitializedType<T>, ConstructorArgTypes...>
		operator T();
	};

	template <class T>
	struct TAttributeReader
	{
		TWeakPtr<FAttribute> WeakAttribute;

		TAttributeReader(FAttribute& Attribute);
		TAttributeReader(const TAttributeReader& Other);
		TAttributeReader(TAttributeReader&& Other);
		T operator()() const;
		~TAttributeReader();
	};

	struct FFactory
	{
		FName Name;
		TFunction<TSharedRef<SWidget>(SWidget* WidgetToTakeUniqueSlotsForm)> ConstructWidget;
		using FUniqueSlots = TArray<SSketchWidget*, TInlineAllocator<4>>;
		TFunction<FUniqueSlots(SWidget& Widget)> EnumerateUniqueSlots;
		using FDynamicSlotTypes = TArray<FName, TInlineAllocator<1>>;
		TFunction<FDynamicSlotTypes(const SWidget& Widget)> EnumerateDynamicSlotTypes;
		TFunction<FSlotBase&(SWidget& Widget, const FName& SlotType)> ConstructDynamicSlot;
		TFunction<void(SWidget& Widget, const FName& SlotType, int Index, FSlotBase& Slot)> DestroyDynamicSlot;
	};

	/** 
	 * Every widget factory should end (or begin) from calling this macro - it sets up attributes inherited by each widget from SWidget 
	 * @todo Move this to SketchHeaderToolTypes.h
	 */
#define SKETCH_WIDGET_FACTORY_BOILERPLATE()\
	 ToolTipText(Sketch("ToolTipText",                         FText                            {}))\
	/*.ToolTip(Sketch("ToolTip",                                 TSharedPtr<IToolTip>             {}))*/\
	.Cursor(Sketch("Cursor",                                   TOptional<EMouseCursor::Type>    {}))\
	.IsEnabled(Sketch("IsEnabled",                             bool                             { true }))\
	.Visibility(Sketch("Visibility",                           EVisibility                      { EVisibility::Visible }))\
	.ForceVolatile(Sketch("ForceVolatile",                     bool                             { false }))\
	.Clipping(Sketch("Clipping",                               EWidgetClipping                  { EWidgetClipping::Inherit }))\
	.PixelSnappingMethod(Sketch("PixelSnappingMethod",         EWidgetPixelSnapping             { EWidgetPixelSnapping::Inherit }))\
	.FlowDirectionPreference(Sketch("FlowDirectionPreference", EFlowDirectionPreference         { EFlowDirectionPreference::Inherit }))\
	.RenderOpacity(Sketch("RenderOpacity",                     float                            { 1.f }))\
	.RenderTransform(Sketch("RenderTransform",                 TOptional<FSlateRenderTransform> {}))\
	.RenderTransformPivot(Sketch("RenderTransformPivot",       FVector2D                        {}))\
	/*.Tag(Sketch("Tag",                                         FName                            {}))*/\
	/*.AccessibleParams(Sketch("AccessibleParams",               TOptional<FAccessibleWidgetData> {}))*/\
	/*.AccessibleText(Sketch("AccessibleText",                   FText                            {}))*/

	struct FFactoryHandle
	{
		FFactory* Resolve() const;
		bool IsValid() const { return !Type.IsNone() && !Name.IsNone(); }

		FName Type;
		FName Name;
		bool operator==(const FFactoryHandle& Other) const { return Type == Other.Type && Name == Other.Name; }
	};
}
