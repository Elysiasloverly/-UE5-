// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/Inv_HUDWidget.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"

void UInv_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UInv_InventoryComponent* InventoryComponent=UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->NoRoomInInventory.AddUniqueDynamic(this,&UInv_HUDWidget::OnNoRoom);
	}
	
}

void UInv_HUDWidget::OnNoRoom()
{
	
	if (!IsValid(InfoMessage)) return;
	InfoMessage->SetMessage(FText::FromString("There is no space in the inventory."));
	
}
