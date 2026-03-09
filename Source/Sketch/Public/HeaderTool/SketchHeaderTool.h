#pragma once
#include "ConstexprStringView.h"
#include "SourceCodeTypes.h"
#include "SourceCodeUtility.h"

#ifdef __RESHARPER__
// Resharper denies analyzing non-free methods. Here it goes, even though colors it as an error.
#define FORMATTER(FormatIndex, FirstArgIndex) [[rscpp::format(wprintf, FormatIndex, FirstArgIndex)]] static
#else
#define FORMATTER(FormatIndex, FirstArgIndex)
#endif

namespace sketch::HeaderTool
{
	struct FSourceLine
	{
		int Line = 0;
		int Column = 0;
	};

	struct FLog
	{
		///
		struct FMessage
		{
			ELogVerbosity::Type Verbosity = ELogVerbosity::Display;
			FSourceLine Source = { 1, 1 };
			int FileIndex = INDEX_NONE;
			int ClassIndex = INDEX_NONE;
			int SlotIndex = INDEX_NONE;
			FString Message;
		};

		///
		const TArray<FMessage>& GetMessages() const { return Messages; }
		const TArray<FString>& GetFiles() const { return Files; }
		const TArray<FString>& GetClasses() const { return Classes; }
		const TArray<FString>& GetSlots() const { return Slots; }

		mutable FSimpleMulticastDelegate OnNewFile;
		mutable FSimpleMulticastDelegate OnMessageAdded;
		mutable FSimpleMulticastDelegate OnFileClosed;
		mutable FSimpleMulticastDelegate OnReset;

		/// Grouping
		void BeginFile(const sketch::FStringView& FileName) { Files.Emplace(FileName.ToString()), CurrentFile = Files.Num() - 1, OnNewFile.Broadcast(); }
		void BeginClass(const sketch::FStringView& ClassName) { Classes.Emplace(ClassName.ToString()), CurrentClass = Classes.Num() - 1; }
		void BeginSlot(const sketch::FStringView& SlotName) { Slots.Emplace(SlotName.ToString()), CurrentSlot = Slots.Num() - 1; }
		void EndSlot() { End(CurrentSlot, Slots, NumSlotMessages); }
		void EndClass() { EndSlot(), End(CurrentClass, Classes, NumClassMessages); }
		void EndFile() { EndClass(), End(CurrentFile, Files, NumFileMessages, &OnFileClosed); }
		void Reset() { Messages.Reset(), Files.Reset(), Classes.Reset(), Slots.Reset(), CurrentFile = CurrentClass = CurrentSlot = INDEX_NONE, NumFileMessages = NumClassMessages = NumSlotMessages = 0, OnReset.Broadcast(); }

		/// Logging
		FORMATTER(2, 4)
		void operator()(ELogVerbosity::Type Verbosity, const TCHAR* Format, const FSourceLine& SourceLine, const auto&... Args)
		{
			UE::Core::TCheckedFormatString<TCHAR, std::decay_t<decltype(Args)>...> UncheckedFormat(TEXT("Why the actual !#&@ do I have to do this"));
			UncheckedFormat.FormatString = Format;
			Messages.Emplace(Verbosity, SourceLine, CurrentFile, CurrentClass, CurrentSlot, FString::Printf(UncheckedFormat, Args...));
			NumFileMessages += CurrentFile != INDEX_NONE;
			NumClassMessages += CurrentClass != INDEX_NONE;
			NumSlotMessages += CurrentSlot != INDEX_NONE;
			OnMessageAdded.Broadcast();
		}

		FORMATTER(1, 3)
		void Err(const TCHAR* Format, const FSourceLine& SourceLine, const auto&... Args) { operator()(ELogVerbosity::Error, Format, SourceLine, std::forward<decltype(Args)>(Args)...); }

		FORMATTER(1, 3)
		void Warn(const TCHAR* Format, const FSourceLine& SourceLine, const auto&... Args) { operator()(ELogVerbosity::Warning, Format, SourceLine, std::forward<decltype(Args)>(Args)...); }

		FORMATTER(1, 3)
		void Display(const TCHAR* Format, const FSourceLine& SourceLine, const auto&... Args) { operator()(ELogVerbosity::Display, Format, SourceLine, std::forward<decltype(Args)>(Args)...); }

		/// Contents
	private:
		FORCENOINLINE static void End(int& Pointer, TArray<FString>& Array, int& Counter, const FSimpleMulticastDelegate* Event = nullptr)
		{
			if (Pointer != INDEX_NONE)
			{
				if (!Counter)
				{
					Array.Pop(EAllowShrinking::No);
				}
				else
				{
					Counter = 0;
					if (Event)
					{
						Event->Broadcast();
					}
				}
				Pointer = INDEX_NONE;
			}
		}

		TArray<FString> Files;
		TArray<FString> Classes;
		TArray<FString> Slots;
		int CurrentFile = INDEX_NONE;
		int CurrentClass = INDEX_NONE;
		int CurrentSlot = INDEX_NONE;
		int NumFileMessages = 0;
		int NumClassMessages = 0;
		int NumSlotMessages = 0;
		TArray<FMessage> Messages;
	};
}

namespace sketch::HeaderTool::Private
{
	struct FScope
	{
		int Start = INDEX_NONE;
		int End = INDEX_NONE;
		int Length() const { return End - Start; }
		bool Contains(int Position) const { return Position > Start && Position < End; }
		bool Contains(const FScope& Scope) const { return Scope.Start > Start && Scope.End < End; }
		sketch::FStringView ToView(const sketch::FStringView& Code) const { return { &Code[0] + Start + 1, Length() - 2 }; }
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
		FIndex() = default;
		FIndex(const sketch::FStringView& Code, FLog& Log);

		TArray<FScope> Scopes;
		TArray<int> Lines;

		bool IsValid() const { return !Lines.IsEmpty() /* File may contain no scopes, but always contains at least one line */; }

		FSourceLine LocatePosition(int Position) const;
		FSourceLine LocatePosition(const FString& Code, const sketch::FStringView& View) const;
		FSourceLine LocatePosition(const FString& Code, const FStringView& View, int Offset) const;
		/** @return INDEX_NONE for global scope */
		int FindScope(int Position) const;
		/** @return INDEX_NONE for global scope */
		int FindOuterScope(int ScopeIndex) const;
		FScope GetScope(const sketch::FStringView& Code, int ScopeIndex);
	};
}

/** @note Totally real unlike the regular one! */
namespace sketch::HeaderTool
{
	///
	/// Reflection
	///
	struct FProperty
	{
		bool IsValid() const { return !Type.String.IsEmpty() && !Name.IsEmpty(); }
		SourceCode::FProcessedString Type;
		sketch::FStringView Name;
		SourceCode::FProcessedString DefaultValue;
		SourceCode::FProcessedString CustomArgumentInitializationCode;
		SourceCode::FProcessedString CustomEntityInitializationCode;
		bool WithCustomInitialization() const { return !CustomArgumentInitializationCode.String.IsEmpty() || !CustomEntityInitializationCode.String.IsEmpty(); }
		bool bSupported = false;
	};

	struct FSlot
	{
		bool IsValid() const { return !Name.IsEmpty(); }

		/** */
		sketch::FStringView Name;
		/** Only set for dynamic slots */
		sketch::FStringView Type;
		/** Only set for dynamic slots */
		TArray<FProperty> Properties;
	};

	struct FClass
	{
		sketch::FStringView Name;
		TArray<FProperty> Properties;
		TArray<FSlot, TInlineAllocator<4>> NamedSlots;
		TArray<FSlot, TInlineAllocator<1>> DynamicSlots;
	};



	///
	/// Overrides
	///
	enum ESlotProperties
	{
		SP_None                         = 0,
		SP_ConstructorInherited         = 1 << 0,
		SP_DestructorInherited          = 1 << 1,
		SP_SlotOperatorsUsesGeneralName = 1 << 2,
	};
	struct FOverride
	{
		sketch::FStringView Name;
		sketch::FStringView TypeOverride;
		sketch::FStringView ValueOverride;

		sketch::FStringView SlotType;
		ESlotProperties SlotProperties = SP_None;

		static constexpr FOverride Value(sketch::FStringView Property, sketch::FStringView Value, sketch::FStringView SlotType = {}) { return FOverride{ .Name = Property, .ValueOverride = Value, .SlotType = SlotType }; }
		static constexpr FOverride Type(sketch::FStringView Property, sketch::FStringView Type, sketch::FStringView SlotType = {}) { return FOverride{ .Name = Property, .TypeOverride = Type, .SlotType = SlotType }; }
		static constexpr FOverride Slot(sketch::FStringView SlotType, int Properties) { return FOverride{ .SlotType = SlotType, .SlotProperties = ESlotProperties(Properties) }; }
	};
	extern SKETCH_API TMap<FName, TArray<FOverride>> GOverrides;



	///
	/// Parsing
	///
	struct FFile
	{
		FString FilePath;
		FString Code;
		TArray<FClass> Classes;

		bool IsValid() const { return !Code.IsEmpty(); }
	};

	struct SKETCH_API FFileBuilder : FFile
	{
		FFileBuilder(FString&& FilePath, FLog& Log);

	private:
		bool Init();
		FProperty ProcessProperty(const FStringView& Properties, const SourceCode::FLocalStringView& PropertyBody, bool bDeprecated = false, bool bExplicitDefaultValue = false, const FStringView& PropertiesDefaults = {}, const FStringView& SlotType = {});
		FProperty ProcessAttribute(const FStringView& Properties, const SourceCode::FLocalStringView& PropertyBody, bool bDeprecated, bool bExplicitDefaultValue, const FStringView& PropertiesDefaults = {}, const sketch::FStringView SlotType = {});
		FSlot ProcessUniqueSlot(const FStringView& Properties, const SourceCode::FLocalStringView& PropertyBody);
		FSlot ProcessDynamicSlot(const FStringView& Properties, const SourceCode::FLocalStringView& PropertyBody, int ClassScopeIndex);

		Private::FIndex Index;
		FLog& Log;
	};

	SKETCH_API TArray<FFile> Scan(FLog& Log, const FString& Path, bool bRecursive);



	///
	/// Generation
	///
	struct SKETCH_API FReflectionGenerator
	{
		FReflectionGenerator(FStringView RegistrarName = TEXT("All"));
		void SetFile(const FFile& File, const FString& InclusionRoot) { FilePtr = &File, InclusionRootPtr = &InclusionRoot; }
		void Add(const FClass& Class);
		void Add(const FFile& File, const FString& InclusionRoot);
		FString GenerateReflection();

	private:
		const FFile* FilePtr = nullptr;
		const FString* InclusionRootPtr = nullptr;
		FString Prologue;
		FString Code;
		FString Epilogue;
	};

	SKETCH_API FString TryGetModuleName(FString& InclusionRoot, const TCHAR* FallbackValue = TEXT("All"));
}

#undef FORMATTER
