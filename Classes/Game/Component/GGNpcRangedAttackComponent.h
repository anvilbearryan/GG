// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGNpcRangedAttackComponent.generated.h"

/**
 *	A generic component which hopefully any minion class can use for launching projectiles. For reason to not create 
 *	numerous such component each for different enemy entities, data aspect of specification lies with the owning actor
 *	instead of within the component unlike those for character. instead, launch function with more parametres for
 *	specification is used and called by the owning actor.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GG_API UGGNpcRangedAttackComponent : public UActorComponent
{
	GENERATED_BODY()
protected:
	//********************************
	//	Specification
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TArray<TEnumAsByte<ECollisionChannel>> TargetObjectTypes;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> ProjectileObjectType;
	/** sprites updated by this component */
	TArray<FLaunchedProjectile, TInlineAllocator<8>> UpdatedProjectiles;
public:	
	TWeakObjectPtr<AGGSpritePool> SpritePool;	
		
	// Sets default values for this component's properties
	UGGNpcRangedAttackComponent();
	
	void LaunchProjectile(UGGProjectileData* InData, const FVector& InLaunchLocation, 
		const FVector& InLaunchDirection, FVector InLaunchScale = FVector(1.f,1.f,1.f), 
		FQuat InLaunchRotation = FQuat(0.f, 0.f, 1.41421356f * 0.5f, 1.41421356f * 0.5f));

	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

		
	
};
