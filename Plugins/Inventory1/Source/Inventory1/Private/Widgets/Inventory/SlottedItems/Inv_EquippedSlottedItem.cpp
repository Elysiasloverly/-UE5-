// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/SlottedItems/Inv_EquippedSlottedItem.h"

FReply UInv_EquippedSlottedItem::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 检查 Widget 是否启用
    UE_LOG(LogTemp, Warning, TEXT("Widget 是否启用: %s"), 
        GetIsEnabled() ? TEXT("是") : TEXT("否"));
    
    OnEquippedSlottedItemClicked.Broadcast(this);
    return FReply::Handled();
}