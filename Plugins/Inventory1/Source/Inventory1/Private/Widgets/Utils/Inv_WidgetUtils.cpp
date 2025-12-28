// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Utils/Inv_WidgetUtils.h"

#include "IDetailTreeNode.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Widget.h"
#include "Items/Inv_InventoryItem.h"


FVector2D UInv_WidgetUtils::GetWidgetLocation(UWidget* Widget)
{
	const FGeometry Geometry=Widget->GetCachedGeometry();
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::LocalToViewport(Widget,Geometry,USlateBlueprintLibrary::GetLocalTopLeft(Geometry),PixelPosition,ViewportPosition);
	return ViewportPosition;
	
}

FVector2D UInv_WidgetUtils::GetWidgetSize(UWidget* Widget)
{
	const FGeometry Geometry=Widget->GetCachedGeometry();
	return Geometry.GetLocalSize();
}

bool UInv_WidgetUtils::IsWitchinBounds(const FVector2D& BoundaryPos, const FVector2D& WidgetSize,
									   const FVector2D& MousePos)
{
	
	return MousePos.X >= BoundaryPos.X && 
		   MousePos.X <= (BoundaryPos.X + WidgetSize.X) &&
		   MousePos.Y >= BoundaryPos.Y &&  
		   MousePos.Y <= (BoundaryPos.Y + WidgetSize.Y);
}

FVector2D UInv_WidgetUtils::GetClampWidgetPosition(const FVector2D& Boundary, const FVector2D& WidgetSize,
	const FVector2D& MousePos)
{
	FVector2D ClampedPosition = MousePos;
	
	//调整水平位置确保在组件内
	if (MousePos.X + WidgetSize.X > Boundary.X)
	{
		ClampedPosition.X = Boundary.X - WidgetSize.X;
	}
	if (MousePos.X < 0.f)
	{
		ClampedPosition.X = 0.f;
	}
	
	if (MousePos.Y + WidgetSize.Y > Boundary.Y)
	{
		ClampedPosition.Y = Boundary.Y - WidgetSize.Y;
	}
	if (MousePos.Y < 0.f)
	{
		ClampedPosition.Y = 0.f;
	}
	
	return ClampedPosition;
	
}

int32 UInv_WidgetUtils::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns)
{
	return Position.Y * Columns + Position.X;
}

FIntPoint UInv_WidgetUtils::GetPositionFromIndex(const int32 Index, const int32 Columns)
{
	if (Columns == 0) return FIntPoint::ZeroValue;
	
	return FIntPoint(Index % Columns, Index / Columns); 
}

