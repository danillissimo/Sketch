#include "AttributesTraites/EnumerationTraits.h"

#include "Types/SlateStructs.h"

#define LOCTEXT_NAMESPACE "Sketch.FEnumerationAttribute"

static sketch::FHeaderToolAttributeFilter GEnumerationFilter([](FStringView Attribute)
{
	return Attribute.StartsWith(TCHAR('E'));
});

int sketch::Private::FEnum::GetMemberByValue(uint64 Value) const
{
	for (int i = 0; i < NumMembers; ++i)
	{
		if (Members[i].Value == Value)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

TSharedRef<SWidget> sketch::FEnumerationAttribute::MakeEditor()
{
	SVerticalBox::FArguments MenuContent;
	MenuContent + SVerticalBox::Slot().AutoHeight()
	[
		SNew(STextBlock).Text(FText::FromString(FString(Enum.Name)))
	];
	for (int i = 0; i < Enum.NumMembers; ++i)
	{
		MenuContent + SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(FString(Enum.GetShortMemberName(i))))
		];
	}
	return SNew(SComboButton)
		.OnGetMenuContent(this, &FEnumerationAttribute::MakeMenu)
		.ButtonContent()
		[
			SNew(STextBlock).Text(this, &FEnumerationAttribute::GetSelectedMemberName)
		];
}

FString sketch::FEnumerationAttribute::GenerateCode() const
{
	const int Index = Enum.GetMemberByValue(Value);
	if (Index == INDEX_NONE)
	{
		FString Result(Enum.Name);
		Result += TEXT("(");
		Result += FString::FromInt(Value);
		Result += TEXT(")");
		return Result;
	}

	FString Result;
	// if (Enum.bClass)
	// {
	// 	Result = Enum.Name;
	// 	Result += TEXT("::");
	// }
	Result += Enum.GetFullMemberName(Index);
	return Result;
}

FText sketch::FEnumerationAttribute::GetSelectedMemberName() const
{
	const int Index = Enum.GetMemberByValue(Value);
	return Index != INDEX_NONE ? FText::FromString(FString(Enum.GetShortMemberName(Index))) : LOCTEXT("UnknownMember", "UNKNOWN MEMBER");
}

TSharedRef<SWidget> sketch::FEnumerationAttribute::MakeMenu()
{
	FMenuBuilder Menu(true, nullptr);
	for (int i = 0; i < Enum.NumMembers; ++i)
	{
		FMenuEntryParams Entry;
		Entry.LabelOverride = FText::FromString(FString(Enum.GetShortMemberName(i)));
		Entry.DirectActions.ExecuteAction.BindSP(this, &FEnumerationAttribute::OnMemberSelected, i);
		Menu.AddMenuEntry(Entry);
	}
	return Menu.MakeWidget();
}

void sketch::FEnumerationAttribute::OnMemberSelected(int Index)
{
	if (Enum.Members[Index].Value != Value)
	{
		Value = Enum.Members[Index].Value;
		OnValueChanged.Broadcast();
	}
}

sketch::FEnumerationAttribute sketch::TAttributeTraits<EVisibility>::ConstructValue(EVisibility Value)
{
	static const Private::FEnum Enum = []()-> Private::FEnum
	{
		Private::FEnum Result;
		Result.bClass = true;
		Result.Name = FStringView(TEXT("EVisibility"));
		Result.NumMembers = 5;

		static constexpr TCHAR Names[]{ TEXT("EVisibility::Visible\0EVisibility::Collapsed\0EVisibility::Hidden\0EVisibility::HitTestInvisible\0EVisibility::SelfHitTestInvisible\0") };
		Result.MembersNames = Names;

		auto Find = [](char Char, int Index) consteval -> int
		{
			for (int i = 0; i < UE_ARRAY_COUNT(Names); ++i)
			{
				if (Names[i] == Char)
				{
					--Index;
					if (Index < 0)
					{
						return i;
					}
				}
			}
			return -1;
		};
		constexpr TCHAR Terminator = TCHAR('\0');
		constexpr TCHAR Separator = TCHAR(':');
		static std::array<Private::FEnum::FMember, 5> Layout = {
			{
				{ .FullNameStart = 00000000000000000000000, .ShortNameStart = Find(Separator, 0) + 2, .NameEnd = Find(Terminator, 0), .Value = static_cast<uint64>(EVisibility::Visible.Value) },
				{ .FullNameStart = Find(Terminator, 0) + 1, .ShortNameStart = Find(Separator, 2) + 2, .NameEnd = Find(Terminator, 1), .Value = static_cast<uint64>(EVisibility::Collapsed.Value) },
				{ .FullNameStart = Find(Terminator, 1) + 1, .ShortNameStart = Find(Separator, 4) + 2, .NameEnd = Find(Terminator, 2), .Value = static_cast<uint64>(EVisibility::Hidden.Value) },
				{ .FullNameStart = Find(Terminator, 2) + 1, .ShortNameStart = Find(Separator, 6) + 2, .NameEnd = Find(Terminator, 3), .Value = static_cast<uint64>(EVisibility::HitTestInvisible.Value) },
				{ .FullNameStart = Find(Terminator, 3) + 1, .ShortNameStart = Find(Separator, 8) + 2, .NameEnd = Find(Terminator, 4), .Value = static_cast<uint64>(EVisibility::SelfHitTestInvisible.Value) },
			}
		};
		Result.Members = Layout.data();

		return Result;
	}();
	return FEnumerationAttribute(Value.Value, Enum);
}

EVisibility sketch::TAttributeTraits<EVisibility>::GetValue(const IAttributeImplementation& Attribute)
{
	EVisibility Result;
	Result.Value = static_cast<decltype(EVisibility::Value)::EnumType>(static_cast<const FEnumerationAttribute&>(Attribute).GetValue());
	return Result;
}
