// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inv_WidgetUtils.generated.h"

class UInv_ItemComponent;
/**
 * 
 */
UCLASS()
class INVENTORY1_API UInv_WidgetUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static FVector2D GetWidgetLocation(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static FVector2D GetWidgetSize(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool IsWitchinBounds(const FVector2D& BoundaryPos,const FVector2D& WidgetSize,const FVector2D& MousePos);
	
	static FVector2D GetClampWidgetPosition(const FVector2D& Boundary,const FVector2D& WidgetSize,const FVector2D& MousePos);
	
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static int32 GetIndexFromPosition(const FIntPoint& Position,const int32 Columns);
	static FIntPoint GetPositionFromIndex(const int32 Index,const int32 Columns);
};
