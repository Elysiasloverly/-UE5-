// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"

void UInv_EquippedGridSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsAvailable()) return;
	
	UInv_HoverItem* HoverItem = UInv_InventoryStatics::GetHoverItem(GetOwningPlayer());
	if (!IsValid(HoverItem)) return;
	
	if (HoverItem->GetItemType().MatchesTag(EquipmentTypeTag))
	{
		SetOccupiedTexture();
		Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInv_EquippedGridSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (!IsAvailable()) return;
	
	UInv_HoverItem* HoverItem = UInv_InventoryStatics::GetHoverItem(GetOwningPlayer());
	if (!IsValid(HoverItem)) return;
	
	if (HoverItem->GetItemType().MatchesTag(EquipmentTypeTag))
	{
		SetUnoccupiedTexture();
		Image_GrayedOutIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

FReply UInv_EquippedGridSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("=== UInv_EquippedGridSlot::NativeOnMouseButtonDown ==="));
    
	// 检查是否有已装备的物品
	if (EquippedSlottedItem != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("槽位已有物品，触发物品点击事件"));
        
		// 如果有物品，触发物品的点击事件而不是槽位的点击事件
		if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsBound())
		{
			EquippedSlottedItem->OnEquippedSlottedItemClicked.Broadcast(EquippedSlottedItem);
		}
        
		return FReply::Handled();
	}
	else
	{
		// 槽位为空，触发槽位点击事件（用于装备物品）
		UE_LOG(LogTemp, Warning, TEXT("槽位为空，触发槽位点击事件"));
		EquippedGridSlotClicked.Broadcast(this, EquipmentTypeTag);
		return FReply::Handled();
	}
}

UInv_EquippedSlottedItem* UInv_EquippedGridSlot::OnItemEquipped(UInv_InventoryItem* Item,
	const FGameplayTag& EquipmentTag, float TileSize)
{
	if (!EquipmentTag.MatchesTag(EquipmentTypeTag)) return nullptr;
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	if(!GridFragment) return nullptr;
	const FIntPoint GridDimensions = GridFragment->GetGridSize();
	
	// 计算物品绘制尺寸
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	const FVector2D DrawSize = GridDimensions * IconTileWidth;
	
	EquippedSlottedItem = CreateWidget<UInv_EquippedSlottedItem>(GetOwningPlayer(), EquippedSlottedItemClass);
	EquippedSlottedItem->SetInventoryItem(Item);
	EquippedSlottedItem->SetEquipmentTypeTag(EquipmentTag);
	EquippedSlottedItem->UpdateStackCount(0);
	SetInventoryItem(Item);
	
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::IconFragment);
	if (!ImageFragment) return nullptr;
	
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = DrawSize;
	
	EquippedSlottedItem->SetImageBrush(Brush);
	
	Overlay_Root->AddChildToOverlay(EquippedSlottedItem);
	


	FGeometry OverlayGeometry = Overlay_Root->GetCachedGeometry();
	FVector2D OverlaySize = OverlayGeometry.GetLocalSize();
    

    if (OverlaySize.IsZero())
    {
        OverlaySize = FVector2D(TileSize, TileSize); 
    }

	const float LeftPadding = (OverlaySize.X - DrawSize.X) / 2.f;
	const float TopPadding = (OverlaySize.Y - DrawSize.Y) / 2.f;

	UOverlaySlot* OverlaySlot = UWidgetLayoutLibrary::SlotAsOverlaySlot(EquippedSlottedItem);
	if (OverlaySlot)
	{
		OverlaySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		OverlaySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
		
		OverlaySlot->SetPadding(FMargin(LeftPadding, TopPadding, 0.f, 0.f));
	}
	
	return EquippedSlottedItem;
}