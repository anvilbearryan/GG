// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGCharacter.h"
#include "Game/Data/GGGameTypes.h"
#include "GGAssaultCharacter.generated.h"

/**
 * 
 */
class UGGSlashAttackComponent;
class UGGDamageReceiveComponent;
class UGGLocomotionAnimComponent;
class UPaperFlipbookComponent;

UCLASS()
class GG_API AGGAssaultCharacter : public AGGCharacter
{
	GENERATED_BODY()
public:
	static FName WeaponEffectComponentName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GG|Animation")
		UPaperFlipbookComponent* WeaponEffectComponent;

	TWeakObjectPtr<UGGLocomotionAnimComponent> LocomotionAnimComponent;
	TWeakObjectPtr<UGGSlashAttackComponent> NormalSlashAttackComponent;	
	TWeakObjectPtr<UGGDamageReceiveComponent> HealthComponent;
	
	TEnumAsByte<EGGActionCategory::Type> ActionState;

	AGGAssaultCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// Override to obtain reference to attack and health components
	virtual void PostInitializeComponents() override;
	
	virtual void Tick(float DeltaSeconds) override;

	// Assault character passive: no interrupt
	virtual void ReceiveDamage(int32 DamageData) override;

	UFUNCTION()
		void OnUseSlashAttack();
	UFUNCTION()
		void OnFinishSlashAttack();
	UFUNCTION()
		void OnFinishWeaponEffectAnimation();

	/** Specific input handling */
	void OnPressedAttack();
};
