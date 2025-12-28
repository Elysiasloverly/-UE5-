#pragma once


#include "Inv_GridTypes.generated.h"
class UInv_InventoryItem;

UENUM(BlueprintType)
enum class EInv_ItemCategory: uint8
{
	Equippable,
	Consumable,
	Craftable,
	None
};

USTRUCT()
struct FInv_SlotAvailability
{
	GENERATED_BODY()

	FInv_SlotAvailability(){}
	FInv_SlotAvailability(int32 ItemIndex,int32 Room,bool bHasItem):Index(ItemIndex),AmountToFill(Room),bItemAtIndex(bHasItem){}

	int32 Index{INDEX_NONE};
	int32 AmountToFill{0};
	bool bItemAtIndex{false};
};

USTRUCT()
struct FInv_SlotAvailabilityResult
{
	GENERATED_BODY()

	FInv_SlotAvailabilityResult(){}
	
	TWeakObjectPtr<UInv_InventoryItem>Item;
	int32 TotalRoomToFill{0};
	int32 Remainder{0};
	bool bStackable{false};
	TArray<FInv_SlotAvailability>SlotAvailabilities;
};

//用于表示光标吸附的坐标系
UENUM(BlueprintType)
enum class EInv_TileQuadrant: uint8
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	None
};

USTRUCT(BlueprintType)
struct FInv_TileParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category="Inventory")
	FIntPoint TileCordinats{};

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category="Inventory")
	int32 TileIndex{INDEX_NONE};

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category="Inventory")
	EInv_TileQuadrant TileQuadrant{EInv_TileQuadrant::None};
	
};

inline bool operator==(const FInv_TileParameters& A,const FInv_TileParameters& B)
{
	return A.TileCordinats == B.TileCordinats && A.TileIndex == B.TileIndex && A.TileQuadrant == B.TileQuadrant;
}

USTRUCT()
struct FInv_SpaceQueryResult
{
	GENERATED_BODY()

	//如果查询空间没有物品，就设为true表示有空间
	bool bHasSpace{false};

	//是否有有效物品在那,是否可以交换（选取与放置状态）
	TWeakObjectPtr<UInv_InventoryItem> ValidItem = nullptr;

	//这个物品的左上角索引是多少
	int32 UpperLeftIndex{INDEX_NONE};
};
