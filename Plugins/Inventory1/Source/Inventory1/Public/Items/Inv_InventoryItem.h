// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/Inv_FragmentTags.h"
#include "Manifest/Inv_ItemManifest.h"
#include "StructUtils/InstancedStruct.h"

#include "Inv_InventoryItem.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY1_API UInv_InventoryItem : public UObject
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override {return true;};
	

	void SetItemManifest(const FInv_ItemManifest& Manifest);
	const FInv_ItemManifest& GetItemManifest() const { return ItemManifest.Get<FInv_ItemManifest>();}
	FInv_ItemManifest& GetItemManifestMutable() {return ItemManifest.GetMutable<FInv_ItemManifest>();}
	bool IsStackable() const;
	bool IsConsumable() const;
	int32 GetStackCount() const {return TotalStackCount; }
	void SetTotalStackCount(int32 Count) {TotalStackCount = Count;}
	

private:

	UPROPERTY(VisibleAnywhere,meta=(BaseStruct="/Script/Inventory1.Inv_ItemManifest"),Replicated)
	FInstancedStruct ItemManifest;

	UPROPERTY(Replicated)
	int32 TotalStackCount;
};

template <typename FragmentType>
const FragmentType* GetFragment(const UInv_InventoryItem* Item,const FGameplayTag& Tag)
{
	if (!IsValid(Item)) return nullptr;

	const FInv_ItemManifest& Manifest = Item->GetItemManifest();
	return Manifest.GetFragmentOfTypeWithTag<FragmentType>(Tag);
}


















