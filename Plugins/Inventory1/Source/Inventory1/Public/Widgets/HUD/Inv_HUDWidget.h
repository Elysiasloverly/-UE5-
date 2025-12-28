// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inv_InfoMessage.h"
#include "Blueprint/UserWidget.h"
#include "Inv_HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY1_API UInv_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	virtual void NativeOnInitialized() override;
	//接受物品信息
	UFUNCTION(BlueprintImplementableEvent,Category="Inventory")
	void ShowPickupMessage(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent,Category="Inventory")
	void HidePickupMessage();

private:

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UInv_InfoMessage> InfoMessage;

	UFUNCTION()
	void OnNoRoom();
	
};
