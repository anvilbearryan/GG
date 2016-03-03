// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGReconCharacter.h"
#include "Game/Component/Implementation/GGLocomotionAnimComponent.h"
#include "Game/Component/Implementation/GGReconRifleComponent.h"
#include "PaperFlipbookComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/Utility/GGFunctionLibrary.h"

FName AGGReconCharacter::WeaponEffectComponentName = TEXT("WeaponEffectComponent");

AGGReconCharacter::AGGReconCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeaponEffectComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(AGGReconCharacter::WeaponEffectComponentName);
	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->AttachTo(BodyFlipbookComponent);
	}
}

void AGGReconCharacter::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAction("Attack", IE_Pressed, this, &AGGReconCharacter::OnPressedAttack);
	InputComponent->BindAxis("AimUp", this, &AGGReconCharacter::AddAimInput);
}

void AGGReconCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RifleComponent = FindComponentByClass<UGGReconRifleComponent>();
	TInlineComponentArray<UGGLocomotionAnimComponent*> LocoComponents;
	GetComponents(LocoComponents);
	for (int32 i = 0; i < LocoComponents.Num(); i++)
	{
		if (i == 0)
		{
			LocomotionAnimComponent_Neutral = LocoComponents[i];
		}
		if (i == 1)
		{
			LocomotionAnimComponent_Up = LocoComponents[i];
		}
		if (i == 2)
		{
			LocomotionAnimComponent_Down = LocoComponents[i];
			UE_LOG(GGMessage, Log, TEXT("Found all locos"));
		}
	}

	/** TODO: handle game save loading */

	UGGReconRifleComponent* loc_Rifle = RifleComponent.Get();
	if (loc_Rifle)
	{
		loc_Rifle->SetComponentTickEnabled(false);
		loc_Rifle->OnInitiateAttack.AddDynamic(this, &AGGReconCharacter::OnBeginShoot);
		loc_Rifle->OnFinalizeAttack.AddDynamic(this, &AGGReconCharacter::OnFinishShoot);
		loc_Rifle->OwnerMovement = GetCharacterMovement();
	}

	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->OnFinishedPlaying.AddDynamic(this, &AGGReconCharacter::OnFinishWeaponEffectAnimation);
	}
	/** Apply saved data to components*/
}

void AGGReconCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGGReconCharacter, InputAimLevel_RepQuan, COND_SkipOwner);
}

void AGGReconCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Animation tick, everyone is interested in
	if (BodyFlipbookComponent)
	{
		switch (ActionState)
		{
		case EGGActionCategory::Locomotion:
		{
			UGGLocomotionAnimComponent* loc_LocomotionAnimComponent = GetActiveLocAnimComponent();
			if (loc_LocomotionAnimComponent) 
			{
				if (loc_LocomotionAnimComponent != LastActiveLocoAnimComponent)
				{
					UGGFunctionLibrary::BlendFlipbookToComponent(BodyFlipbookComponent, loc_LocomotionAnimComponent->GetCurrentAnimation());
					LastActiveLocoAnimComponent = loc_LocomotionAnimComponent;

					UE_LOG(GGWarning, Warning, TEXT("Change of anim state"));
				}
				else
				{
					BodyFlipbookComponent->SetFlipbook(loc_LocomotionAnimComponent->GetCurrentAnimation());					
				}
			}
			else
			{
				UE_LOG(GGWarning, Warning, TEXT("No active anim component"));
			}
		}
		break;
		case EGGActionCategory::Attack:
		{
			UGGReconRifleComponent* loc_RifleComponent = RifleComponent.Get();
			if (loc_RifleComponent && BodyFlipbookComponent)
			{				
				UPaperFlipbook* toFlipbook = loc_RifleComponent->GetCurrentBodyAnimation();
				if (bOverridePlaybackPosition)
				{
					// first frame only
					bOverridePlaybackPosition = false;
					BodyFlipbookComponent->SetFlipbook(toFlipbook);
				}
				else 
				{
					UGGFunctionLibrary::BlendFlipbookToComponent(BodyFlipbookComponent, toFlipbook);
				}				
			}
		}
		break;		
		}
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("No body flipbook component"));
	}
}

const float SMALL_DECIMAL_RECON = 0.2f;
UGGLocomotionAnimComponent* AGGReconCharacter::GetActiveLocAnimComponent() const
{
	if (IsLocallyControlled())
	{
		// use aim level
		if (InputAimLevel > SMALL_DECIMAL_RECON)
		{
			return LocomotionAnimComponent_Up.Get();
		}
		else if (InputAimLevel < -SMALL_DECIMAL_RECON)
		{
			return LocomotionAnimComponent_Down.Get();
		}
	}
	else
	{
		if (InputAimLevel_RepQuan > 1)
		{
			return LocomotionAnimComponent_Up.Get();
		}
		else if (InputAimLevel_RepQuan == 0)
		{
			return LocomotionAnimComponent_Down.Get();
		}
	}
	return LocomotionAnimComponent_Neutral.Get();
}

void AGGReconCharacter::ReceiveDamage(const FGGDamageReceivingInfo& InDamageInfo)
{
	// base class method takes care of applying the damage information and flashese the body flipbook
	Super::ReceiveDamage(InDamageInfo);
	
	// Recon characters do typical platformer nudge moving damage reaction, default implementation stands still
	EnforcedMovementStrength = 0.5f;
	FVector2D impactDirection = InDamageInfo.GetImpactDirection();
	EnforcedMovementDirection = FVector(0.f, impactDirection.X, impactDirection.Y);

	ActionState = EGGActionCategory::Damaged;
	BodyFlipbookComponent->SetLooping(true);
	BodyFlipbookComponent->SetFlipbook(ReceiveDamageFlipbook);
	BodyFlipbookComponent->Play();
}

void AGGReconCharacter::OnCompleteDamageReceiveReaction()
{
	Super::OnCompleteDamageReceiveReaction();
	bUseEnforcedMovement = false;
	ActionState = EGGActionCategory::Locomotion;
}

void AGGReconCharacter::OnBeginShoot()
{
	ActionState = EGGActionCategory::Attack;
	if (BodyFlipbookComponent)
	{
		BodyFlipbookComponent->SetLooping(true);
		bOverridePlaybackPosition = true;
	}
	UGGReconRifleComponent* loc_RifleComponent = RifleComponent.Get();
	if (loc_RifleComponent && BodyFlipbookComponent)
	{
		UPaperFlipbook* loc_EffectFlipbook = loc_RifleComponent->GetEffectAnimation();
		if (loc_EffectFlipbook != nullptr)
		{
			WeaponEffectComponent->SetFlipbook(loc_EffectFlipbook);
			WeaponEffectComponent->SetVisibility(true);
			WeaponEffectComponent->SetLooping(false);
			WeaponEffectComponent->PlayFromStart();
		}
	}
}

void AGGReconCharacter::OnFinishShoot()
{
	bOverridePlaybackPosition = true;
	ActionState = EGGActionCategory::Locomotion;
	if (BodyFlipbookComponent)
	{
		BodyFlipbookComponent->SetLooping(true);
		BodyFlipbookComponent->Play();
	}
}

void AGGReconCharacter::OnFinishWeaponEffectAnimation()
{
	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->SetVisibility(false);
	}
}

void AGGReconCharacter::AddAimInput(float ScaleValue)
{
	if (bActionInputDisabled)
	{
		return;
	}
	uint8 locProposedAimLevel = 0;
	if (ScaleValue > SMALL_DECIMAL_RECON)
	{
		InputAimLevel = 1.f;
		locProposedAimLevel = 2;
	}
	else if (ScaleValue < -SMALL_DECIMAL_RECON)
	{
		InputAimLevel = -1.f;
	}
	else
	{
		InputAimLevel = 0.f;
		locProposedAimLevel = 1;
	}
	if (InputAimLevel_RepQuan != locProposedAimLevel)
	{		
		ServerUpdateAim(locProposedAimLevel);
		// ensure local player updates the value, though listen server would do it twice but that doesn't hurt
		InputAimLevel_RepQuan = locProposedAimLevel;
	}
}

bool AGGReconCharacter::ServerUpdateAim_Validate(uint8 NewAimLevel)
{
	return true;
}

void AGGReconCharacter::ServerUpdateAim_Implementation(uint8 NewAimLevel)
{
	InputAimLevel_RepQuan = NewAimLevel;
}

void AGGReconCharacter::OnPressedAttack()
{
	if (bActionInputDisabled)
	{
		return;
	}
	
	UGGReconRifleComponent* loc_RifleComponent = RifleComponent.Get();
	if (loc_RifleComponent)
	{
		uint8 locAimLevel = 0;
		if (FMath::Abs(InputAimLevel) < SMALL_DECIMAL_RECON)
		{
			locAimLevel = 1;
		}
		else if (InputAimLevel > 0)
		{
			locAimLevel = 2;
		}
		loc_RifleComponent->LocalAttemptsAttack(false, locAimLevel);
	}
}
