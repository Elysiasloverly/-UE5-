// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Inv_InventoryComponent.h"

#include "Items/Components/Inv_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Items/Inv_InventoryItem.h"
#include "GameFramework/Pawn.h"
#include "Items/Fragments/Inv_ItemFragment.h"


UInv_InventoryComponent::UInv_InventoryComponent():InventoryList(this)
{

	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); 
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenu=false;

}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UInv_InventoryComponent, InventoryList);
}

bool UInv_InventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	// 调用父类逻辑，它会自动处理 bReplicateUsingRegisteredSubObjectList
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
}

void UInv_InventoryComponent::TryAddItem(UInv_ItemComponent* ItemComponent)
{
	
	
	FInv_SlotAvailabilityResult Result=InventoryMenu->HasRoomForItem(ItemComponent);

	UInv_InventoryItem* FoundItem=InventoryList.FindFristItemByType(ItemComponent->GetItemManifest().GetItemType());
	Result.Item = FoundItem;
	
	if (Result.TotalRoomToFill == 0)
	{
		NoRoomInInventory.Broadcast();
		return;
	}
	
	if (Result.Item.IsValid()&&Result.bStackable)
	{
		//向库存中添加已知物品堆叠，不是创建新插槽
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent,Result.TotalRoomToFill,Result.Remainder);
	}
	else if (Result.TotalRoomToFill>0)
	{
		//这个物品不存在，创建一个新物品插槽并更新背包插槽状态
		Server_AddNewItem(ItemComponent,Result.bStackable ? Result.TotalRoomToFill : 0);
	}
	
}


void UInv_InventoryComponent::Server_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount)
{
	
	UInv_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	NewItem->SetTotalStackCount(StackCount);
	
	OnItemAdded.Broadcast(NewItem);

	ItemComponent->PickedUp();
}

void UInv_InventoryComponent::Server_AddStacksToItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount,int32 Remainder)
{
	const FGameplayTag& ItemType = IsValid(ItemComponent) ? ItemComponent->GetItemManifest().GetItemType():FGameplayTag::EmptyTag;
	UInv_InventoryItem* Item=InventoryList.FindFristItemByType(ItemType);
	if(!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetStackCount()+StackCount);

	if (Remainder==0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifest().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
	 
}

void UInv_InventoryComponent::SpawnDroppedItem(UInv_InventoryItem* Item, int32 StackCount)
{
	const APawn* OwningPawn = OwningController->GetPawn();
	//在一个范围内随机生成凋落物
	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin,DropSpawnAngleMmx),FVector::UpVector);
	FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * FMath::FRandRange(DropSpawnDistanceMin,DropSpawnDistanceMax);
	SpawnLocation.Z -= RelativeSpawnElevation;
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	
	//将物品清单附加给掉落物.
	FInv_ItemManifest ItemManifest = Item->GetItemManifestMutable();
	if (FInv_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		//更新计数堆
		StackableFragment->SetStackCount(StackCount);
	}
	ItemManifest.SpawnPickupActor(this,SpawnLocation,SpawnRotation);
}

void UInv_InventoryComponent::Server_DropItem_Implementation_Implementation(UInv_InventoryItem* Item, int32 StackCount)
{
	const int32 NewStackCount = Item->GetStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}
	
	//生成掉落物品
	SpawnDroppedItem(Item,StackCount);
}

void UInv_InventoryComponent::Server_ConsumeItem_Implementation(UInv_InventoryItem* Item)
{
	const int32 NewStackCount = Item->GetStackCount() - 1;
	
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}
	
	
	//实际创建可消耗物品碎片
	if (FInv_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
	
}

void UInv_InventoryComponent::Server_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip)
{
	Multicast_EquipSlotClicked(ItemToEquip,ItemToUnequip);
}

void UInv_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip)
{
	//装备组件监听这些委托
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
}

void UInv_InventoryComponent::ToggleInventoryMenu()
{
	if (bInventoryMenu)
	{
		CloseInventoryMenu();
	}else
	{
		OpenInventoryMenu();
	}
	OnInventoryMenuToggled.Broadcast(bInventoryMenu);
}

void UInv_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	if (IsUsingRegisteredSubObjectList()&&IsReadyForReplication()&&IsValid(SubObj))
	{
		AddReplicatedSubObject(SubObj);	
	}
}

void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	ConstructInventory();
}

void UInv_InventoryComponent::ConstructInventory()
{
	OwningController=Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(),TEXT("物品组件应该以玩家控制器作为拥有者"));
	if (!OwningController->IsLocalController())return;
	
	InventoryMenu=CreateWidget<UInv_InventoryBase>(OwningController.Get(),InventoryMenuClass);
	InventoryMenu->AddToViewport();
	CloseInventoryMenu();
}

void UInv_InventoryComponent::OpenInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Visible);
	bInventoryMenu=true;

	if (!OwningController.IsValid())return;

	FInputModeGameAndUI InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Collapsed);
	bInventoryMenu=false;

	if (!OwningController.IsValid())return;

	FInputModeGameOnly InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(false);
}


