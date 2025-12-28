// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_SpatialInventory.h"

#include "../../../../../../Source/Inventory1/Public/Inventory1.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Widgets/ItemDescription/Inv_ItemDescription.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"


void UInv_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Equippables->OnClicked.AddDynamic(this,&ThisClass::ShowEquippables);
	Button_Consumables->OnClicked.AddDynamic(this,&ThisClass::ShowConsumables);
	Button_Craftables->OnClicked.AddDynamic(this,&ThisClass::ShowCraftables);
	
	Grid_Equippables->SetOwningCanvas(CanvasPanel);
	Grid_Consumables->SetOwningCanvas(CanvasPanel);
	Grid_Craftables->SetOwningCanvas(CanvasPanel);

	ShowEquippables();
	
	//遍历Widget树并筛选特定类型Widget
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UInv_EquippedGridSlot* EquippedGridSlot = Cast<UInv_EquippedGridSlot>(Widget);
		if (IsValid(EquippedGridSlot))
		{
			EquippedGridSlots.Add(EquippedGridSlot);
			EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this,&ThisClass::EquippedGridSlotClicked);
		}
	});
}

void UInv_SpatialInventory::EquippedGridSlotClicked(UInv_EquippedGridSlot* EquippedGridSlot,
	const FGameplayTag& EquipmentTypeTag)
{
	//检查我们是否可以装备这个悬停物品（Hover Item）。
	if (!CanEquipHoverItem(EquippedGridSlot,EquipmentTypeTag)) return;
	
	UInv_HoverItem* HoverItem = GetHoverItem();
	
	//创建一个“已装备”的槽位物品（Equipped slotted Item），并将其添加到已装备的网格槽（Equipped Grid Slot）中。
	const float TileSize = UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize();
	UInv_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
		HoverItem->GetInventoryItem(),
		EquipmentTypeTag,
		TileSize
	);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this,&ThisClass::EquippedSlottedItemClicked);
	
	//通知服务器我们已经装备了一个物品
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	
	InventoryComponent->Server_EquipSlotClicked(HoverItem->GetInventoryItem(),nullptr);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(HoverItem->GetInventoryItem());
	}
	//清除悬停物品（Hover Item）。
	Grid_Equippables->ClearHoverItem();
}

void UInv_SpatialInventory::EquippedSlottedItemClicked(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	
	// 移除物品描述
	UInv_InventoryStatics::ItemUnHovered(GetOwningPlayer());
	
	if (IsValid(GetHoverItem()) && GetHoverItem()->GetStackCount()) return;
	
	//获取装备的item
	UInv_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	
	//获取没装备的item
	UInv_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();
	
	// 获取持有此物品的已装备网格槽位（Equipped Grid Slot）
	UInv_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	
	// 清除该已装备网格槽位中的物品（将其库存物品设置为 nullptr）
	ClearSoltOfItem(EquippedGridSlot);
	
	//将之前装备的物品指定为悬停物品（Hover Item）
	Grid_Equippables->AssignHoverItem(ItemToUnequip);
	
	
	// 从已装备网格槽位中移除已装备的插槽物品（Equipped Slotted Item）
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	
	//将选取item创建一个新的装备库存插槽
	MakeEquippedSlottedItem(EquippedSlottedItem,EquippedGridSlot,ItemToEquip);
	
	//在装备或者卸下装备时候广播(来自于IC)
	BroadcastSlotClickedDelegates(ItemToEquip,ItemToUnequip);
	
}


FReply UInv_SpatialInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ActiveGrid->DropItem();
	return FReply::Handled(); 
}


void UInv_SpatialInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!IsValid(ItemDescription)) return;
	SetItemDescriptionSizeAndPosition(ItemDescription,CanvasPanel);
}


void UInv_SpatialInventory::SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description,
	UCanvasPanel* Canvas) 
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(ItemDescriptionCPS)) return;
	
	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);
	
	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampWidgetPosition(
		UInv_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	
	ItemDescriptionCPS->SetPosition(ClampedPosition);
}

bool UInv_SpatialInventory::CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot,
	const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;
	
	UInv_HoverItem* HoverItem = GetHoverItem();
	if (!IsValid(HoverItem)) return false;
	
	UInv_InventoryItem* HeldItem = HoverItem->GetInventoryItem();
	
	return HasHoverItem() && IsValid(HeldItem) 
			&& !HoverItem->IsStackable() 
				&& HeldItem->GetItemManifest().GetItemCategory() == EInv_ItemCategory::Equippable &&
					HeldItem->GetItemManifest().GetItemType().MatchesTag(EquipmentTypeTag);
}

UInv_EquippedGridSlot* UInv_SpatialInventory::FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const
{
	auto * FoundEquippedGridSlote = EquippedGridSlots.FindByPredicate([EquippedItem](UInv_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == EquippedItem;
	});
	
	return FoundEquippedGridSlote? *FoundEquippedGridSlote : nullptr;
}

void UInv_SpatialInventory::ClearSoltOfItem(UInv_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
	}
}

void UInv_SpatialInventory::RemoveEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	
	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this,&ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this,&ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UInv_SpatialInventory::MakeEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem,
	UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot)) return;	
	
	UInv_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		EquippedSlottedItem->GetEquipmentTypeTag(),
		UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize());

	if (IsValid(SlottedItem))
	{
		SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this,&ThisClass::EquippedSlottedItemClicked);
	}
	
	
	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UInv_SpatialInventory::BroadcastSlotClickedDelegates(UInv_InventoryItem* ItemToEquip,UInv_InventoryItem* ItemToUnequip) const
{
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	InventoryComponent->Server_EquipSlotClicked(ItemToEquip,ItemToUnequip);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(ItemToEquip);
		InventoryComponent->OnItemUnequipped.Broadcast(ItemToUnequip);
	}
}


FInv_SlotAvailabilityResult UInv_SpatialInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	switch (UInv_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{ 
		case EInv_ItemCategory::Equippable:
			return Grid_Equippables->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Consumable:
			return Grid_Consumables->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Craftable:
			return Grid_Craftables->HasRoomForItem(ItemComponent);
		default:
			UE_LOG(LogInventory, Warning, TEXT("物品组件无效"));
			return FInv_SlotAvailabilityResult();
	}	
	
	FInv_SlotAvailabilityResult Result;
	Result.TotalRoomToFill = 1;
	return Result;
}

void UInv_SpatialInventory::OnItemHovered(UInv_InventoryItem* Item)
{
	const auto& Manifest = Item->GetItemManifest();
	UInv_ItemDescription* DescriptionWidget = GetItemDescription();
	DescriptionWidget->SetVisibility(ESlateVisibility::Collapsed);
	
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
	
	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this,&Manifest,DescriptionWidget]()
	{
		Manifest.AssimilateInventoryFragments(DescriptionWidget);
		GetItemDescription()->SetVisibility(ESlateVisibility::HitTestInvisible);
	});
	
	GetOwningPlayer()->GetWorldTimerManager().SetTimer(DescriptionTimer,DescriptionTimerDelegate,DescriptionTimerDelay,false);
}

void UInv_SpatialInventory::OnItemUnHovered()
{
	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
}

bool UInv_SpatialInventory::HasHoverItem() const
{
	if (Grid_Equippables->HasHoverItem()) return true;
	if (Grid_Consumables->HasHoverItem()) return true;
	if (Grid_Craftables->HasHoverItem()) return true;
	return false;
	
}

UInv_HoverItem* UInv_SpatialInventory::GetHoverItem() const
{
	if (!ActiveGrid.IsValid())
	{
		return nullptr;
	}
	
	return ActiveGrid->GetHoverItem();
}

float UInv_SpatialInventory::GetTileSize() const
{
	
	return Grid_Equippables->GetTileSize();
}

void UInv_SpatialInventory::ShowEquippables()
{
	SetActiveGrid(Grid_Equippables,Button_Equippables);
}

void UInv_SpatialInventory::ShowConsumables()
{
	SetActiveGrid(Grid_Consumables,Button_Consumables);
}

void UInv_SpatialInventory::ShowCraftables()
{
	SetActiveGrid(Grid_Craftables,Button_Craftables);
}

void UInv_SpatialInventory::DisableButton(UButton* Button)
{
	Button_Equippables->SetIsEnabled(true);
	Button_Consumables->SetIsEnabled(true);
	Button_Craftables->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UInv_SpatialInventory::SetActiveGrid(UInv_InventoryGrid* Grid, UButton* Button)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->HideCursor();
		ActiveGrid->OnHide();
	}
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid())	ActiveGrid->ShowCursor();
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}

UInv_ItemDescription* UInv_SpatialInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(),ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}