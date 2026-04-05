#pragma once
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/IItemsSource.h"
#include "Widgets/Views/SListView.h"

template <class T>
class SIdList : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SWidget>, FOnGenerateRow, const T&, TAttribute<FText>&& TextToHighlight);
	DECLARE_DELEGATE_OneParam(FOnIdSelected, const T&);

	SLATE_BEGIN_ARGS(SIdList)
		{
		}

		SLATE_ITEMS_SOURCE_ARGUMENT(T, Items)
		SLATE_EVENT(FOnGenerateRow, OnGenerateRow)
		SLATE_EVENT(FOnIdSelected, OnIdSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Items = Args.GetItems().ArrayPointer;
		OnGenerateRow = Args._OnGenerateRow;
		OnIdSelected = Args._OnIdSelected;

		ChildSlot
		[
			SNew(SBox)
			.MinDesiredWidth(700.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.f, 4.f)
				[
					SAssignNew(SearchBox, SSearchBox)
					.OnTextChanged(this, &SIdList::OnFilterChanged)
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.)
				[
					SAssignNew(List, SListView<T>)
					.ListItemsSource(Args.GetItems())
					.OnGenerateRow(this, &SIdList::GenerateRow)
					.OnSelectionChanged(this, &SIdList::OnSelectionChanged)
				]
			]
		];
	}

	void SetUserFocus()
	{
		RegisterActiveTimer(0., FWidgetActiveTimerDelegate::CreateSPLambda(this, [this](double, float)
		{
			FSlateApplication::Get().SetAllUserFocus(SearchBox, EFocusCause::WindowActivate);
			return EActiveTimerReturnType::Stop;
		}));
	}

private:
	void OnFilterChanged(const FText& FilterText)
	{
		if (FilterText.IsEmpty())
		{
			List->SetItemsSource(Items);
		}
		else
		{
			FilteredItems.Reset();
			for (const T& Item : *Items)
			{
				if (Item.ContainsText(FilterText.ToString()))
				{
					FilteredItems.Add(Item);
				}
			}
			List->SetItemsSource(&FilteredItems);
			List->RequestListRefresh();
		}
	}

	TSharedRef<ITableRow> GenerateRow(T Item, const TSharedRef<STableViewBase>& Owner)
	{
		return SNew(STableRow<T>, Owner)
			.Padding(FMargin{ 8.f, 4.f })
			[
				OnGenerateRow.Execute(Item, TAttribute<FText>::CreateSP(SearchBox.Get(), &SSearchBox::GetText))
			];
	}

	void OnSelectionChanged(T Item, ESelectInfo::Type SelectionType)
	{
		if (SelectionType == ESelectInfo::OnNavigation)
			return;

		OnIdSelected.Execute(Item);
		FSlateApplication::Get().DismissAllMenus();
	}

	const TArray<T>* Items = nullptr;
	TArray<T> FilteredItems;

	TSharedPtr<SListView<T>> List;
	TSharedPtr<SSearchBox> SearchBox;

	FOnGenerateRow OnGenerateRow;
	FOnIdSelected OnIdSelected;
};
