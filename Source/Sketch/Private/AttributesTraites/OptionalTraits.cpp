#include "AttributesTraites/OptionalTraits.h"

#include "Components/HorizontalBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "Sketch.TOptionalAttribute"

static sketch::FHeaderToolAttributeFilter GOptionalFilter([](FStringView Attribute)
{
	static bool bIsRunning = false;
	if (bIsRunning) return false;
	TGuardValue Guard(bIsRunning, true);

	if (!Attribute.StartsWith(TEXT("TOptional"))) return false;
	int FirstParenthesis, SecondParenthesis;
	if (!Attribute.FindChar(TCHAR('<'), FirstParenthesis) || !Attribute.FindChar(TCHAR('>'), SecondParenthesis) || FirstParenthesis >= SecondParenthesis)
		return false;
	FStringView OptionalType = Attribute.Mid(FirstParenthesis + 1, SecondParenthesis - FirstParenthesis - 1);
	OptionalType.TrimStartAndEndInline();
	const bool Result = sketch::IsSupportedAttributeType(OptionalType);
	return Result;
});

TSharedRef<SWidget> sketch::Private::FOptionalAttribute::MakeOptionalEditor(
	TSharedRef<SWidget>&& Content,
	TAttribute<ECheckBoxState>&& IsChecked,
	FOnCheckStateChanged&& OnStateChanged,
	TAttribute<bool>&& IsEnabled
)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked(MoveTemp(IsChecked))
			.OnCheckStateChanged(MoveTemp(OnStateChanged))
			.ToolTipText(LOCTEXT("OptionalToggleTooltip", "Toggle optional state"))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SBox)
			.IsEnabled(MoveTemp(IsEnabled))
			[
				MoveTemp(Content)
			]
		];
}

#undef LOCTEXT_NAMESPACE
