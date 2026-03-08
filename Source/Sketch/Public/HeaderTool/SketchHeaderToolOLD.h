#pragma once

class SSketchWidget;

/** @note Totally real unlike the regular one! */
namespace sketch::HeaderTool
{
	struct FProcessedString
	{
		/** May rely on source string or on Container if any processing took place */
		FStringView String;
		/** Only contains data if source string needed modifications */
		FString Container;
	};

	struct FScope
	{
		int Start = INDEX_NONE;
		int End = INDEX_NONE;
		int Length() const { return End - Start; }
		bool Contains(int Position) const { return Position > Start && Position < End; }
		bool Contains(const FScope& Scope) const { return Scope.Start > Start && Scope.End < End; }
	};

	/** Nice try. Sadly failed to implement without deep code analysis in a one-pass algorithm */
	// enum EExpressionType
	// {
	// 	ET_Unknown,      // Should never actually appear
	// 	ET_Preprocessor, // Anything beginning with a "#"
	// 	ET_Comment,      // Comments, obviously
	// 	ET_Subscope,     // Anything surrounded by "{}"
	// 	ET_Functional,   // Anything surrounded by "()"
	// 	ET_Subscript,    // Anything surrounded by "[]"
	// 	ET_Generic,      // Everything from ending of last generic expression (including empty expressions) followed by "{}" or ending with a ";"
	//
	// 	// ET_TemplateArguments, // Anything surrounded by "<>" - can't really differ them from comparison operators without deep analysis
	// };
	// struct FExpression
	// {
	// 	EExpressionType Type = ET_Unknown;
	// 	int Start = INDEX_NONE;
	// 	int End = INDEX_NONE;
	// 	int Length() const { return End - Start; }
	// };

	struct FIndex
	{
		/** @todo Remove */
		FIndex() = default;
		FIndex(const FStringView& Code);

		TArray<FScope> Scopes;
		TArray<int> Lines;

		struct FPosition
		{
			int Line = INDEX_NONE;
			int Column = INDEX_NONE;
		};
		FPosition LocatePosition(int Position) const;
		FPosition LocatePosition(const FString& Code, const FStringView& View) const;
		/** @return INDEX_NONE for global scope */
		int FindScope(int Position) const;
	};



	struct FProperty
	{
		FProcessedString Type;
		FStringView Name;
		TOptional<FProcessedString> DefaultValue;
		FProcessedString CustomArgumentInitializationCode;
		FProcessedString CustomEntityInitializationCode;
		bool WithCustomInitialization() const { return !CustomArgumentInitializationCode.String.IsEmpty() || !CustomEntityInitializationCode.String.IsEmpty(); }
		bool bSupported = false;
	};
	struct FSlot
	{
		FStringView Name;

		/** Only set for dynamic slots */
		FStringView TypeName;
		TArray<FProperty> Properties;
	};
	struct FClass
	{
		FStringView Name;
		TArray<FProperty> Properties;
		TArray<FSlot, TInlineAllocator<4>> NamedSlots;
		TArray<FSlot, TInlineAllocator<1>> DynamicSlots;
	};
	struct FFile
	{
		FString Path;
		FString Contents;
		TArray<FClass> Classes;

		bool IsValid() const { return !Contents.IsEmpty(); }
	};



	struct FPropertyAnchors
	{
		int FirstBracket = INDEX_NONE;
		FStringView Type;
		FStringView Name;
		int SecondBracket = INDEX_NONE;

		FPropertyAnchors() = default;
		FPropertyAnchors(const FStringView& PropertyAndFurther, const auto& LogErr, auto&&... Args);

		bool IsValid() const { return FirstBracket != INDEX_NONE && SecondBracket != INDEX_NONE && !Type.IsEmpty() && !Name.IsEmpty(); }
		operator bool() const { return IsValid(); }
	};
	struct FAnchoredProperty
	{
		FProperty Property;
		FPropertyAnchors Anchors;
	};



	/** @note Totally real unlike the regular one! */
	class SKETCH_API FHeaderTool_BASE
	{
	public:
		enum ESlotProperties
		{
			SP_None                         = 0,
			SP_ConstructorInherited         = 1 << 0,
			SP_DestructorInherited          = 1 << 1,
			SP_SlotOperatorsUsesGeneralName = 1 << 2,
		};
		struct FOverride
		{
			FStringView Property;
			FStringView ValueOverride;
			FStringView TypeOverride;

			FStringView SlotType;
			ESlotProperties SlotProperties = SP_None;

			static constexpr FOverride Value(FStringView Property, FStringView Value, FStringView SlotType = {}) { return FOverride{ .Property = Property, .ValueOverride = Value, .SlotType = SlotType }; }
			static constexpr FOverride Type(FStringView Property, FStringView Type, FStringView SlotType = {}) { return FOverride{ .Property = Property, .TypeOverride = Type, .SlotType = SlotType }; }
			static constexpr FOverride Slot(FStringView SlotType, int Properties) { return FOverride{ .SlotType = SlotType, .SlotProperties = ESlotProperties(Properties) }; }
		};
		static TMap<FName, TArray<FOverride>> ClassOverrides;

		static TArray<FFile> Scan(const FString& Path, bool bRecursive);
		static FFile Scan(const FString& FilePath);
		static FString GenerateReflectionPrologue();
		static FString GenerateReflectionEpilogue(const FString& InclusionRoot);
		static void GenerateReflection(
			const FString& FilePath,
			const FString& InclusionRoot,
			const FClass& Class,
			FString& Prologue,
			FString& Code,
			FString& Epilogue
		);
		static FString CombineReflection(const FString& Prologue, const FString& Code, const FString& Epilogue);

	private:
		static TOptional<FIndex> IndexCode(const FString& Code);
		static bool ProcessCode(const FString& Code, TArray<FClass>& OutClasses);
		static TOptional<FAnchoredProperty> ProcessSlateProperty(const FIndex& Index, const FString& Code, const FStringView& ClassName, const FStringView& PropertiesDefaults, const FStringView& PropertyAndFurther, const FStringView& SlotTypeName);
		static TOptional<TTuple<FSlot, FPropertyAnchors>> ProcessUniqueSlateSlot(const FIndex& Index, const FString& Code, const FStringView& PropertyAndFurther);
		static TOptional<FSlot> ProcessDynamicSlot(const FIndex& Index, const FString& Code, int ClassScopeIndex, const FStringView& ClassName, const FStringView& PropertyAndFurther);
		static TOptional<FProcessedString> ProcessSlateArgumentDefaultValue(const FIndex& Index, const FString& Code, const FStringView& PropertyAndFurther, FPropertyAnchors& Anchors);
	};
}
