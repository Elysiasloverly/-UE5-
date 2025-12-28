// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inv_Leaf.h"
#include "Inv_Leaf_LabeledValue.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class INVENTORY1_API UInv_Leaf_LabeledValue : public UInv_Leaf
{
	GENERATED_BODY()
	
public:
	
	void SetTExt_Label(const FText& Text,bool bCollapse)const;
	void SetTExt_Value(const FText& Text,bool bCollapse)const;
	virtual void NativePreConstruct()override;
	
private:
	
	UPROPERTY( meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Label;
		
	UPROPERTY( meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Value;
	
	UPROPERTY(EditAnywhere,Category="Inventory")
	int32 FrontSize_Label{12};
	
	UPROPERTY(EditAnywhere,Category="Inventory")
	int32 FrontSize_Value{18};
	
};
