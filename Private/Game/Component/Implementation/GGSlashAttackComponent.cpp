// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/Implementation/GGSlashAttackComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Utility/GGFunctionLibrary.h"

void UGGSlashAttackComponent::LocalAttemptsAttack(bool InIsOnGround, bool InIsCharged, bool InIsMoving)
{
	if (PendingAttackIndex != 0 && !InIsCharged && !bQueuedAttack)
	{
		// convert local uint32 state to bools
		bool loc_bUsingAirAttack = bUsingAirAttack;
		bool loc_bUsingMovingAttack = bUsingMovingAttack;
		// Condition for when a combo is applicable i.e. instruction is of same category as current chain of attack
		if (loc_bUsingAirAttack != InIsOnGround && loc_bUsingMovingAttack == InIsMoving)
		{
			// check TimeStamp data allows us to chain
			UGGMeleeAttackData* loc_LastInitiatedAttack = LastInitiatedAttack.Get();
			if (loc_LastInitiatedAttack && loc_LastInitiatedAttack->IsInComboWindow(CurrentTimeStamp))
			{
				// can chain on				
				if (loc_LastInitiatedAttack->HasPastHardCooldown(CurrentTimeStamp))
				{
					// do the transfer now
					LocalInitiateAttack(GetIndexFromAttackInformation(InIsOnGround, InIsCharged, InIsMoving));
				}
				else
				{
					float DelayToInitiate = loc_LastInitiatedAttack->TimeToLaunchCombo(CurrentTimeStamp);
					if (DelayToInitiate > 0.f)
					{	// set queue
						LocalQueuedAttackIdentifier = GetIndexFromAttackInformation(InIsOnGround, InIsCharged, InIsMoving);
						bQueuedAttack = true;
						GetWorld()->GetTimerManager().SetTimer(
							AttackQueueHandle, this, &UGGSlashAttackComponent::UseQueuedAttack,
							loc_LastInitiatedAttack->TimeToLaunchCombo(CurrentTimeStamp));
					}
					else
					{
						// this should only happened on wrongly setup attack data
						UE_LOG(GGWarning, Warning, TEXT("Please recheck AttackData asset, not in combo window but past hard cooldown"));
						LocalInitiateAttack(LocalQueuedAttackIdentifier);
					}
				}
			}
			// no else, do nothing if we can't chain
		} 		
	}
	else
	{
		// charged or first attack in combo
		LocalInitiateAttack(GetIndexFromAttackInformation(InIsOnGround, InIsCharged, InIsMoving));
	}	
}

void UGGSlashAttackComponent::UseQueuedAttack()
{
	bQueuedAttack = false;
	// with attack queued, not possible to clean up input ignoring normall in Finalize
	UGGMeleeAttackData* AttackData = LastInitiatedAttack.Get();
	if (AttackData)
	{
		// remove attack modifiers from last attack to character
		if (AttackData->IsRootingMove())
		{
			SetControllerReceiveMoveInput();
		}
	}
	LocalInitiateAttack(LocalQueuedAttackIdentifier);
}

const int32 BIT_CHARGED = 16;
const int32 BIT_ONGROUND = 32;
const int32 BIT_MOVING = 64;

void UGGSlashAttackComponent::InitiateAttack(uint8 Identifier)
{
	LastInitiatedAttack = GetAttackDataFromIndex(Identifier);
	
	if ( (Identifier & BIT_CHARGED) == 0)
	{
		// needs to increment attack index for next attack if not using a charged one
		const TArray<UGGMeleeAttackData*>* Chain = GetAttacksArrayFromIndex(Identifier);
		if (Chain)
		{		
#if WITH_EDITOR
			if (Chain->Num() == 0)
			{
				UE_LOG(GGWarning, Warning, TEXT("Requested attack arrary is empty"));
			}
#endif
			PendingAttackIndex = (PendingAttackIndex + 1) % Chain->Num();
		}
		else
		{
#if WITH_EDITOR
			UE_LOG(GGWarning, Warning, TEXT("Requested attack arrary is null"));
#endif
		}
	}
	bUsingAirAttack = (Identifier & BIT_ONGROUND) == 0;
	bUsingMovingAttack = (Identifier & BIT_MOVING) != 0;

	UGGMeleeAttackData* AttackData = LastInitiatedAttack.Get();
	if (AttackData)
	{
		// initialize state for attack
		CurrentTimeStamp = 0.f;
		SetActive(true);
		// apply attack modifiers to character
		if (AttackData->IsRootingMove())
		{
			SetControllerIgnoreMoveInput();
		}
	}
}

void UGGSlashAttackComponent::FinalizeAttack()
{	
	// Cleanup if nothing follows
	if (!bQueuedAttack)
	{
		LastInitiatedAttack.Reset();
		PendingAttackIndex = 0;		
	}
	SetActive(false);
}

void UGGSlashAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// if a valid attack is set
	UGGMeleeAttackData* UpdatedAttack = LastInitiatedAttack.Get();
	if (UpdatedAttack)
	{
		float loc_NextTimeStamp = CurrentTimeStamp + DeltaTime;
		// separate into stages of an attack
		if (UpdatedAttack->IsInActivePhase(CurrentTimeStamp) || UpdatedAttack->IsInActivePhase(DeltaTime))
		{
			// do hit checks, also counts cases where we lagged long so as to not skip the hit checks
			int32 Length = AffectedEntities.Num();
			// obtain required data to do our hit box query
			const FGGMeleeHitDefinition& loc_Definition = *(UpdatedAttack->GetActiveDefinition(CurrentTimeStamp));
			AGGCharacter* loc_Character = static_cast<AGGCharacter*>(GetOwner());			
			FVector loc_Direction = FVector(1.f, loc_Character ? loc_Character->GetPlanarForwardVector().Y : 1.f, 1.f);
			FVector loc_TraceCentre = GetOwner()->GetActorLocation() + loc_Definition.HitboxCentre * loc_Direction;
			
			if (UGGFunctionLibrary::WorldOverlapMultiActorByChannel(
				GetWorld(), loc_TraceCentre,
				DamageChannel, loc_Definition.AttackShapeInternal, AffectedEntities))
			{
				int32 NewLength = AffectedEntities.Num();
				for (int32 i = Length; i < NewLength; i++)
				{
					LocalHitTarget(AffectedEntities[i], CachedAttackIdentifier);
				}
			}
			// check if we are about to change specific attack phase, if so resets caches
			if (UpdatedAttack->ChangesAttackDefinition(CurrentTimeStamp, DeltaTime))
			{
				AffectedEntities.Reset();
			}
		}		
		else if (!UpdatedAttack->HasPastHardCooldown(CurrentTimeStamp) && UpdatedAttack->HasPastHardCooldown(loc_NextTimeStamp))
		{
			// release imposed movement modifiers, this block is only possible to be entered once per attack sequence				
			if (UpdatedAttack->IsRootingMove())
			{
				SetControllerReceiveMoveInput();
			}
		}
		else if (UpdatedAttack->HasPastSoftCooldown(CurrentTimeStamp))
		{
			FinalizeAttack();
		}
		// updates timestamp at end of tick
		CurrentTimeStamp = loc_NextTimeStamp;
	}
	else
	{
		// tick enabled mistakenly
		SetActive(false);
	}
}

void UGGSlashAttackComponent::HitTarget(AActor * target, uint8 Identifier)
{

}

uint8 UGGSlashAttackComponent::GetIndexFromAttackInformation(bool InIsOnGround, bool InIsCharged, bool InIsMoving) const
{
	uint8 Index = 0;
	if (InIsCharged) 
	{
		Index |= BIT_CHARGED;
	}
	if (InIsOnGround)
	{
		Index |= BIT_ONGROUND;
	}
	if (InIsMoving)
	{
		Index |= BIT_MOVING;
	}
	Index |= PendingAttackIndex;
	return Index;
}

UGGMeleeAttackData* UGGSlashAttackComponent::GetAttackDataFromIndex(uint8 Identifier) const
{
	int32 Index = Identifier;
	bool IsCharged= (Index & BIT_CHARGED) > 0;
	bool IsOnGround = (Index & BIT_ONGROUND) > 0;
	bool IsMoving = (Index & BIT_MOVING) > 0;

	const int32 BIT_ATTACKINDEX = ~(BIT_CHARGED | BIT_ONGROUND | BIT_MOVING);
	int32 loc_PendingAttackIndex = Index & BIT_ATTACKINDEX;
	
	if (IsCharged)
	{
		return IsOnGround ? GroundChargedAttack : AirChargedAttack;
	}
	else if (IsOnGround)
	{
		if (IsMoving && GroundMovingAttacks.Num() > 0 && GroundMovingAttacks.IsValidIndex(loc_PendingAttackIndex))
		{
			return GroundMovingAttacks[loc_PendingAttackIndex];
		}
		else if (GroundNormalAttacks.Num() > 0 && GroundNormalAttacks.IsValidIndex(loc_PendingAttackIndex))
		{
			return GroundNormalAttacks[loc_PendingAttackIndex];
		}						
		UE_LOG(GGWarning, Warning, TEXT("No valid ground attack found from PendingAttackIndex"));
	}
	else
	{
		if (AirNormalAttacks.IsValidIndex(loc_PendingAttackIndex))
		{
			return AirNormalAttacks[loc_PendingAttackIndex];
		}
		UE_LOG(GGWarning, Warning, TEXT("No valid air attack found from PendingAttackIndex"));
	}
	return nullptr;
}

const TArray<UGGMeleeAttackData*>* UGGSlashAttackComponent::GetAttacksArrayFromIndex(uint8 Identifier) const
{
	int32 Index = Identifier;
	bool IsOnGround = (Index & BIT_ONGROUND) > 0;
	bool IsMoving = (Index & BIT_MOVING) > 0;

	if (IsOnGround)
	{
		return IsMoving ? &GroundMovingAttacks : &GroundNormalAttacks;
	}
	else
	{
		return &AirNormalAttacks;
	}
}

