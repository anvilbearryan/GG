// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGCharacter.h"
#include "Game/Data/GGGameTypes.h"
#include "GGReconCharacter.generated.h"

class UGGDamageReceiveComponent;
class UGGLocomotionAnimComponent;
class UGGReconRifleComponent;
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
	
	TWeakObjectPtr<UGGDamageReceiveComponent> HealthComponent;
	TWeakObjectPtr<UGGReconRifleComponent> RifleComponent;

	TWeakObjectPtr<UGGLocomotionAnimComponent> LocomotionAnimComponent_Neutral;
	TWeakObjectPtr<UGGLocomotionAnimComponent> LocomotionAnimComponent_Up;
	TWeakObjectPtr<UGGLocomotionAnimComponent> LocomotionAnimComponent_Down;

	// States
	TEnumAsByte<EGGActionCategory::Type> ActionState;	
	uint8 bOverridePlaybackPosition: 1;
	
	UGGLocomotionAnimComponent* LastActiveLocoAnimComponent;

	float InputAimLevel;
	UPROPERTY(Transient, Replicated)
		uint8 InputAimLevel_RepQuan;

	AGGReconCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// Override to obtain reference to attack and health components
	virtual void PostInitializeComponents() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	UGGLocomotionAnimComponent* GetActiveLocAnimComponent() const;

	// Recon character passive: dodge
	virtual void ReceiveDamage(const FGGDamageReceivingInfo& InDamageInfo) override;
	
	UFUNCTION()
		void OnBeginShoot();
	UFUNCTION()
		void OnFinishShoot();
	UFUNCTION()
		void OnFinishWeaponEffectAnimation();

	/** Specific input handling */
	void AddAimInput(float ScaleValue = 0.f);
	UFUNCTION(Server, unreliable, WithValidation)
		void ServerUpdateAim(uint8 NewAimLevel);
	bool ServerUpdateAim_Validate(uint8 NewAimLevel);
	void ServerUpdateAim_Implementation(uint8 NewAimLevel);
	void OnPressedAttack();
};
