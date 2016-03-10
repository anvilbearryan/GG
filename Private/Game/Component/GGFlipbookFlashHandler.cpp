// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

// Sets default values for this component's properties
UGGFlipbookFlashHandler::UGGFlipbookFlashHandler()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UGGFlipbookFlashHandler::SetFlashSchedule(UMeshComponent* InUpdatedComponent, float InDuration)
{	
	UpdatedComponent = InUpdatedComponent;
	if (UpdatedComponent.IsValid())
	{		
		Duration = InDuration;
		Timemark = 0.f;
		FrameCount = 0;

		bIsFlipbook = !!Cast<UPaperFlipbookComponent>(InUpdatedComponent);
		if (!bIsFlipbook)
		{
			bIsSprite = !!Cast<UPaperSpriteComponent>(InUpdatedComponent);
		}
		if (bIsFlipbook || !bIsSprite)
		{
			SetActive(true);
		}
	}
	else
	{
		SetActive(false);
	}	
}

// Called every frame
void UGGFlipbookFlashHandler::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	if (bIsFlipbook)
	{
		TickFlipbook(DeltaTime);
	}
	else if (bIsSprite)
	{
		TickSprite(DeltaTime);
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("Something is wrong in this flashhandler"));
		SetActive(false);
	}
}

void UGGFlipbookFlashHandler::TickFlipbook(float DeltaTime)
{
	UPaperFlipbookComponent* updatedFlipbook = static_cast<UPaperFlipbookComponent*>(UpdatedComponent.Get());
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
		if (updatedFlipbook)
		{
			updatedFlipbook->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f));
			UpdatedComponent = nullptr;
		}
		SetActive(false);
	}
}

void UGGFlipbookFlashHandler::TickSprite(float DeltaTime)
{
	UPaperSpriteComponent* updatedSprite = static_cast<UPaperSpriteComponent*>(UpdatedComponent.Get());
	if (!!updatedSprite && Timemark < Duration)
	{
		Timemark += DeltaTime;
		FrameCount++;
		if (FrameCount % 3 == 0)
		{
			// == normal color for out additive material
			updatedSprite->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f));
		}
		else if (FrameCount % 3 == 1)
		{
			// == white color for out additive material
			updatedSprite->SetSpriteColor(FLinearColor(1.f, 1.f, 1.f));
		}
		else
		{
			// == invisible for out additive material
			updatedSprite->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		}
	}
	else
	{
		if (updatedSprite)
		{
			updatedSprite->SetSpriteColor(FLinearColor(0.f, 0.f, 0.f));
			UpdatedComponent = nullptr;
		}
		SetActive(false);
	}
}