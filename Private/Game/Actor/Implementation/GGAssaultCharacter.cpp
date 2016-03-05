// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGAssaultCharacter.h"
#include "Game/Component/Implementation/GGSlashAttackComponent.h"
#include "Game/Component/Implementation/GGLocomotionAnimComponent.h"

#include "PaperFlipbookComponent.h"

FName AGGAssaultCharacter::WeaponEffectComponentName = TEXT("WeaponEffectComponent");

AGGAssaultCharacter::AGGAssaultCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeaponEffectComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(AGGAssaultCharacter::WeaponEffectComponentName);
	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->AttachTo(BodyFlipbookComponent);
	}
}

void AGGAssaultCharacter::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAction("Attack", IE_Pressed, this, &AGGAssaultCharacter::OnPressedAttack);
}

void AGGAssaultCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();	

	NormalSlashAttackComponent = FindComponentByClass<UGGSlashAttackComponent>();
	
	LocomotionAnimComponent = FindComponentByClass<UGGLocomotionAnimComponent>();
	/** TODO: handle game save loading */
	
	UGGSlashAttackComponent* loc_SlashComp = NormalSlashAttackComponent.Get();
	if (loc_SlashComp)
	{		
		loc_SlashComp->SetActive(false);
		loc_SlashComp->OnInitiateAttack.AddDynamic(this, &AGGAssaultCharacter::OnUseSlashAttack);
		loc_SlashComp->OnFinalizeAttack.AddDynamic(this, &AGGAssaultCharacter::OnFinishSlashAttack);
		loc_SlashComp->OwnerMovement = GetCharacterMovement();
	}
	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->OnFinishedPlaying.AddDynamic(this, &AGGAssaultCharacter::OnFinishWeaponEffectAnimation);
	}
	/** Apply saved data to components*/	
}

void AGGAssaultCharacter::Tick(float DeltaSeconds)
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
			UGGSlashAttackComponent* loc_AttackComponent = NormalSlashAttackComponent.Get();
			if (loc_AttackComponent && loc_AttackComponent->ShouldRemainInState())
			{
				BodyFlipbookComponent->SetFlipbook(loc_AttackComponent->GetCurrentAnimation());
			}
			else
			{
				ActionState = EGGActionCategory::Locomotion;
				BodyFlipbookComponent->SetLooping(true);
				UGGLocomotionAnimComponent* loc_LocomotionAnimComponent = LocomotionAnimComponent.Get();
				BodyFlipbookComponent->SetFlipbook(loc_LocomotionAnimComponent->GetCurrentAnimation());
			}
		}
		break;
		}
	}
}

void AGGAssaultCharacter::CommenceDamageReaction(const FGGDamageReceivingInfo& InDamageInfo)
{
	// base class method takes care of applying the damage information and flashese the body flipbook
	Super::CommenceDamageReaction(InDamageInfo);
	// AssaultCharacters do not recoil, return
}

void AGGAssaultCharacter::OnUseSlashAttack()
{
	ActionState = EGGActionCategory::Attack;
	if (BodyFlipbookComponent) 
	{
		BodyFlipbookComponent->SetLooping(false);
	}
	UGGSlashAttackComponent* loc_AttackComponent = NormalSlashAttackComponent.Get();
	if (loc_AttackComponent)
	{
		UPaperFlipbook* loc_EffectFlipbook = loc_AttackComponent->GetEffectAnimation();
		if (loc_EffectFlipbook != nullptr)
		{
			WeaponEffectComponent->SetFlipbook(loc_EffectFlipbook);
			WeaponEffectComponent->SetVisibility(true);
			WeaponEffectComponent->SetLooping(false);
			WeaponEffectComponent->PlayFromStart();
		}
	}
}

void AGGAssaultCharacter::OnFinishSlashAttack()
{
	ActionState = EGGActionCategory::Locomotion;
	if (BodyFlipbookComponent)
	{
		BodyFlipbookComponent->SetLooping(true);
		BodyFlipbookComponent->Play();
	}	
}

void AGGAssaultCharacter::OnFinishWeaponEffectAnimation()
{
	if (WeaponEffectComponent)
	{
		WeaponEffectComponent->SetVisibility(false);
	}
}

void AGGAssaultCharacter::OnPressedAttack()
{
	if (bActionInputDisabled)
	{
		return;
	}
	UGGSlashAttackComponent* loc_AttackComponent = NormalSlashAttackComponent.Get();
	if (loc_AttackComponent && GetCharacterMovement())
	{
		loc_AttackComponent->LocalAttemptsAttack(false, GetVelocity().Y != 0.f);
	}
}

