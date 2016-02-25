// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGCharacter.h"
#include "Game/Data/GGGameTypes.h"
#include "GGReconCharacter.generated.h"

class UGGDamageReceiveComponent;
class UGGLocomotionAnimComponent;
class UGGReaconRifleComponent;
class UPaperFlipbookComponent;

UCLASS()
class GG_API AGGReconCharacter : public AGGCharacter
{
	GENERATED_BODY()
public:	
	static FName WeaponEffectComponentName;
	// Composition
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GG|Animation")
		UPaperFlipbookComponent* WeaponEffectComponent;
	TWeakObjectPtr<UGGLocomotionAnimComponent> LocomotionAnimComponent;	
	TWeakObjectPtr<UGGDamageReceiveComponent> HealthComponent;
	TWeakObjectPtr<UGGReaconRifleComponent> RifleComponent;

	// States
	TEnumAsByte<EGGActionCategory::Type> ActionState;	
	uint8 bOverridePlaybackPosition: 1;


	AGGReconCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// Override to obtain reference to attack and health components
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaSeconds) override;

	// Assault character passive: no interrupt
	virtual void ReceiveDamage(int32 DamageData) override;
	
	UFUNCTION()
		void OnBeginShoot();
	UFUNCTION()
		void OnFinishShoot();
	UFUNCTION()
		void OnFinishWeaponEffectAnimation();

	/** Specific input handling */
	float AimLevel;
	void AddAimInput(float ScaleValue = 0.f);
	void OnPressedAttack();
};
