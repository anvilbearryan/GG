// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGReconCharacter.h"
#include "Game/Component/Implementation/GGLocomotionAnimComponent.h"
#include "Game/Component/GGDamageReceiveComponent.h"
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