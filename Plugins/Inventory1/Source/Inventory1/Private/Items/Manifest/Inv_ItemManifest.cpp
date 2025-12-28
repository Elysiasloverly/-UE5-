#include "Items/Manifest/Inv_ItemManifest.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Composite/Inv_CompositeBase.h"


UInv_InventoryItem* FInv_ItemManifest::Manifest(UObject* NewOuter)
{
	UInv_InventoryItem* Item=NewObject<UInv_InventoryItem>(NewOuter,UInv_InventoryItem::StaticClass());
	Item->SetItemManifest(*this);
	for (auto& Fragment : Item->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().Manifest();
	}
	clearFragments();
	
	return Item;
}

void FInv_ItemManifest::AssimilateInventoryFragments(UInv_CompositeBase* Composite) const
{
	const auto& InventoryItemFragments = GetAllFragmentsOfType<FInv_InventoryItemFragment>();
	for (const auto* Fragment : InventoryItemFragments)
	{
		Composite->ApplyFunction([Fragment](UInv_CompositeBase* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}

void FInv_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation,
                                         const FRotator& SpawnRotation)
{
	if (!IsValid(PickupActorClass) || !IsValid(WorldContextObject)) return;
	
	AActor* SpawnedActor = WorldContextObject->GetWorld()->SpawnActor<AActor>(PickupActorClass,SpawnLocation,SpawnRotation);
	if (!IsValid(SpawnedActor)) return;
	
	//设置生成物品的属性清单,初始化Item属性清单(创建一个新物品清单，将旧有的物品清单复制到新的清单上)
	UInv_ItemComponent* ItemComp = SpawnedActor->FindComponentByClass<UInv_ItemComponent>();
	check(ItemComp);
	
	ItemComp->InitItemManifest(*this);
	
}

void FInv_ItemManifest::clearFragments()
{
	for (auto& Fragment: Fragments)
	{
		Fragment.Reset();
	}
	Fragments.Empty();
}
