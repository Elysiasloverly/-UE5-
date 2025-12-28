#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Inv_InventoryItem.h"
#include "Runtime/Engine/Internal/VT/VirtualTextureVisualizationData.h"
#include "Types/Inv_GridTypes.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Inv_InventoryGrid.generated.h"

class UInv_ItemPopUp;
class UInv_HoverItem;
struct FInv_ImageFragment;
class UInv_SlottedItem;
struct FInv_GridFragment;
class UCanvasPanel;
class UInv_InventoryComponent;
struct FInv_ItemManifest;
class UInv_InventoryItem;
class UInv_ItemComponent;
struct FGameplayTag;
enum class EInv_GridSlotState : uint8;

/**
 * 
 */

UCLASS()
class INVENTORY1_API UInv_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	EInv_ItemCategory GetInv_ItemCategory() const { return ItemCategory; }
	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_ItemComponent* ItemComponent);
	void ShowCursor();
	void HideCursor();
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	void DropItem();
	bool HasHoverItem() const;
	UInv_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; };
	void ClearHoverItem();
	void AssignHoverItem(UInv_InventoryItem* InventoryItem);
	void OnHide();

	UFUNCTION()
	void AddItem(UInv_InventoryItem* Item);

private:

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	void ConstructGrid();
	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryItem* Item,const int32 StackAmountOverride = -1);
	FInv_SlotAvailabilityResult HasRoomForItem(const FInv_ItemManifest& Manifest,const int32 StackAmountOverride = -1);
	void AddItemToIndices(const FInv_SlotAvailabilityResult& Result,UInv_InventoryItem* NewItem);
	bool MatchesCateGory(const UInv_InventoryItem*Item) const;
	FVector2D GetDrawSize(const FInv_GridFragment* GridFragment)const;
	void SetSolottedItemImage(const UInv_SlottedItem* SlottedItem,const FInv_GridFragment* GridFragment,const FInv_ImageFragment* ImageFragment) const;
	void AddItemAtIndex(UInv_InventoryItem* Item,const int32 Index,const bool bStackable,const int32 StackAmount);
	UInv_SlottedItem* CreateSlottedItem(UInv_InventoryItem* Item,
		const bool bStackable,
		const int32 StackAmount,
		const FInv_GridFragment* GridFragment,
		const FInv_ImageFragment* ImageFragment,
		const int32 Index);
	void AddSlottedItemToCanvas(const int32 Index,const FInv_GridFragment* GridFragment,UInv_SlottedItem* SlottedItem) const;
	//更新item网格体笔刷
	void UpdateGridSlots(UInv_InventoryItem* NewItem,const int32 Index,bool bStackableItem,const int32 StackAmount);
	bool IsIndexClaimed(const TSet<int32>&CheckedIndndices,const int32 Index)const;
	bool HasRoomAtIndex(const UInv_GridSlot* GridSlot,
		const FIntPoint& Dimensions,
		TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize);
	bool CheckSlotConstraints(const UInv_GridSlot* GridSlot,
		const UInv_GridSlot* SubGridSlot,
		TSet<int32>& CheckedIndices,
		TSet<int32>& OutTentativelyClaimed,
		const FGameplayTag& ItemType,
		const int32 MaxStackSize) const;
	FIntPoint GetItemDimensions(const FInv_ItemManifest& Manifest) const;
	bool HasValidItem(const UInv_GridSlot* GridSlot) const;
	bool IsUpperLeftSlot(const UInv_GridSlot* GridSlot,const UInv_GridSlot* SubGridSlot) const;
	bool DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType) const;
	bool IsInGridBounds(const int32 StartIndex,const FIntPoint& ItemDimensions) const;
	int32 DeterminFillAmountForSlot(const bool bStackable,const int32 MaxStackSize,const int32 AmountToFill,const UInv_GridSlot* GridSlot);
	int32 GetStackAmount(const UInv_GridSlot* GridSlot) const;
	bool IsRightClick(const FPointerEvent& MouseEvent)const;
	bool IsLeftClick(const FPointerEvent& MouseEvent)const;
	void PickUp(UInv_InventoryItem* ClickedInventoryItem,const int32 GridIndex);
	void AssignHoverItem(UInv_InventoryItem* InventoryItem,const int32 GridIndex,const int32 PreviousGridIndex);
	void RemoveItemFromGrid(UInv_InventoryItem* InventoryItem,const int32 GridIndex);
	void UpdateTileParameters(const FVector2D& CanvasPosition,const FVector2D& MousePosition);
	FIntPoint CalculateHoveredCoordinates(const FVector2D& CanvasPosition,const FVector2D& MousePosition)const ;
	EInv_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPosition ,const FVector2D& MousePosition) const;
	void OnTileParaMetersUpdated(const FInv_TileParameters& Parameters);
	FIntPoint CalculateStartingCoordinate(const FIntPoint& Cordinate,const FIntPoint& Dimensions,const EInv_TileQuadrant Quadrant) const;
	FInv_SpaceQueryResult CheckHoverPosition(const FIntPoint& Cordinate,const FIntPoint& Dimensions) ;
	bool CursorExitedCanvas(const FVector2D& BoundaryPos,const FVector2D& BoundarySize, const FVector2D& Location) ;
	void HighlightSlots (const int32 Index,const FIntPoint& Dimensions);
	void UnHighlightSlots (const int32 Index,const FIntPoint& Dimensions);
	void ChangeHoverType(const int32 Index,const FIntPoint& Dimensions,EInv_GridSlotState GridSlotState);
	void PutDownOnIndex(const int32 Index);
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	bool IsSameStackable(const UInv_InventoryItem* ClickedInventoryItem)const;
	void SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem,const int32 GridIndex);
	bool ShouldSwapStackCounts(const int32 RoomInClickedSlot,const int32 HoveredStackCount,const int32 MaxStackSize) const;
	void SwapStackCounts(const int32 ClickedStackCount,const int32 HoveredStackCount,const int32 Index);
	bool ShouleConsumeHoverItemStacks(const int32 HoverStackCount,const int32 RoomInClickedSlot)const ;
	void ConsumeHoverItemStacks(const int32 ClickedStackCount,const int32 HoveredStackCount,const int32 Index);
	bool ShouleFillInStacks(const int32 RoomInClickedSlot,const int32 HoveredStackCount)const ;
	void FillInStacks(const int32 FillAmount,const int32 Remainder,const int32 Index);
	void CreateItemPopUp(const int32 GridIndex);
	void PutHoverItemBack();
	
	
	
	UPROPERTY(EditAnywhere,Category="Inventory")
	TSubclassOf<UInv_ItemPopUp> ItemPopUpClass;
	
	UPROPERTY()
	TObjectPtr<UInv_ItemPopUp> ItemPopUp;
	
	UPROPERTY(EditAnywhere,Category=" Inventory")
	TSubclassOf<UUserWidget> VisibleCursorWidgetClass;
	
	UPROPERTY(EditAnywhere,Category=" Inventory")
	TSubclassOf<UUserWidget> HiddenCursorWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> VisibleCursorWidget;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> HiddenCursorWidget;
	
	UPROPERTY(EditAnywhere,Category="Inventory")
	FVector2D ItemPopUpOffset;
	
	UFUNCTION()
	void AddStacks(const FInv_SlotAvailabilityResult& Result);
	
	UFUNCTION()
	void OnSlottedItemClicked(int32 GridIndex,const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotClicked(int32 GridIndex,const FPointerEvent& MouseEvent);

	//鼠标途径背包方格高亮
	UFUNCTION()
	void OnGridSlotHovered(int32 GridIndex,const FPointerEvent& MouseEvent);
	UFUNCTION()
	void OnGridSlotUnHovered(int32 GridIndex,const FPointerEvent& MouseEvent);
	
	UFUNCTION()
	void OnPopUpMenuSplit(int32 SplitAmount, int32 Index);
	
	UFUNCTION()
	void OnPopUpMenuDrop(int32 Index);
	
	UFUNCTION()
	void OnPopUpMenuConsume(int32 Index);
	
	UFUNCTION()
	void OnInventoryMenuToggled(bool bOpen);
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess="true"),Category="Inventory")
	EInv_ItemCategory ItemCategory;

	UPROPERTY()
	TArray<TObjectPtr<UInv_GridSlot>> GridSlots;

	UPROPERTY(EditAnywhere,Category="Inventory")
	TSubclassOf<UInv_GridSlot> GridSlotClass;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere,Category="Inventory")
	TSubclassOf<UInv_SlottedItem>SlottedItemClass;

	UPROPERTY()
	TMap<int32,TObjectPtr<UInv_SlottedItem>> SlottedItems;

	UPROPERTY(EditAnywhere,Category="Inventory")
	int32 Rows;

	UPROPERTY(EditAnywhere,Category="Inventory")
	int32 Columns;

	UPROPERTY(EditAnywhere,Category="Inventory")
	float TileSize;

	UPROPERTY(EditAnywhere,Category="Inventory")
	TSubclassOf<UInv_HoverItem> HoverItemClass;

	UPROPERTY()
	TObjectPtr<UInv_HoverItem> HoverItem;

	FInv_TileParameters TileParameters;
	FInv_TileParameters LastTileParameters;

	//物品应放置索引
	int32 ItemDropIndex{INDEX_NONE};
	FInv_SpaceQueryResult CurrentQueryResult;
	int32 LastHighlightedIndex = INDEX_NONE;
	FIntPoint LastHighlightedDimensions = FIntPoint(1,1);
	bool bMouseWithinCanvas = false;
	bool bLastMouseWithinCanvas = false;
	
};



