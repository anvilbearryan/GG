// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Game/Data/GGGameTypes.h"
#include "GGPickup.generated.h"

class UPaperSpriteComponent;
class UGGFlipbookFlashHandler;
class AGGCharacter;

USTRUCT(BlueprintType)
struct FGGLootReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GGPickups")
		EGGLootTypes Type;
	UPROPERTY(EditAnywhere, Category = "GGPickups")
		int32 Value;
};

UCLASS(Abstract, Blueprintable, ClassGroup = "GGPickups", meta = (BlueprintSpawnableComponent))
class GG_API AGGPickup : public AActor
{
	GENERATED_BODY()

	// ********************************
	// Specification
	UPROPERTY(EditAnywhere, Category ="GGPickups")
		FGGLootReward LootReward;
	/** Time this pickup exist normally before starting disappeareance sequence */
	UPROPERTY(EditAnywhere, Category = "GGPickups")
		float Lifetime_Normal;
	/** We flashes this pickup to indicate it is disappearing, this denotes the duration of such flash */
	UPROPERTY(EditAnywhere, Category = "GGPickups")
		float Lifetime_Vanishing;
	UPROPERTY(EditAnywhere, Category = "GGPickups")
		USoundCue* PickupSound;
	// ********************************
	// Composition	 
	TWeakObjectPtr<UPrimitiveComponent> ViewComponent; // so it can be a sprite or flipbook
	// for disappearing
	TWeakObjectPtr<UGGFlipbookFlashHandler> FlashComponent;

	// ********************************
	// States	 
	uint8 bConsumed : 1;
	FTimerHandle LifecycleHandle;

public:	
	// Sets default values for this actor's properties
	AGGPickup();
	
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	virtual void NotifyActorBeginOverlap(AActor* Other) override;

	bool CanBePickedUpBy(AActor* actor) const;
	void OnPickedUpBy(AGGCharacter* character);

	UFUNCTION()
		void BeginVanishing();
	UFUNCTION()
		void RemoveFromGame();
};
