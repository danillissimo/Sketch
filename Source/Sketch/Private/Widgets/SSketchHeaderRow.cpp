#include "Widgets/SSketchHeaderRow.h"

#include <array>

#include "Textures/SlateIcon.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "SSketchHeaderRow"

void SSketchHeaderRow::Construct(const FArguments& InArgs)
{
	SHeaderRow::FArguments HeaderArgs;
	std::array<FColumnProperties, 7> ColumnsProperties = GetColumnsProperties();
	constexpr int NumColumns = ColumnsProperties.size();
	std::array<bool, NumColumns> ColumnVisibility = {
		InArgs._ShowLine,
		InArgs._ShowName,
		InArgs._ShowInteractivity,
		InArgs._ShowNumUsers,
		InArgs._AllowCodePatching,
		true,
		true
	};
	std::array<TSharedPtr<SWidget>, NumColumns> Widgets =
	{
		nullptr,
		nullptr,
		SNew(SBox)
		.ToolTipText(LOCTEXT("InteractivityTooltip", "Interactivity.\nInteractive attributes are read via TAttribute-s, and are expected to reflect changes immediately.\nNon-interactive attributes are likely to require additional actions to reflect changes."))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Profiler.EventGraph.ExpandHotPath16").GetIcon())
		],
		SNew(SBox)
		.ToolTipText(LOCTEXT("NumUsersTooltip", "Number of reading TAttributes"))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.MakeSet_16x").GetIcon())
		],
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Import").GetIcon())
		],
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Themes.Export").GetIcon())
		],
		nullptr
	};
	TArray<FName> HiddenColumns;
	HiddenColumns.Reserve(ColumnVisibility.size());
	for (int i = 0; i < NumColumns; ++i)
	{
		FColumn::FArguments Column =
			SHeaderRow::Column(ColumnsProperties[i].Id)
			.DefaultLabel(ColumnsProperties[i].Label);
		switch (ColumnsProperties[i].SizeMode)
		{
		case EColumnSizeMode::Fill:
			Column.FillWidth(ColumnsProperties[i].Size);
			break;
		case EColumnSizeMode::Fixed:
			Column.FixedWidth(ColumnsProperties[i].Size);
			break;
		case EColumnSizeMode::Manual:
			Column.ManualWidth(ColumnsProperties[i].Size);
			break;
		case EColumnSizeMode::FillSized:
			Column.FillSized(ColumnsProperties[i].Size);
			break;
		}
		if (!ColumnVisibility[i])
		{
			HiddenColumns.Add(ColumnsProperties[i].Id);
		}
		if (Widgets[i])
			Column.HeaderContent()[Widgets[i].ToSharedRef()];
		HeaderArgs + Column;
	}
	HeaderArgs.HiddenColumnsList(MoveTemp(HiddenColumns));
	SHeaderRow::Construct(HeaderArgs);
}

std::array<SSketchHeaderRow::FColumnProperties, 7> SSketchHeaderRow::GetColumnsProperties()
{
	return {
		{
			{ TEXT("Line"), 60.f, EColumnSizeMode::Fixed, LOCTEXT("LineColumnName", "Line") },
			{ TEXT("Name"), 0.3f, EColumnSizeMode::Fill, LOCTEXT("NameColumnName", "Name") },
			{ TEXT("Interactivity"), 24.f, EColumnSizeMode::Fixed, LOCTEXT("InteractivityColumnName", "Interactivity") },
			{ TEXT("NumUsers"), 30.f, EColumnSizeMode::Fixed, LOCTEXT("NumUsersColumnName", "NumUsers") },
			{ TEXT("PatchCode"), 30.f, EColumnSizeMode::Fixed, LOCTEXT("PatchCodeColumnName", "PatchCode") },
			{ TEXT("CopyCode"), 30.f, EColumnSizeMode::Fixed, LOCTEXT("CopyCodeColumnName", "CopyCode") },
			{ TEXT("Data"), 0.7f, EColumnSizeMode::Fill, LOCTEXT("DataColumnName", "Data") },
		}
	};
}

#undef LOCTEXT_NAMESPACE
