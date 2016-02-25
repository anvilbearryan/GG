// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGReconCharacter.h"
#include "Game/Component/Implementation/GGLocomotionAnimComponent.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Game/Component/Implementation/GGReconRifleComponent.h"
#include "PaperFlipbookComponent.h"


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

	HealthComponent = FindComponentByClass<UGGDamageReceiveComponent>();
	LocomotionAnimComponent = FindComponentByClass<UGGLocomotionAnimComponent>();
	/** TODO: handle game save loading */


	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->OnFinishedPlaying.AddDynamic(this, &AGGReconCharacter::OnFinishWeaponEffectAnimation);
	}
	/** Apply saved data to components*/
	if (HealthComponent.IsValid())
	{
		HealthComponent.Get()->InitializeHpState();
	}
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
			UGGLocomotionAnimComponent* loc_LocomotionAnimComponent = LocomotionAnimComponent.Get();
			if (loc_LocomotionAnimComponent)
			{
				BodyFlipbookComponent->SetFlipbook(loc_LocomotionAnimComponent->GetCurrentAnimation());
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
				else if (toFlipbook && loc_RifleComponent->GetFlipbook() != toFlipbook)
				{
					// find current playbkpos
					float currentPlaybackPositionNormalized =
						BodyFlipbookComponent->GetPlaybackPosition() / BodyFlipbookComponent->GetFlipbookLength();
					// conserve playback position
					BodyFlipbookComponent->SetFlipbook(toFlipbook);
					BodyFlipbookComponent->SetNewtime(currentPlaybackPositionNormalized*toFlipbook->GetTotalDuration());
				}				
			}
		}
		break;
		}
	}
}

void AGGReconCharacter::ReceiveDamage(int32 DamageData)
{
	Super::ReceiveDamage(DamageData);
	// ask damage receiving component to handle it
	UGGDamageReceiveComponent* loc_Hp = HealthComponent.Get();
	if (loc_Hp)
	{
		loc_Hp->HandleDamageData(DamageData);
	}
	// should flash flipbook component
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
	const float AIM_DEADZONE = 0.1f;
	if (ScaleValue > AIM_DEADZONE)
	{
		AimLevel = 1.f;
	}
	else if (ScaleValue < -AIM_DEADZONE)
	{
		AimLevel = -1.f;
	}
	else
	{
		AimLevel = 0.f;
	}
}

void AGGReconCharacter::OnPressedAttack()
{
	
}