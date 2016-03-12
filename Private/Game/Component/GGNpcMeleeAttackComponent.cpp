// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcMeleeAttackComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Actor/GGCharacter.h"

// Sets default values for this component's properties
UGGNpcMeleeAttackComponent::UGGNpcMeleeAttackComponent()
{		
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = false;
	bWantsInitializeComponent = true;
}

void UGGNpcMeleeAttackComponent::InitializeComponent()
{
	Super::InitializeComponent();
	for (auto& hitbox : AttackHitbox_0)
	{
		hitbox.HitShapeInternal = hitbox.AttackShape.ConvertToEngineShape();
	}
	SetActive(false);
}

void UGGNpcMeleeAttackComponent::UseAttack(uint8 InInstructionID)
{
	InstructionId = InInstructionID;
	CurrentTimeStamp = 0.f;
	FullAttackLength = 0.f;
	if (InstructionId == 0)
	{
		for (auto& hitbox : AttackHitbox_0)
		{
			FullAttackLength = FMath::Max(FullAttackLength, hitbox.End);
		}
	}
	SetActive(true);
}

void UGGNpcMeleeAttackComponent::FinishAttack()
{
	AffectedEntities.Reset(false);
	SetActive(false);
	SetComponentTickEnabled(false);
}

// Called every frame
void UGGNpcMeleeAttackComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
	
	AGGMinionBase* locOwner = static_cast<AGGMinionBase*>(GetOwner());
	if (InstructionId == 0 && locOwner)
	{
		FVector locOwnerLocation = locOwner->GetActorLocation();
		FVector locOwnerDirection = locOwner->GetPlanarForwardVector();
		for (auto& hitbox : AttackHitbox_0)
		{
			if (
				(CurrentTimeStamp > hitbox.Begin && CurrentTimeStamp < hitbox.End) ||
				(CurrentTimeStamp < hitbox.Begin && CurrentTimeStamp + DeltaTime > hitbox.End)
				)
			{
				FVector loc_Direction = FVector(1.f, locOwnerDirection.Y, 1.f);
				FVector loc_TraceCentre = locOwnerLocation + hitbox.HitboxCentre * loc_Direction;
				int32 Length = AffectedEntities.Num();
				if (UGGFunctionLibrary::WorldOverlapMultiActorByChannel(
					GetWorld(), loc_TraceCentre, DamageChannel, hitbox.HitShapeInternal, AffectedEntities))
				{
					int32 NewLength = AffectedEntities.Num();
					for (int32 i = Length; i < NewLength; i++)
					{
						// Hit code
						AGGCharacter* character = Cast<AGGCharacter>(AffectedEntities[i]);
						if (character && character->IsLocallyControlled())
						{
							FGGDamageReceivingInfo locInfo;
							locInfo.ImpactDirection = FGGDamageReceivingInfo::ConvertDeltaPosition(
								character->GetActorLocation() - loc_TraceCentre);
							locInfo.Type = hitbox.Type;
							locInfo.Direct_BaseMultiplier = hitbox.DirectDamageBase + DirectWeaponDamageBase;
							locInfo.Indirect_BaseMultiplier = hitbox.IndirectDamageBase + IndirectWeaponDamageBase;
							character->LocalReceiveDamage(locInfo);
						}
					}
				}
			}
		}
	}
	CurrentTimeStamp += DeltaTime;
	if (CurrentTimeStamp > FullAttackLength)
	{
		FinishAttack();
	}
}

