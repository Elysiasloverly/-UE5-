// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Composite/Inv_Leaf_LabeledValue.h"

#include "Components/TextBlock.h"

void UInv_Leaf_LabeledValue::SetTExt_Label(const FText& Text,bool bCollapse) const
{
	if (bCollapse)
	{
		Text_Label->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Label->SetText(Text);
}

void UInv_Leaf_LabeledValue::SetTExt_Value(const FText& Text,bool bCollapse) const
{
	if (bCollapse)
	{
		Text_Value->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Value->SetText(Text);
}

void UInv_Leaf_LabeledValue::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	FSlateFontInfo FontInfo_Label = Text_Label->GetFont();
	FontInfo_Label.Size = FrontSize_Label;
	Text_Label->SetFont(FontInfo_Label);
	
	FSlateFontInfo FontInfo_Value = Text_Value->GetFont();
	FontInfo_Value.Size = FrontSize_Value;
	Text_Value->SetFont(FontInfo_Value);
}
