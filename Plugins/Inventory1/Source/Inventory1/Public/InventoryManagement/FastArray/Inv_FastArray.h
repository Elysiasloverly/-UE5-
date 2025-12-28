#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/NoExportTypes.h" 

#include "Inv_FastArray.generated.h"

class UInv_InventoryComponent;
class UInv_InventoryItem; 
class UInv_ItemComponent;

struct FGameplayTag;


/** 库存系统中的一个条目 */
USTRUCT(BlueprintType)
struct FInv_InventoryEntry:public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInv_InventoryEntry(){}

private:
	friend struct FInv_InventoryFastArray;
	friend UInv_InventoryComponent;

	UPROPERTY()
	TObjectPtr<UInv_InventoryItem> Item = nullptr;
};

/** 库存条目的列表 */
USTRUCT(BlueprintType)
struct FInv_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY()

	FInv_InventoryFastArray() : OwnerComponent(nullptr){}
	FInv_InventoryFastArray(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent){}

	TArray<UInv_InventoryItem*> GetAllItem() const;

	//来自于快速数组的功能函数
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices , int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices , int32 FinalSize);
	//来自于快速数组的功能函数自这里结束

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FInv_InventoryEntry,FInv_InventoryFastArray>(Entries,DeltaParams,*this);
	}

	UInv_InventoryItem* AddEntry(UInv_ItemComponent* ItemComponent);
	UInv_InventoryItem* AddEntry(UInv_InventoryItem* Item);
	void RemoveEntry(UInv_InventoryItem* Item);
	UInv_InventoryItem* FindFristItemByType(const FGameplayTag& ItemType);
	
private:
	friend struct FInv_InventoryFastArray;
	friend class UInv_InventoryComponent;
	
	//这将是被复制的项目列表
	UPROPERTY()
	TArray<FInv_InventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};


template<>
struct TStructOpsTypeTraits<FInv_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FInv_InventoryFastArray>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
 


