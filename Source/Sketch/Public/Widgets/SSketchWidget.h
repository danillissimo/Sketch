#pragma once
#include "SketchTypes.h"
#include "Widgets/SCompoundWidget.h"

class SKETCH_API SSketchWidget : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SSketchWidget, SCompoundWidget)
public:
	///
	/// Public interface
	///
	bool IsRoot() const { return bRoot; }
	SWidget& GetContent() const { return Overlay->GetChildren()->GetChildAt(0).Get(); }
	const TArray<TSharedPtr<sketch::FAttribute>>& GetAttributes() const { return Attributes; }
	const sketch::FFactoryHandle& GetContentFactory() const { return ContentFactory; }
	const auto& GetDynamicSlots() const { return Slots; }

	FSimpleMulticastDelegate OnModification;

	sketch::FFactory::FUniqueSlots CollectUniqueSlots() const { return CollectUniqueSlots(GetContent()); }
	struct FSlot;
	FSlot* FindSlotFor(SSketchWidget* Widget);
	/** @return First owning SSketchWidget, or this if is a root, nullptr on failure */
	SSketchWidget* GetParent() const;
	SSketchWidget* FindRoot() const;

	void AssignFactory(const FName& FactoryType, int FactoryIndex, bool bSuppressModificationEvent);
	void UnassignFactory(bool bSuppressModificationEvent);

	int AddDynamicSlot(const FName& Type, bool bSuppressModificationEvent);
	void AssignDynamicSlot(const FName& Type, int Index, const FName& FactoryType, int FactoryIndex, bool bSuppressModificationEvent);
	void ReleaseDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent);
	void RemoveDynamicSlot(const FName& Type, int Index, bool bSuppressModificationEvent);

	void Highlight() { bHighlighted = true; }
	void Unhighlight() { bHighlighted = false; }

	FString GenerateCode() const;

	void NotifyEditorDetached();



	///
	/// Slate
	///
	SLATE_BEGIN_ARGS(SSketchWidget) {}
		SLATE_ARGUMENT_DEFAULT(bool, bRoot) = false;
		SLATE_ARGUMENT_DEFAULT(bool, bAttachTarget) = true;

		SKETCH_API WidgetArgsType& SetupAsUniqueSlotContainer(const FName& SlotName);
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;



	///
	/// Internal logic
	///
	struct FSlot
	{
		FSlotBase* Slot = nullptr;
		TSharedPtr<SSketchWidget> Widget;
		/** @note Shared ptr so it doesn't move during outer container reallocations */
		TSharedPtr<TArray<TSharedPtr<sketch::FAttribute>>> Attributes;
	};

private:
	sketch::FFactory::FUniqueSlots CollectUniqueSlots(SWidget& Content) const;

	void BroadcastModification(bool bSuppress);
	void FinalizeOverlayRebuild();

	void UnassignFactory();
	void RebuildWidget();

	void OnSlotNonDynamicAttributeChanged(FName Type, int Index);

	void OnConstructSlot(FName Name);
	void OnDestroySlot(FName Type, int Index);

	void OnListFactoriesOfType(FMenuBuilder& SubMenu, FName Type);
	void OnFactorySelected(FName Type, int Index);

	///
	/// UI logic
	///
	FSlateColor GetBorderColor() const;

	///
	/// Data
	///	
	bool bRoot = false;
	sketch::FFactoryHandle ContentFactory;
	TArray<TSharedPtr<sketch::FAttribute>> Attributes;
	TMap<FName, TArray<FSlot, TInlineAllocator<1>>, TInlineSetAllocator<1>> Slots;

	TSharedPtr<SOverlay> Overlay;
	TSharedPtr<SBorder> Border;
	TSharedPtr<SWidget> AttachTargetHint;
	uint8 bHighlighted : 1 = false;
};
