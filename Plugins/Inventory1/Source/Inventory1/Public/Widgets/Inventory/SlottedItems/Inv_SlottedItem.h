// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IAutomationReport.h"
#include "Blueprint/UserWidget.h"
#include "Items/Inv_InventoryItem.h"
#include "Inv_SlottedItem.generated.h"

class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSlottedItemClicked,int32,GridIndex,const FPointerEvent&,MouseEvent);

UCLASS() 
class INVENTORY1_API UInv_SlottedItem : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	bool GetIsStackable() const {return bIsStackable;}
	void SetIsStackable(bool bStackable){this->bIsStackable=bStackable;}
	UImage* GetImageIcon() const {return Image_Icon;}
	void SetGridIndex(int32 Index) {GridIndex = Index;}
	int32 GetGridIndex() const {return GridIndex;}
	void SetGridIndex(const FIntPoint& Dimensions) {GridDimensions=Dimensions;}
	FIntPoint GetGridDimensions() const {return GridDimensions;}
	void SetInventoryItem(UInv_InventoryItem*Item) ;
	UInv_InventoryItem* GetInventoryItem() const { return  InventoryItem.Get();}
	void SetImageBrush(const FSlateBrush& Brush)const;
	void UpdateStackCount(int32 StackCount);

	FSlottedItemClicked OnSlottedItemClicked;


private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	int32 GridIndex;
	FIntPoint GridDimensions;
	TWeakObjectPtr<UInv_InventoryItem> InventoryItem;
	bool bIsStackable{false};
};
