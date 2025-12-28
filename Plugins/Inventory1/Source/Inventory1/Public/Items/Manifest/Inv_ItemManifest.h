#pragma once

#include "CoreMinimal.h"
#include "Types/Inv_GridTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"

#include "Inv_ItemManifest.generated.h"

/**
 *  这个项目包含所有用于创建新的库存物品的必要信息
 */

class UInv_CompositeBase;
class UInv_InventoryItem;
struct FInv_ItemFragment;

USTRUCT(BlueprintType)
struct INVENTORY1_API FInv_ItemManifest
{
	GENERATED_BODY()

	TArray<TInstancedStruct<FInv_ItemFragment>>& GetFragmentsMutable()  {return Fragments;}
	UInv_InventoryItem* Manifest(UObject* NewOuter);
	EInv_ItemCategory GetItemCategory() const { return ItemCategory;}
	FGameplayTag GetItemType() const { return ItemType;}
	 void AssimilateInventoryFragments(UInv_CompositeBase* Composite)const;

	template <typename T>	requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;

	template <typename T>	requires std::derived_from<T, FInv_ItemFragment>
	const T* GetFragmentOfType() const;

	template <typename T>	requires std::derived_from<T, FInv_ItemFragment>
	T* GetFragmentOfTypeMutable() ;
	
	template <typename T>	requires std::derived_from<T, FInv_ItemFragment>
	TArray<const T*> GetAllFragmentsOfType() const;
	
	void SpawnPickupActor(const UObject* WorldContextObject,const FVector& SpawnLocation,const FRotator& SpawnRotation);
	

private:

	UPROPERTY(EditAnywhere,category="Inventory",meta=(ExcludeBaseStruct))
	TArray<TInstancedStruct<FInv_ItemFragment>> Fragments;
	
	UPROPERTY(EditAnywhere,Category="Inventory")
	EInv_ItemCategory ItemCategory{EInv_ItemCategory::None};

	UPROPERTY(EditAnywhere,Category="Inventory",meta = (Categories="GamepItems"))
	FGameplayTag ItemType;
	
	UPROPERTY(EditAnywhere,category="Inventory")
	TSubclassOf<AActor> PickupActorClass;
	
	void clearFragments();
	
};
template <typename T>
requires std::derived_from<T, FInv_ItemFragment>
const T* FInv_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T*FragmentPtr = Fragment.GetPtr<T>())
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTag(FragmentTag)) continue;
			return FragmentPtr;
		}
	}

	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
const T* FInv_ItemManifest::GetFragmentOfType() const
{
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T*FragmentPtr = Fragment.GetPtr<T>())
		{
			
			return FragmentPtr;
		}
	}

	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
T* FInv_ItemManifest::GetFragmentOfTypeMutable() 
{
	for (TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (T*FragmentPtr = Fragment.GetMutablePtr<T>())
		{
			
			return FragmentPtr;
		}
	}

	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_ItemFragment>
TArray<const T*> FInv_ItemManifest::GetAllFragmentsOfType() const
{
	TArray<const T*> Result;
	for (const TInstancedStruct<FInv_ItemFragment>& Fragment : Fragments)
	{
		if (const T*FragmentPtr = Fragment.GetPtr<T>())
		{
			
			Result.Add(FragmentPtr);
		}
	}
	return Result;
}




























