// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/Implementation/GGSlashAttackComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Actor/GGMinionBase.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

void UGGSlashAttackComponent::InitializeComponent()
{
	Super::InitializeComponent();
	for (auto a : GroundNormalAttacks)
	{
		if (a) a->RecalculateCaches();		
	}
	for (auto a : GroundMovingAttacks)
	{
		if (a) a->RecalculateCaches();
	}
	if (GroundChargedAttack) GroundChargedAttack->RecalculateCaches();
	for (auto a : AirNormalAttacks)
	{
		if (a) a->RecalculateCaches();
	}
	if (AirChargedAttack) 
	{
		AirChargedAttack->RecalculateCaches();
	}
}

void UGGSlashAttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UGGSlashAttackComponent, bModeChargedAttack, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UGGSlashAttackComponent, bModeChargedAttack_Queued, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(UGGSlashAttackComponent, bModeMobileAttack, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UGGSlashAttackComponent, bModeMobileAttack_Queued, COND_SkipOwner);
}

void UGGSlashAttackComponent::LocalAttemptsAttack(bool InIsCharged, bool InIsMobile)
{
	if (NextAttackIndex != 0 && !InIsCharged && !bQueuedAttack)
	{
		// Condition for when a combo is applicable i.e. instruction is of same category as current chain of attack
		if (bModeMobileAttack == InIsMobile)
		{
			// check TimeStamp data allows us to chain
			UGGMeleeAttackData* loc_LastInitiatedAttack = LastInitiatedAttack.Get();
			if (loc_LastInitiatedAttack && loc_LastInitiatedAttack->IsInComboWindow(CurrentTimeStamp))
			{
				// can chain on				
				LocalInitiateAttack(GetEncryptedAttackIdentifier(InIsCharged, InIsMobile));
			}
			// no else, do nothing if we can't chain
		}
	}
	else if (!LastInitiatedAttack.IsValid())
	{
		// use charged or is a first attack in combo if LastInitiatedAttack is null (meaning it has been finalized in tick)
		LocalInitiateAttack(GetEncryptedAttackIdentifier(InIsCharged, InIsMobile));
	}
}

void UGGSlashAttackComponent::HitTarget(const FMeleeHitNotify& InHitNotify)
{
	// base class method deals with the damage dealing logic
	Super::HitTarget(InHitNotify);
	// subclass overrides primarily for additional visual effects that should be played, note function is called only on causer client and the server		
}

void UGGSlashAttackComponent::PushAttackRequest()
{
	// TODO: add exit condition for data resets so as to not invoke invalid attacks 
	// server handle the receipt of instructions
	if (LastInitiatedAttack.IsValid())
	{
		// case queued attack
		if (GetOwnerRole() > ROLE_SimulatedProxy)
		{
			bool locCharged;
			bool locMobile;
			DecryptAttackIdentifier(AttackIdentifier, locCharged, locMobile);
			bModeChargedAttack_Queued = locCharged ? 1 : 0;
			bModeMobileAttack_Queued = locMobile ? 1 : 0;
			// trigger the repnotify
			bAttackToggle = !bAttackToggle;
		}
		bQueuedAttack = true;
	}
	else
	{
		if (GetOwnerRole() > ROLE_SimulatedProxy)
		{
			// case new attack	
			bool locCharged;
			bool locMobile;
			DecryptAttackIdentifier(AttackIdentifier, locCharged, locMobile);
			bModeChargedAttack = locCharged;
			bModeMobileAttack = locMobile;
			// trigger the repnotify
			bAttackToggle = !bAttackToggle;
		}				
		InitiateAttack();
	}
}

void UGGSlashAttackComponent::InitiateAttack()
{
	LastInitiatedAttack = GetAttackToUse();

	UGGMeleeAttackData* AttackData = LastInitiatedAttack.Get();
	if (AttackData)
	{
		AffectedEntities.Reset();
		// initialize state for attack
		CurrentTimeStamp = 0.f;
		SetComponentTickEnabled(true);
		// apply attack modifiers to character
		if (AttackData->IsRootingMove())
		{
			SetControllerIgnoreMoveInput();
		}
		NextAttackIndex++;
	}
	// broadcasts delegates
	Super::InitiateAttack();
}

void UGGSlashAttackComponent::FinalizeAttack()
{
	// Cleanup if nothing follows
	if (!bQueuedAttack)
	{
		LastInitiatedAttack.Reset();
		NextAttackIndex = 0;		
	}
	else
	{
		UE_LOG(GGWarning, Warning, TEXT("Called FinalizeAttack while bQueuedAttack is true => early exit from tick failed"));
		UseQueuedAttack();
	}
	Super::FinalizeAttack();
}

void UGGSlashAttackComponent::UseQueuedAttack()
{	
	InitiateAttack();
	// set Queue flag here, so that InitializeAttack knows we are invoking a queued attack before resetting
	bQueuedAttack = false;
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
		if (bIsLocalInstruction && UpdatedAttack->IsInActivePhase(CurrentTimeStamp))
		{
			// do hit checks, also counts cases where we lagged long so as to not skip the hit checks
			int32 Length = AffectedEntities.Num();
			// obtain required data to do our hit box query
			const FGGMeleeHitDefinition* loc_Definition = (UpdatedAttack->GetActiveDefinition(CurrentTimeStamp));
			if (loc_Definition)
			{
				AGGCharacter* loc_Character = static_cast<AGGCharacter*>(GetOwner());
				FVector loc_Direction = FVector(1.f, loc_Character ? loc_Character->GetPlanarForwardVector().Y : 1.f, 1.f);
				FVector loc_TraceCentre = GetOwner()->GetActorLocation() + loc_Definition->HitboxCentre * loc_Direction;

				if (UGGFunctionLibrary::WorldOverlapMultiActorByChannel(
					GetWorld(), loc_TraceCentre, DamageChannel, loc_Definition->AttackShapeInternal, AffectedEntities))
				{
					int32 NewLength = AffectedEntities.Num();
					for (int32 i = Length; i < NewLength; i++)
					{
						FMeleeHitNotify notifier;
						notifier.Target = AffectedEntities[i];
						notifier.DamageCategory = UpdatedAttack->GetDamageType();
						notifier.DamageBaseMultipliers = FMeleeHitNotify ::CompressedDamageLevels(
							DirectWeaponDamageBase + UpdatedAttack->GetDirectionDamageBase(), 
							IndirectWeaponDamageBase + UpdatedAttack->GetIndirectionDamageBase());
						LocalHitTarget(notifier);
					}
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
		else if (bQueuedAttack && UpdatedAttack->HasPastHardCooldown(CurrentTimeStamp))
		{
			// use queued attack
			UseQueuedAttack();
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
		SetComponentTickEnabled(false);
	}
}

const int32 BIT_CHARGED_GGSLASh = 2;
const int32 BIT_MOVING_GGSLASH = 4;
uint8 UGGSlashAttackComponent::GetEncryptedAttackIdentifier(bool InIsCharged, bool InIsMobile) const
{
	uint8 Index = 0;
	if (InIsCharged)
	{
		Index |= BIT_CHARGED_GGSLASh;
	}
	if (InIsMobile)
	{
		Index |= BIT_MOVING_GGSLASH;
	}	
	return Index;
}

void UGGSlashAttackComponent::DecryptAttackIdentifier(const uint8 InIdentifier, bool & OutIsCharged, bool & OutIsMobile)
{
	OutIsCharged = (InIdentifier & BIT_CHARGED_GGSLASh) != 0;
	OutIsMobile = (InIdentifier & BIT_MOVING_GGSLASH) != 0;
}

bool UGGSlashAttackComponent::GetOwnerGroundState() const
{
	UCharacterMovementComponent* loc_CMC = OwnerMovement.Get();
	if (loc_CMC == nullptr)
	{
		AGGCharacter* loc_Character = static_cast<AGGCharacter*>(GetOwner());
		if (loc_Character)
		{
			loc_CMC = loc_Character->GetCharacterMovement();
		}
		if (loc_CMC)
		{
			return  loc_CMC->IsMovingOnGround();
		}
	}	
	else
	{
		return  loc_CMC->IsMovingOnGround();
	}
	UE_LOG(GGWarning, Warning, TEXT("SlashAtkComponent has no owner / charactermovement component in owner heirarchy"));		
	// always return true if not charactermovement 
	return true;
}

UGGMeleeAttackData* UGGSlashAttackComponent::GetAttackToUse()
{	
	bool loc_Charged = bQueuedAttack ? bModeChargedAttack_Queued : bModeChargedAttack;
	bool loc_Mobile = bQueuedAttack ? bModeMobileAttack_Queued : bModeMobileAttack;
	bModeAerialAttack = !GetOwnerGroundState();
	if (loc_Charged)
	{
		return bModeAerialAttack ? AirChargedAttack : GroundChargedAttack;
	}
	else if (bModeAerialAttack)
	{
		if (AirNormalAttacks.IsValidIndex(NextAttackIndex))
		{
			return AirNormalAttacks[NextAttackIndex];
		}
		UE_LOG(GGWarning, Warning, TEXT("No valid air attack found from NextAttackIndex"));		
	}
	else
	{
		if (loc_Mobile && GroundMovingAttacks.Num() > 0 && GroundMovingAttacks.IsValidIndex(NextAttackIndex))
		{
			return GroundMovingAttacks[NextAttackIndex];
		}
		else if (GroundNormalAttacks.Num() > 0 && GroundNormalAttacks.IsValidIndex(NextAttackIndex))
		{
			return GroundNormalAttacks[NextAttackIndex];
		}
		UE_LOG(GGWarning, Warning, TEXT("No valid ground attack found from NextAttackIndex"));
	}
	return nullptr;
}

bool UGGSlashAttackComponent::ShouldRemainInState() const
{
	if (GetOwner() == nullptr)
	{
		UE_LOG(GGMessage, Log, TEXT("Should not remain in attack state"));
		return false;
	}
	
	if (bModeMobileAttack)
	{
		FVector loc_Velocity = GetOwner()->GetVelocity();
		if (FMath::Abs(loc_Velocity.Y) < 50.f)
		{
			UE_LOG(GGMessage, Log, TEXT("Should not remain in attack state"));
			return false;
		}
	}
	if (bModeAerialAttack)
	{
		AGGCharacter* loc_Character = static_cast<AGGCharacter*>(GetOwner());
		bool loc_onGround = loc_Character && loc_Character->GetCharacterMovement() && loc_Character->GetCharacterMovement()->IsMovingOnGround();
		if (loc_onGround)
		{
			UE_LOG(GGMessage, Log, TEXT("Should not remain in attack state"));
			return false;
		}
	}
	return true;
}

UPaperFlipbook* UGGSlashAttackComponent::GetCurrentAnimation() const
{
	UGGMeleeAttackData* AttackData = LastInitiatedAttack.Get();
	if (AttackData)
	{
		return AttackData->GetAttackAnimation();
	}
	return nullptr;
}

UPaperFlipbook* UGGSlashAttackComponent::GetEffectAnimation() const
{
	UGGMeleeAttackData* AttackData = LastInitiatedAttack.Get();
	if (AttackData)
	{
		return AttackData->GetEffectAnimation();
	}
	return nullptr;
}