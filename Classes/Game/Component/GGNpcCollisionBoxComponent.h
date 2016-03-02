// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/BoxComponent.h"
#include "GGNpcCollisionBoxComponent.generated.h"

/**
 * A collision box tied to Npcs that damages players on contact. Defaults to Standard damage. 
 *	Independent of needing a minion owner so we might also be able to use it for damage volumes.
 */
class AGGCharacter;
UCLASS()
class GG_API UGGNpcCollisionBoxComponent : public UBoxComponent
{
	GENERATED_BODY()
	/** The additive adjustment for collision damage based on default damage level */
	UPROPERTY(EditDefaultsOnly, Category = "GG|Damage")
		int32 CollisionDamageStrength_Additive;	
	/** The multiplicative adjustment for collision damage based on default damage level */
	UPROPERTY(EditDefaultsOnly, Category = "GG|Damage")
		float CollisionDamageStrength_Multiplicative;
	
	TWeakObjectPtr<AGGCharacter> OverlappeddCharacter;
	TWeakObjectPtr<UPrimitiveComponent> OverlappedCharacterHitbox;
	
public:	
	UGGNpcCollisionBoxComponent();

	virtual void InitializeComponent() override;

	UFUNCTION()
		void BeginOverlapToggle(
			AActor* OtherActor, UPrimitiveComponent*  OtherComp, int32 OtherBodyIndex, 
			bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
		void EndOverlapToggle(AActor* OtherActor, UPrimitiveComponent*  OtherComp, int32 OtherBodyIndex);
	
	void OnOverlapCharacter(AGGCharacter* InCharacter);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
};
