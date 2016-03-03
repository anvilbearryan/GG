// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "PaperFlipbookComponent.h"

// Sets default values for this component's properties
UGGFlipbookFlashHandler::UGGFlipbookFlashHandler()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UGGFlipbookFlashHandler::SetFlashSchedule(UPaperFlipbookComponent* InUpdatedComponent, float InDuration)
{	
	UpdatedComponent = InUpdatedComponent;
	if (UpdatedComponent.IsValid())
	{		
		Duration = InDuration;
		Timemark = 0.f;
		FrameCount = 0;
		SetActive(true);
		SetComponentTickEnabled(true);
	}
	else
	{
		SetActive(false);
		SetComponentTickEnabled(true);
	}
}

// Called every frame
void UGGFlipbookFlashHandler::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	UPaperFlipbookComponent* updatedFlipbook = UpdatedComponent.Get();
	if (!!updatedFlipbook && Timemark < Duration)
	{
		Timemark += DeltaTime;
		FrameCount++;
		if (FrameCount % 3 == 0)
		{
			// == normal color for out additive material
			updatedFlipbook->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f));
		}
		else if (FrameCount % 3 == 1)
		{
			// == white color for out additive material
			updatedFlipbook->SetSpriteColor(FLinearColor(1.f, 1.f, 1.f));
		}
		else
		{
			// == invisible for out additive material
			updatedFlipbook->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		}
	}
	else
	{
		SetActive(false);
	}
}

