// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"

#include "InteractiveToolManager.h"
#include "../../../../../../../../Source/Inventory/Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/ItemPopUp/Inv_ItemPopUp.h"

void UInv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConstructGrid();

	//获取库存组件
	InventoryComponent=UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	InventoryComponent->OnItemAdded.AddDynamic(this,&ThisClass::AddItem);
	InventoryComponent->OnStackChange.AddDynamic(this,&ThisClass::AddStacks);
	InventoryComponent->OnInventoryMenuToggled.AddDynamic(this,&ThisClass::OnInventoryMenuToggled);
}

void UInv_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//记录鼠标位置
	const FVector2D CanvasPosition = UInv_WidgetUtils::GetWidgetLocation(CanvasPanel);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	//检查光标是否已经退出背包面板
	if (CursorExitedCanvas(CanvasPosition,UInv_WidgetUtils::GetWidgetSize(CanvasPanel),MousePosition))
	{
		return;
	}
	
	
	UpdateTileParameters(CanvasPosition,MousePosition);
}

void UInv_InventoryGrid::UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!bMouseWithinCanvas) return;
	
	//如果鼠标不在画布就直接return
	//计算高亮方块
	const FIntPoint HoveredTileCoordinates = CalculateHoveredCoordinates(CanvasPosition,MousePosition);

	LastTileParameters = TileParameters;
	TileParameters.TileCordinats = HoveredTileCoordinates;
	TileParameters.TileIndex = UInv_WidgetUtils::GetIndexFromPosition(HoveredTileCoordinates,Columns);
	TileParameters.TileQuadrant = CalculateTileQuadrant(CanvasPosition,MousePosition);
	
	//移走就取消高亮
	OnTileParaMetersUpdated(TileParameters);
}

void UInv_InventoryGrid::OnTileParaMetersUpdated(const FInv_TileParameters& Parameters)
{
	//检查悬停item
	if (!IsValid(HoverItem))
	{
		return;
	}

	//获取悬停item尺寸
	const FIntPoint Dimensions = HoverItem->GetGridDimensions();

	//计算预选起始坐标
	const FIntPoint StartingCoordinate = CalculateStartingCoordinate(Parameters.TileCordinats,Dimensions,Parameters.TileQuadrant);
	ItemDropIndex = UInv_WidgetUtils::GetIndexFromPosition(StartingCoordinate,Columns);
	
	//检查停靠位置
	CurrentQueryResult = CheckHoverPosition(StartingCoordinate,Dimensions);
	
	if (CurrentQueryResult.bHasSpace)
	{
		HighlightSlots(ItemDropIndex,Dimensions);
		return;
	}
	UnHighlightSlots(LastHighlightedIndex,LastHighlightedDimensions);

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		//空间有一个单独的项，就交换
		const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(CurrentQueryResult.ValidItem.Get(),FragmentTags::GridFragment);
		if (!GridFragment) return;

		ChangeHoverType(CurrentQueryResult.UpperLeftIndex,GridFragment->GetGridSize(),EInv_GridSlotState::GrayedOut);
		
	}
		
}

FInv_SpaceQueryResult UInv_InventoryGrid::CheckHoverPosition(const FIntPoint& Position,
	const FIntPoint& Dimensions) 
{
	FInv_SpaceQueryResult Result;

	Result.bHasSpace = true;
	//是否在背包内
	if (!IsInGridBounds(UInv_WidgetUtils::GetIndexFromPosition(Position,Columns),Dimensions)) 
	{
		Result.bHasSpace = false; 
		return Result;
	}

	//如果多个索引被同一项占用，检查他们是否有相同的左上角索引(判断是否为同一个物品)
	TSet<int32> OccupiedUpperLeftIndices;
	UInv_InventoryStatics::ForEach2D(GridSlots,UInv_WidgetUtils::GetIndexFromPosition(Position,Columns),Dimensions,Columns,[&](const UInv_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});
	
	//如果有，检查是否可以交换(交换条件为覆盖框仅有唯一物品)（选取/放置）
	if (OccupiedUpperLeftIndices.Num()==1)//单个物品可交换
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}
	
	return Result;
}

bool UInv_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize,
	const FVector2D& Location) 
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = UInv_WidgetUtils::IsWitchinBounds(BoundaryPos,BoundarySize,Location);
	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		//如果离开画布就取消高亮插槽
		UnHighlightSlots(LastHighlightedIndex,LastHighlightedDimensions);
		return true;
	}
	return false;
}

void UInv_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	if (!bMouseWithinCanvas)
	{
		UE_LOG(LogTemp, Error, TEXT("Debug: HighlightSlots blocked because bMouseWithinCanvas is FALSE"));
		return;
	}
	UnHighlightSlots(LastHighlightedIndex,LastHighlightedDimensions);
	UInv_InventoryStatics::ForEach2D(GridSlots,Index,Dimensions,Columns,[&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
	
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
	
}

void UInv_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
		UInv_InventoryStatics::ForEach2D(GridSlots,Index,Dimensions,Columns,[&](UInv_GridSlot* GridSlot)
	{
		if (GridSlot->IsAvailable())
		{
			GridSlot->SetUnoccupiedTexture();
		}
		else
		{
			GridSlot->SetOccupiedTexture();
		}
	});
}

void UInv_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions,
	EInv_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHighlightedIndex,LastHighlightedDimensions);
	UInv_InventoryStatics::ForEach2D(GridSlots,Index,Dimensions,Columns,[State = GridSlotState](UInv_GridSlot* GridSlot)
	{
		switch (State)
		{
			case EInv_GridSlotState::Occupied:
				GridSlot->SetOccupiedTexture();
				break;
			case EInv_GridSlotState::Unoccupied:
				GridSlot->SetUnoccupiedTexture();
				break;
			case EInv_GridSlotState::GrayedOut:
				GridSlot->SetGrayedOutTexture();
				break;
			case EInv_GridSlotState::Selected:
				GridSlot->SetSelectedTexture();
				break;
		}
	});

	LastHighlightedIndex = Index;
	LastHighlightedDimensions = Dimensions;
}

FIntPoint UInv_InventoryGrid::CalculateStartingCoordinate(const FIntPoint& Cordinate, const FIntPoint& Dimensions,
                                                          const EInv_TileQuadrant Quadrant) const
{
	const int32 HasEventWidth = Dimensions.X % 2==0 ? 1 : 0;
	const int32 HasEventHeight = Dimensions.Y % 2==0 ? 1 : 0;

	FIntPoint StartingCoord ;
	switch (Quadrant)
	{
	case EInv_TileQuadrant::TopLeft:
			StartingCoord.X = Cordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Cordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
		break;
	case EInv_TileQuadrant::TopRight:
			StartingCoord.X = Cordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEventWidth;
			StartingCoord.Y = Cordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
		break;
	case EInv_TileQuadrant::BottomLeft:
			StartingCoord.X = Cordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Cordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEventWidth;
		break;
	case EInv_TileQuadrant::BottomRight:
			StartingCoord.X = Cordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEventWidth;
			StartingCoord.Y = Cordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEventWidth;
		break;
	default:
		UE_LOG(LogInventory,Error,TEXT("	不是有效的象限	"))
		return FIntPoint(-1,-1);
	}
	return StartingCoord;
}



FIntPoint UInv_InventoryGrid::CalculateHoveredCoordinates(const FVector2D& CanvasPosition,
                                                          const FVector2D& MousePosition) const
{
	return FIntPoint {
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X)/TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y)/TileSize)),
		
	};
}

EInv_TileQuadrant UInv_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPosition,
	const FVector2D& MousePosition) const
{
	//计算瓦片相对位置
	const float TileLocalX = FMath::Fmod(MousePosition.X - CanvasPosition.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePosition.Y - CanvasPosition.Y, TileSize);

	//确定位于哪个象限
	const bool bIsTop = TileLocalY < TileSize/2.f;
	const bool bIsLeft = TileLocalX <TileSize/2.f;

	EInv_TileQuadrant HoveredTileQuadrant{EInv_TileQuadrant::None};
	if (bIsTop&&bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopLeft;
	else if (bIsTop &&!bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopRight;
	else if (!bIsTop&&bIsLeft) HoveredTileQuadrant  = EInv_TileQuadrant::BottomLeft;
	else if (!bIsTop &&!bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::BottomRight;

	return HoveredTileQuadrant;
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_InventoryItem* Item,const int32 StackAmountOverride)
{
	return HasRoomForItem(Item->GetItemManifest(),StackAmountOverride);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const FInv_ItemManifest& Manifest,const int32 StackAmountOverride )
{
	FInv_SlotAvailabilityResult Result;

	//判断Item是否可以叠加
	const FInv_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable=StackableFragment != nullptr;
	
	//要添加多少在这个堆上
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1 ;
	if (StackAmountOverride != -1 && Result.bStackable)
	{
		AmountToFill = StackAmountOverride;
	}

	TSet<int32> CheckedIndices;
	//遍历每个插槽
	for (const auto& GridSlot: GridSlots)
	{
		// 若库存已满，提前跳出循环。
		if (AmountToFill==0) break;
		
		// 该索引是否已被占用？
		if (IsIndexClaimed(CheckedIndices,GridSlot->GetIndex()))continue;

		//是否在网格体内
		if (!IsInGridBounds(GridSlot->GetIndex(),GetItemDimensions(Manifest)))continue;
		
		// 该物品能否放入此处？是否超出网格边界？
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot,GetItemDimensions(Manifest),CheckedIndices,TentativelyClaimed,Manifest.GetItemType(),MaxStackSize))
		{
			continue;
		}

		CheckedIndices.Append(TentativelyClaimed);
		
		// 需要填充多少数量？
		const int32 AmountToFillInSlot=DeterminFillAmountForSlot(Result.bStackable,MaxStackSize,AmountToFill,GridSlot);
		if (AmountToFillInSlot == 0) continue;
		
		// 更新剩余待填充的数量
		Result.TotalRoomToFill+=AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FInv_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				GridSlot->GetInventoryItem().IsValid()
			}
		);
		
		AmountToFill-=AmountToFillInSlot;
		
		Result.Remainder= AmountToFill;
		
		if (AmountToFill==0) return Result;
	}
	
	return Result;	
}

bool UInv_InventoryGrid::HasRoomAtIndex(const UInv_GridSlot* GridSlot,
										const FIntPoint& Dimensions,
										TSet<int32>& CheckedIndices,
										TSet<int32>& OutTentativelyClaimed,
										const FGameplayTag& ItemType,
										const int32 MaxStackSize)
{
	// 该索引位置是否有可用空间？是否有其他物品阻挡？
	bool bHasRoomAtIndex=true;
	
	UInv_InventoryStatics::ForEach2D(GridSlots,GridSlot->GetIndex(),Dimensions,Columns,[&](const UInv_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(GridSlot,SubGridSlot,CheckedIndices,OutTentativelyClaimed,ItemType,MaxStackSize))
		{
			OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		}
		else
		{
			bHasRoomAtIndex=false;
		}
	});
	
	return bHasRoomAtIndex;
}

bool UInv_InventoryGrid::CheckSlotConstraints(	const UInv_GridSlot* GridSlot,
												const UInv_GridSlot* SubGridSlot,
												TSet<int32>& CheckedIndices,
												TSet<int32>& OutTentativelyClaimed,
												const FGameplayTag& ItemType,
												const int32 MaxStackSize) const
{
			// 索引已被占用？
	if (IsIndexClaimed(CheckedIndices,SubGridSlot->GetIndex())) return false;
	
			// 存在有效物品？
	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		return true;
	}

			//是否为左上角的插槽
	if (!IsUpperLeftSlot(GridSlot,SubGridSlot)) return false;
	
			// 该物品是否与尝试添加的物品类型一致？
	const UInv_InventoryItem* SubItem =SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;
	
			// 若类型一致，该物品是否为可堆叠物品？
	if (!DoesItemTypeMatch(SubItem,ItemType)) return false;
	
			// 若为可堆叠物品，该槽位是否已达最大堆叠数量？
	if (GridSlot->GetStackCount()>=MaxStackSize) return false;	
	
	return true;
}

FIntPoint UInv_InventoryGrid::GetItemDimensions(const FInv_ItemManifest& Manifest) const
{
	const FInv_GridFragment* GridFragment=Manifest.GetFragmentOfType<FInv_GridFragment>();
	return   GridFragment ? GridFragment->GetGridSize():FIntPoint(1,1);
}

bool UInv_InventoryGrid::HasValidItem(const UInv_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UInv_InventoryGrid::IsUpperLeftSlot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UInv_InventoryGrid::DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UInv_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex<0 || StartIndex >= GridSlots.Num()) return false;
	const int32 EndColumn= (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;
	return EndColumn<=Columns && EndRow<=Rows;
}

int32 UInv_InventoryGrid::DeterminFillAmountForSlot(const bool bStackable, const int32 MaxStackSize,
	const int32 AmountToFill, const UInv_GridSlot* GridSlot)
{
	const int32 RoomInSlot= MaxStackSize-GetStackAmount(GridSlot);
	return bStackable ? FMath::Min(AmountToFill,RoomInSlot): 1;
}

int32 UInv_InventoryGrid::GetStackAmount(const UInv_GridSlot* GridSlot) const
{
	int32 CurrentslotStackCount = 0;

	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex();UpperLeftIndex!=INDEX_NONE)
	{
		UInv_GridSlot* UpperLeftGridSolot=GridSlots[UpperLeftIndex];
		CurrentslotStackCount=UpperLeftGridSolot->GetStackCount();
	}
	return CurrentslotStackCount;
}

bool UInv_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UInv_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent )const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

void UInv_InventoryGrid::PickUp(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	//选中物品跟随鼠标
	AssignHoverItem(ClickedInventoryItem,GridIndex,GridIndex);
	
	//移除选取的物品在背包中
	RemoveItemFromGrid(ClickedInventoryItem,GridIndex);
	
}

void UInv_InventoryGrid::AssignHoverItem(UInv_InventoryItem* InventoryItem, const int32 GridIndex,
	const int32 PreviousGridIndex)
{
	AssignHoverItem(InventoryItem);

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);
	HoverItem->UpdateStackCount(InventoryItem->IsStackable() ? GridSlots[GridIndex]->GetStackCount() : 0);
}

void UInv_InventoryGrid::RemoveItemFromGrid(UInv_InventoryItem* InventoryItem, const int32 GridIndex)
{
	const FInv_GridFragment* GridFragment=GetFragment<FInv_GridFragment>(InventoryItem,FragmentTags::GridFragment);
	if (!GridFragment) return;

	UInv_InventoryStatics::ForEach2D(GridSlots,GridIndex,GridFragment->GetGridSize(),Columns,[&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(nullptr);
		GridSlot->SetUpperLeftIndex(INDEX_NONE);
		GridSlot->SetUnoccupiedTexture();
		GridSlot->SetAvailable(true);
		GridSlot->SetStackCount(0);
	});
		
	if (SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UInv_SlottedItem> FoundSlottedItem;
		SlottedItems.RemoveAndCopyValue(GridIndex,FoundSlottedItem);
		FoundSlottedItem->RemoveFromParent();
	}
}

void UInv_InventoryGrid::AssignHoverItem(UInv_InventoryItem* InventoryItem)
{
	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UInv_HoverItem>(GetOwningPlayer(),HoverItemClass);
	}

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem,FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFraagment = GetFragment<FInv_ImageFragment>(InventoryItem,FragmentTags::IconFragment);
	if (!GridFragment || !ImageFraagment) return;

	const FVector2D DrawSize=GetDrawSize(GridFragment);

	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFraagment->GetIcon());
	IconBrush.DrawAs=ESlateBrushDrawType::Image;
	IconBrush.ImageSize=DrawSize * UWidgetLayoutLibrary::GetViewportScale(this);

	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetGridDimensions(GridFragment->GetGridSize());
	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());

	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default,HoverItem);
}

void UInv_InventoryGrid::OnHide()
{
	PutHoverItemBack();
}

void UInv_InventoryGrid::AddStacks(const FInv_SlotAvailabilityResult& Result)
{
	if (!MatchesCateGory(Result.Item.Get())) return;

	for (const auto& Availablity:Result.SlotAvailabilities)
	{
		if (Availablity.bItemAtIndex)
		{
			const auto& GridSlot=GridSlots[Availablity.Index];
			const auto& SlottedItem=SlottedItems.FindChecked(Availablity.Index);
			SlottedItem->UpdateStackCount(GridSlot->GetStackCount()+Availablity.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount()+Availablity.AmountToFill);
		}
		else
		{
			AddItemAtIndex(Result.Item.Get(),Availablity.Index,Result.bStackable,Availablity.AmountToFill);
			UpdateGridSlots(Result.Item.Get(),Availablity.Index,Result.bStackable,Result.TotalRoomToFill);
			
		}
	}
}

void UInv_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_InventoryStatics::ItemUnHovered(GetOwningPlayer());
	//确保小组件不会超出边界
	
	check(GridSlots.IsValidIndex(GridIndex));
	UInv_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		PickUp(ClickedInventoryItem,GridIndex);
		return;
	}

	if (IsRightClick(MouseEvent))
	{
		CreateItemPopUp(GridIndex);
		return;
	}
	
	//处理同一个类型
	if (IsSameStackable(ClickedInventoryItem))
	{
		//获取数量
		const int32 ClickedStackCount=GridSlots[GridIndex]->GetStackCount();
		const FInv_StackableFragment* StackableFragment=ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();
		const int32 MaxStackSize =StackableFragment->GetMaxStackSize();
		const int32 RoomInClickedSlot = MaxStackSize - ClickedStackCount;
		const int32 HoveredStackCount = HoverItem->GetStackCount();
		
		//我们是否要交换他们的叠加数量（库存和选取两者有且仅有一个达到最大堆叠数量，两者数量相等不交换）
		if (ShouldSwapStackCounts(RoomInClickedSlot,HoveredStackCount,MaxStackSize))
		{
			SwapStackCounts(ClickedStackCount,HoveredStackCount,GridIndex);
			return;
		}
		
		//消耗选取item的数量去补充库存里的数量
		if (ShouleConsumeHoverItemStacks(HoveredStackCount,RoomInClickedSlot))
		{
			ConsumeHoverItemStacks(ClickedStackCount,HoveredStackCount,GridIndex);
			return;
		}
		
		//我们应该填充库存物品的叠加数量吗（不是消耗悬停物品的数量）
		if (ShouleFillInStacks(RoomInClickedSlot, HoveredStackCount))
		{
			FillInStacks(RoomInClickedSlot, HoveredStackCount - RoomInClickedSlot, GridIndex);
			return;
		}
		
		//槽位满了，不做任何操作
		if (RoomInClickedSlot == 0)
		{
			return;
		}
		
	}

	if (CurrentQueryResult.ValidItem.IsValid())
	{
		SwapWithHoverItem(ClickedInventoryItem,GridIndex);
	}

}

void UInv_InventoryGrid::CreateItemPopUp(const int32 GridIndex)
{
	UInv_InventoryItem * RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;
	
	ItemPopUp = CreateWidget<UInv_ItemPopUp>(this,ItemPopUpClass);
	GridSlots[GridIndex]->SetItemPopUp(ItemPopUp);
	
	OwningCanvasPanel->AddChild(ItemPopUp);
	UCanvasPanelSlot * CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	CanvasSlot->SetPosition(MousePosition - ItemPopUpOffset);
	CanvasSlot->SetSize(ItemPopUp->GetBoxSize());
	
	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1; 
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->OnSplit.BindDynamic(this,& ThisClass::OnPopUpMenuSplit);
		ItemPopUp->SetSliderParams(SliderMax,FMath::Max(1,GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}
	
	//下拉菜单函数回调
	ItemPopUp->OnDrop.BindDynamic(this,& ThisClass::OnPopUpMenuDrop);
	
	//判断是否为消费品
	if (RightClickedItem->IsConsumable())
	{
		ItemPopUp->OnConsume.BindDynamic(this,& ThisClass::OnPopUpMenuConsume);
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
	
 }

void UInv_InventoryGrid::PutHoverItemBack()
{
	if (!IsValid(HoverItem)) return;
	
	FInv_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(),HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();
	
	AddStacks(Result);
	ClearHoverItem();
}

//从库存中移除item
void UInv_InventoryGrid::DropItem()
{
	if (!IsValid(HoverItem)) return;
	if (!IsValid(HoverItem->GetInventoryItem())) return;
	
	//告诉服务器移除item，在服务端
	InventoryComponent->Server_DropItem_Implementation(HoverItem->GetInventoryItem(),HoverItem->GetStackCount());
	
	ClearHoverItem();
	ShowCursor();
}

bool UInv_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

UInv_HoverItem* UInv_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}

void UInv_InventoryGrid::AddItem(UInv_InventoryItem* Item)
{

	if (Item == nullptr)
	{
		return;
	}

	
	if (!MatchesCateGory(Item)) return;
	


	FInv_SlotAvailabilityResult Result=HasRoomForItem(Item);
	AddItemToIndices(Result,Item);
}

void UInv_InventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* NewItem)
{

	for (const auto& Availability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(NewItem,Availability.Index,Result.bStackable,Availability.AmountToFill);
		UpdateGridSlots(NewItem,Availability.Index,Result.bStackable,Availability.AmountToFill);
	}

}

void UInv_InventoryGrid::AddItemAtIndex(UInv_InventoryItem* Item, const int32 Index, const bool bStackable,
	const int32 StackAmount)
{
	
	const FInv_GridFragment* GridFragment=GetFragment<FInv_GridFragment>(Item,FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment=GetFragment<FInv_ImageFragment>(Item,FragmentTags::IconFragment);
	if (!GridFragment||!ImageFragment) return;

	UInv_SlottedItem* SlottedItem=CreateSlottedItem(Item,bStackable,StackAmount,GridFragment,ImageFragment,Index);
	AddSlottedItemToCanvas(Index,GridFragment,SlottedItem);

	SlottedItems.Add(Index,SlottedItem);
}

UInv_SlottedItem* UInv_InventoryGrid::CreateSlottedItem(UInv_InventoryItem* Item, const bool bStackable,
	const int32 StackAmount, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment,
	const int32 Index)
{
	
	UInv_SlottedItem* SlottedItem=CreateWidget<UInv_SlottedItem>(GetOwningPlayer(),SlottedItemClass);
	SlottedItem->SetInventoryItem(Item);
	SetSolottedItemImage(SlottedItem,GridFragment,ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount= bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	SlottedItem->OnSlottedItemClicked.AddDynamic(this,&ThisClass::OnSlottedItemClicked);

	return SlottedItem;
}

void UInv_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FInv_GridFragment* GridFragment,
	UInv_SlottedItem* SlottedItem) const
{
	CanvasPanel->AddChild(SlottedItem);
	//获取大小
	UCanvasPanelSlot *CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	CanvasSlot->SetSize(GetDrawSize(GridFragment));
	//设置位置
	const FVector2D DrawPos=UInv_WidgetUtils::GetPositionFromIndex(Index,Columns)*TileSize;
	//设置内边距
	const FVector2D DrawPosWithPadding = DrawPos+FVector2D(GridFragment->GetGridPadding());
	CanvasSlot->SetPosition(DrawPosWithPadding);
	
	
}

void UInv_InventoryGrid::UpdateGridSlots(UInv_InventoryItem* NewItem, const int32 Index, bool bStackableItem,
	const int32 StackAmount)
{
	check(GridSlots.IsValidIndex(Index));

	if (bStackableItem)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}

	const FInv_GridFragment* GridFragment =GetFragment<FInv_GridFragment>(NewItem,FragmentTags::GridFragment);
	if (!GridFragment) return;

	const FIntPoint Dimensions=GridFragment ? GridFragment->GetGridSize() : FIntPoint(1,1);

	UInv_InventoryStatics::ForEach2D(GridSlots,Index,Dimensions,Columns,[&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

bool UInv_InventoryGrid::IsIndexClaimed(const TSet<int32>& CheckedIndndices, const int32 Index) const
{
	return CheckedIndndices.Contains(Index);
}

FVector2D UInv_InventoryGrid::GetDrawSize(const FInv_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding()*2;
	return GridFragment->GetGridSize()*IconTileWidth;
}

void UInv_InventoryGrid::SetSolottedItemImage(const UInv_SlottedItem* SlottedItem,
	const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment) const
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs=ESlateBrushDrawType::Image;
	Brush.ImageSize=GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(Brush);
}

void UInv_InventoryGrid::ConstructGrid()
{
	GridSlots.Empty();
	GridSlots.Reserve(Rows*Columns);

	for (int32 i=0;i<Rows;i++)
	{
		for (int32 j=0;j<Columns;j++)
		{
			UInv_GridSlot*GridSolot=CreateWidget<UInv_GridSlot>(this,GridSlotClass);
			CanvasPanel->AddChild(GridSolot);

			const FIntPoint TilePosition(j,i);
			GridSolot->SetTileIndex(UInv_WidgetUtils::GetIndexFromPosition(FIntPoint(j,i),Columns));

			UCanvasPanelSlot* GridCPS=UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSolot);
			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(TilePosition*TileSize);

			GridSlots.Add(GridSolot);
			GridSolot->GridSlotClicked.AddDynamic(this,&ThisClass::OnGridSlotClicked);
			GridSolot->GridSlotHovered.AddDynamic(this,&ThisClass::OnGridSlotHovered);
			GridSolot->GridSlotUnHovered.AddDynamic(this,&ThisClass::OnGridSlotUnHovered);
		}
	}
}

void UInv_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	//没选取的时候点击无效
	if (!IsValid(HoverItem)) return;
	//目标槽位索引无效返回
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;
	
	//交换物品检查（是否唯一物品）
	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		OnSlottedItemClicked(CurrentQueryResult.UpperLeftIndex,MouseEvent);
		return;
	}

	auto GridSlot = GridSlots[ItemDropIndex];
	if (!GridSlot->GetInventoryItem().IsValid())
	{
		//交换选取物品
		PutDownOnIndex(ItemDropIndex);
	}
}

void UInv_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	//将新物品添加到背包
	AddItemAtIndex(HoverItem->GetInventoryItem(),Index,HoverItem->IsStackable(),HoverItem->GetStackCount());
	//更新背包
	UpdateGridSlots(HoverItem->GetInventoryItem(),Index,HoverItem->IsStackable(),HoverItem->GetStackCount());

	ClearHoverItem();
	
}

void UInv_InventoryGrid::ClearHoverItem()
{
	if (!IsValid(HoverItem)) return;
	
	HoverItem->SetInventoryItem(nullptr);
	HoverItem->SetIsStackable(false);
	HoverItem->SetPreviousGridIndex(INDEX_NONE);
	HoverItem->UpdateStackCount(0);
	HoverItem->SetImageBrush(FSlateNoResource());
	
	HoverItem->RemoveFromParent();
	HoverItem = nullptr;
	
	//显示鼠标光标
	ShowCursor();
	
}

UUserWidget* UInv_InventoryGrid::GetVisibleCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(),VisibleCursorWidgetClass);
	}
	return VisibleCursorWidget;
}

UUserWidget* UInv_InventoryGrid::GetHiddenCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(),HiddenCursorWidgetClass);
	}
	return HiddenCursorWidget;
}

bool UInv_InventoryGrid::IsSameStackable(const UInv_InventoryItem* ClickedInventoryItem) const
{
	const bool bIsSameItem = ClickedInventoryItem == HoverItem->GetInventoryItem();	
	bool bIsStackable = ClickedInventoryItem->IsStackable();
	return bIsSameItem && bIsStackable && HoverItem->GetItemType().MatchesTagExact(ClickedInventoryItem->GetItemManifest().GetItemType());
}

void UInv_InventoryGrid::SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;
	
	UInv_InventoryItem* TempInventoryItem = HoverItem->GetInventoryItem();
	const int32 TempStackCount = HoverItem->GetStackCount();
	const bool bTempIsStackable = TempInventoryItem->IsStackable();
	
	//保持相同的前一个网格索引
	AssignHoverItem(ClickedInventoryItem,GridIndex,HoverItem->GetPreviousGridIndex());
	RemoveItemFromGrid(ClickedInventoryItem,GridIndex);
	AddItemAtIndex(TempInventoryItem,ItemDropIndex,bTempIsStackable,TempStackCount);
	UpdateGridSlots(TempInventoryItem,ItemDropIndex,bTempIsStackable,TempStackCount);
}

bool UInv_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount,
	const int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize ;
}

void UInv_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount,const int32 Index)
{
	UInv_GridSlot* GridSlot =GridSlots[Index];
	GridSlot->SetStackCount(HoveredStackCount);
	
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(HoveredStackCount);
	
	HoverItem->UpdateStackCount(ClickedStackCount);
}

bool UInv_InventoryGrid::ShouleConsumeHoverItemStacks(const int32 HoverStackCount, const int32 RoomInClickedSlot) const
{
	return RoomInClickedSlot >= HoverStackCount;
}

void UInv_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;
	
	GridSlots[Index]->SetStackCount(NewClickedStackCount);
	SlottedItems.FindChecked(Index)->UpdateStackCount(NewClickedStackCount);
	ClearHoverItem();
	ShowCursor();
	
	const FInv_GridFragment* GridFragment = GridSlots[Index]->GetInventoryItem()->GetItemManifest().GetFragmentOfType<FInv_GridFragment>();
	const FIntPoint Dimensions =GridFragment ? GridFragment->GetGridSize() : FIntPoint(1,1);
	HighlightSlots(Index,Dimensions);
}

bool UInv_InventoryGrid::ShouleFillInStacks(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}

void UInv_InventoryGrid::FillInStacks(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UInv_GridSlot* GridSlot =GridSlots[Index];
	const int32 NewStackCount = GridSlots[Index]->GetStackCount() + FillAmount;
	
	GridSlot->SetStackCount(NewStackCount);
	
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(NewStackCount);
	
	HoverItem->UpdateStackCount(Remainder);
}

void UInv_InventoryGrid::ShowCursor()
{
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default,GetVisibleCursorWidget());
}

void UInv_InventoryGrid::HideCursor()
{
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default,GetHiddenCursorWidget());
}

void UInv_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UInv_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UInv_GridSlot* GridSlot=GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetOccupiedTexture();
	}
}

void UInv_InventoryGrid::OnGridSlotUnHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;

	UInv_GridSlot* GridSlot=GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetUnoccupiedTexture();
	}
}

void UInv_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;
	
	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	AssignHoverItem(RightClickedItem,UpperLeftIndex,UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UInv_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	
	//将item转换为HoverItem，
	PickUp(RightClickedItem,Index);
	//将item放下
	DropItem();
}

void UInv_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	
	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	InventoryComponent->Server_ConsumeItem(RightClickedItem);
	
	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(RightClickedItem,UpperLeftIndex);
	}
	
}

void UInv_InventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		PutHoverItemBack();
	}
}

bool UInv_InventoryGrid::MatchesCateGory(const UInv_InventoryItem* Item) const
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}



