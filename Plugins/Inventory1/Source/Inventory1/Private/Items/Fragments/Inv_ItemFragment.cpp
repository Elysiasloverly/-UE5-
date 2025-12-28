#include "Items/Fragments/Inv_ItemFragment.h"

#include "EquipmentManagement/EquipActor/Inv_EquipActor.h"
#include "Widgets/Composite/Inv_CompositeBase.h"
#include "Widgets/Composite/Inv_Leaf_Image.h"
#include "Widgets/Composite/Inv_Leaf_LabeledValue.h"
#include "Widgets/Composite/Inv_Leaf_Text.h"

void FInv_InventoryItemFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	if (!MatchesWidgetTag(Composite)) return;
	Composite->Expand();
}

bool FInv_InventoryItemFragment::MatchesWidgetTag(const UInv_CompositeBase* Composite) const
{
	return Composite->GetFragmentTag().MatchesTagExact(GetFragmentTag());
}

void FInv_ImageFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	FInv_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;
	
	UInv_Leaf_Image* Image = Cast<UInv_Leaf_Image>(Composite);
	if (!IsValid(Image)) return;
	
	Image->SetImage(Icon);
	Image->SetBoxSize(IconDimensions);
	Image->SetImageSize(IconDimensions);
}

void FInv_TextFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	FInv_InventoryItemFragment::Assimilate(Composite);
	FInv_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;
	
	UInv_Leaf_Text* LeafText = Cast<UInv_Leaf_Text>(Composite);
	if (!IsValid(LeafText)) return;
	
	LeafText->SetText(FragmentText);
}

void FInv_LabeledNumFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	FInv_InventoryItemFragment::Assimilate(Composite);
	if (!MatchesWidgetTag(Composite)) return;
	
	UInv_Leaf_LabeledValue* LabeledValue = Cast<UInv_Leaf_LabeledValue>(Composite);
	if (!IsValid(LabeledValue)) return;
	
	LabeledValue->SetTExt_Label(Text_Label,bCollapseLabel);
	
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = MinFractionalDigits;
	Options.MaximumFractionalDigits = MaxFractionalDigits;
	
	LabeledValue->SetTExt_Value(FText::AsNumber(Value,&Options),bCollapseValue);
}

void FInv_LabeledNumFragment::Manifest()
{
	FInv_InventoryItemFragment::Manifest();
	
	if (bRandomizeOnManifest)
	{
		Value = FMath::RandRange(Min,Max);
	}
	
	bRandomizeOnManifest= false;
}


void FInv_ConsumableFragment::OnConsume(APlayerController* PC)
{
	for (auto& Modifier: ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnConsume(PC);
	}
}

void FInv_ConsumableFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	FInv_InventoryItemFragment::Assimilate(Composite);
	for (const auto& Modifier: ConsumeModifiers)
	{
		const auto& ModRef = Modifier.Get();
		ModRef.Assimilate(Composite);
	}
}

void FInv_ConsumableFragment::Manifest()
{
	FInv_InventoryItemFragment::Manifest();
	for (auto& Modifier: ConsumeModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.Manifest();
	}
}

void FInv_HealthPotionFragment::OnConsume(APlayerController* PC)
{
	//添加消耗效果
	// 从玩家控制器或其控制的Pawn获取统计组件
	// 或获取能力系统组件并施加游戏效果
	// 或调用治疗功能的接口函数
	
	GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Green,FString::Printf(TEXT("恢复了%f点生命值"),GetValue()));
}

void FInv_ManaPotionFragment::OnConsume(APlayerController* PC)
{
	//添加消耗效果
	
	GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Green,FString::Printf(TEXT("恢复了%f点法力值"),GetValue()));
}

void FInv_StrengthModifier::OnEquip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Green,FString::Printf(TEXT("增加了%f点力量"),GetValue()));
}

void FInv_StrengthModifier::OnUnequip(APlayerController* PC)
{
	GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Red,FString::Printf(TEXT("减少了%f点力量"),GetValue()));
}

void FInv_EquipmentFragment::OnEquip(APlayerController* PC)
{
	if (bEquipped) return;
	bEquipped=true;
	for (auto& Modifier: EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnEquip(PC);
	}
}

void FInv_EquipmentFragment::OnUnequip(APlayerController* PC)
{	
	if (!bEquipped) return;
	bEquipped=false;
	for (auto& Modifier: EquipModifiers)
	{
		auto& ModRef = Modifier.GetMutable();
		ModRef.OnUnequip(PC);
	}
}

void FInv_EquipmentFragment::Assimilate(UInv_CompositeBase* Composite) const
{
	FInv_InventoryItemFragment::Assimilate(Composite);
	for (auto& Modifier: EquipModifiers)
	{
		const auto& ModRef = Modifier.Get<>();
		ModRef.Assimilate(Composite);
	}
}

void FInv_EquipmentFragment::Manifest()
{
	FInv_InventoryItemFragment::Manifest();
	for (auto& Modifier: EquipModifiers)
	{
		 auto& ModRef = Modifier.GetMutable<>();
		ModRef.Manifest();
	}
}

AInv_EquipActor* FInv_EquipmentFragment::SpawnAttachedActor(USkeletalMeshComponent* AttachMesh) const
{
	if (!IsValid(EquipActorClass) || !IsValid(AttachMesh)) return nullptr;
	
	AInv_EquipActor* SpawnedActor = AttachMesh->GetWorld()->SpawnActor<AInv_EquipActor>(EquipActorClass);
	SpawnedActor->AttachToComponent(AttachMesh,FAttachmentTransformRules::SnapToTargetNotIncludingScale,SocketAttachPoint);
	
	return SpawnedActor;
}

void FInv_EquipmentFragment::DestroyEquipActor() const
{
	if (EquippedActor.IsValid())
	{
		EquippedActor->Destroy();
	}
}

void FInv_EquipmentFragment::SetEquippedActor(AInv_EquipActor* EquipActor)
{
	EquippedActor=EquipActor;
}
