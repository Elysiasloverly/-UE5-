// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Inv_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory1.h"
#include "Engine/GameViewportClient.h" 
#include "Interaction/Inv_Highlightable.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/HUD/Inv_HUDWidget.h" 

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;//开启tick
	TraceLength = 500.0f;
	ItemTraceChannel=ECC_GameTraceChannel1;	
}

void AInv_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TraceForItem();
}

void AInv_PlayerController::ToggleInventory()
{
	if (!InventoryComponent.IsValid())return;

	InventoryComponent->ToggleInventoryMenu();
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	//初始化增强输入系统
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem
		<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultIMC, 0);
	}
 
	InventoryComponent=FindComponentByClass<UInv_InventoryComponent>();
	CreateHUDWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked
		<UEnhancedInputComponent>(InputComponent))
	{
		
		EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
		EnhancedInputComponent->BindAction(ToggleInteractAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
	}
}

void AInv_PlayerController::PrimaryInteract()
{
	if (!IsLocalController()) return;
 	
	
	if (!ThisActor.IsValid()) return;
	
	
	UInv_ItemComponent* ItemComp=ThisActor->FindComponentByClass<UInv_ItemComponent>();
	if (!IsValid(ItemComp)||!InventoryComponent.IsValid()) return;
	
	InventoryComponent->TryAddItem(ItemComp);
}

void AInv_PlayerController::CreateHUDWidget()
{
	if (!IsLocalController()) return;


	if (!HUDWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("HUDWidgetClass is missing in Inv_PlayerController!"));
		return;
	}

	HUDWidget = CreateWidget<UInv_HUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}

void AInv_PlayerController::TraceForItem()
{

	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	

	const FVector2D ViewportCenter = ViewportSize / 2;
	FVector TraceStart;
	FVector Forward;

	
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) 
	{
		return;
	}

	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	
	FHitResult HitResult;

	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); 
	if (GetPawn())
	{
		QueryParams.AddIgnoredActor(GetPawn()); 
	}

	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel, QueryParams);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	
	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
	}
	
	if (ThisActor == LastActor) return;
	
	if (ThisActor.IsValid())
	{
		if (UActorComponent* Highlightable=ThisActor->FindComponentByInterface(UInv_Highlightable::StaticClass());IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_Highlight(Highlightable);
		}
		
		UInv_ItemComponent* ItemComponent=ThisActor->FindComponentByClass<UInv_ItemComponent>();
		
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HUDWidget))HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}


	if (LastActor.IsValid())
	{
		if (UActorComponent* Highlightable=LastActor->FindComponentByInterface(UInv_Highlightable::StaticClass());IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
	
}