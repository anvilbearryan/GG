// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGCharacterMovementComponent.h"
#include "Game/Actor/GGCharacter.h"

// MAGIC NUMBERS
const float VERTICAL_SLOPE_NORMAL_Z_CHARMOVEMENT = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.

bool UGGCharacterMovementComponent::IsTouchingWall(const FVector &InDirection)
{
	return IsTouchingWall(InDirection, MaxGapForWallJump);
}

bool UGGCharacterMovementComponent::IsTouchingWall(const FVector &InDirection, float Distance)
{
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);

	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	
	// Test with a box that is enclosed by the capsule.
	const float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FCollisionShape BoxShape = FCollisionShape::MakeBox(FVector(CapsuleRadius, CapsuleRadius, CapsuleHeight));

	FVector Start = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();// +InDirection * CapsuleRadius * 0.5f;
	FVector End = Start + InDirection * Distance;
	bool bBlockingHit = GetWorld()->SweepTestByChannel(Start, End, FQuat::Identity, CollisionChannel, BoxShape, QueryParams, ResponseParam);

	return bBlockingHit;
}

bool UGGCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	AGGCharacter* ggChar = static_cast<AGGCharacter*>(CharacterOwner);
	if (ggChar)
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			//	Vertical ignore dash
			Velocity.Z = JumpZVelocity;
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	return false;
}

bool UGGCharacterMovementComponent::DoWallJump()
{
	AGGCharacter* ggChar = static_cast<AGGCharacter*>(CharacterOwner);
	if (ggChar)
	{
		Velocity.Z = WallJumpZVelocity;
		//	TODO: make it nicer than this hardcode nudge
		if (ggChar->bPressedWallJumpLeft)
		{
			Velocity.Y = (ggChar->CanDash() ? DashedWallJumpLateralSpeed : WallJumpLateralSpeed) * -0.25f;
		}
		else if (ggChar->bPressedWallJumpRight)
		{
			Velocity.Y = (ggChar->CanDash() ? DashedWallJumpLateralSpeed : WallJumpLateralSpeed) * 0.25f;
		}		
		return true;
	}
	return false;
}

FVector UGGCharacterMovementComponent::NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const
{
	FVector Result = InitialVelocity;

	if (!Gravity.IsZero())
	{
		// Snippet copied from UCharacterMovementComponent
		Result += Gravity * DeltaTime;

		const FVector GravityDir = Gravity.GetSafeNormal();
		const float TerminalLimit = bWantsWallSlide ? 
			WallSlideSpeed : FMath::Abs(GetPhysicsVolume()->TerminalVelocity);

		// Don't exceed terminal velocity.
		if ((Result | GravityDir) > TerminalLimit)
		{
			Result = FVector::PointPlaneProject(Result, FVector::ZeroVector, GravityDir) + GravityDir * TerminalLimit;
		}
	}
	return Result;
}

void UGGCharacterMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{		
	AGGCharacter* ggChar = static_cast<AGGCharacter*>(CharacterOwner);
	if (ggChar)
	{
		if (deltaTime < MIN_TICK_TIME)
		{
			return;
		}
		/**
		* During PhysFalling, bWantsWallSlide may be set to true, AFTER calculating NewFallVelocity,
		* so whether we need to cleanup (L2) depends on last iteration's state which is saved in the beginning (L1)
		*/
		bool bShallCleanFlag = bWantsWallSlide;	// L1

		//	the input acceleration
		FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
		FallAcceleration.Z = 0.f;
        FVector SavedFallAcceleration = FallAcceleration;
		//	saving the wall jump enforced acceleration to avoid calculating it each substep
		bool bWallJumpLateralMode = ggChar->bModeWallJump;
		FVector WallJumpAcceleration = FVector::ZeroVector;
		if (ggChar->bPressedWallJumpLeft)
		{
			bWallJumpLateralMode = true;
			WallJumpAcceleration.Y = -1.f;
		}
		else if (ggChar->bPressedWallJumpRight)
		{
			bWallJumpLateralMode = true;
			WallJumpAcceleration.Y = 1.f;
		}
		else
		{
			bWallJumpLateralMode = false;
		}
		
		if (bWallJumpLateralMode)
		{
			WallJumpAcceleration = ScaleInputAcceleration(WallJumpAcceleration);
			if (!HasAnimRootMotion() && WallJumpAcceleration.SizeSquared2D() > 0.f)
			{
				WallJumpAcceleration = GetAirControl(deltaTime, AirControl, WallJumpAcceleration);
				WallJumpAcceleration = WallJumpAcceleration.GetClampedToMaxSize(GetMaxAcceleration());
			}
			WallJumpAcceleration.Z = 0.f;
            FallAcceleration = WallJumpAcceleration;
		}

		const bool bHasAirControl = FallAcceleration.SizeSquared2D() > 0.f;

		float remainingTime = deltaTime;

		while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
		{
			Iterations++;
			const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
			remainingTime -= timeTick;

			const FVector OldLocation = UpdatedComponent->GetComponentLocation();
			const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
			bJustTeleported = false;

			RestorePreAdditiveRootMotionVelocity();

			FVector OldVelocity = Velocity;
			FVector VelocityNoAirControl = Velocity;
			//	Update our timestamp so we know when to switch back to input acceleration during a substep
			if (bWallJumpLateralMode)
			{
				ggChar->WallJumpLateralHoldTime += timeTick;
				if (!ggChar->CanWallJump())
				{
					bWallJumpLateralMode = false;
                    FallAcceleration = SavedFallAcceleration;
					UE_LOG(GGMessage, Log, TEXT("Wall jump finished"));
				}
				else
				{
					//UE_LOG(GGMessage, Log, TEXT("Wall jump ongoing"));
				}
			}
			else if (ggChar->bModeWallJump)
			{
				GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::Red, TEXT("Problem...."));
			}

			// Apply input
			if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
			{
				// Compute VelocityNoAirControl
				if (bHasAirControl)
				{
					// Find velocity *without* acceleration.
					TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
					TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
					Velocity.Z = 0.f;
					CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
					VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
				}
				// Compute Velocity
                {
                    // Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
                    TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
                    Velocity.Z = 0.f;
                    CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
                    Velocity.Z = OldVelocity.Z;
                }
                
                // Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
                if (!bHasAirControl)
                {
                    VelocityNoAirControl = Velocity;
                }
			}
			/**
			***************************************************************************************************
			* We modify how graivty takes role in PhysFalling below, sub-iterations no longer lower jump
			* speed from gravity and instead increases the JumpKeyHoldTime in Character when our character
			* is providing jump force as indicated in IsJumpProvidingForce().  
			* Note IsJumpProvidingForce() checks bPressedWallJumpL/R in the AGGCharacter to see which JumpHoldTime.
			* The advantage of such is more consistent jump height in lower framerates (previously less)
			***************************************************************************************************
			*/
			if (ggChar->IsJumpProvidingForce())
			{
				//	Don't apply gravity if Jump is still providing force
				ggChar->JumpKeyHoldTime += timeTick;
			}
			else
			{
				// Apply gravity
				const FVector Gravity(0.f, 0.f, GetGravityZ());
				Velocity = NewFallVelocity(Velocity, Gravity, timeTick);
				VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, timeTick);
			}

			const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;

			ApplyRootMotionToVelocity(timeTick);

			if (bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f))
			{
				// Just passed jump apex since now going down
				bNotifyApex = false;
				NotifyJumpApex();
			}

			// Move
			FHitResult Hit(1.f);
			FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
			SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

			if (!HasValidData())
			{
				return;
			}

			float LastMoveTimeSlice = timeTick;
			float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

			if (IsSwimming()) //just entered water
			{
				remainingTime += subTimeTickRemaining;
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
			else if (Hit.bBlockingHit)
			{
				if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
				{
					remainingTime += subTimeTickRemaining;
					ProcessLanded(Hit, remainingTime, Iterations);
					return;
				}
				else
				{
					// Compute impact deflection based on final velocity, not integration step.
					// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
					Adjusted = Velocity * timeTick;

					// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
					if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
					{
						const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
						FFindFloorResult FloorResult;
						FindFloor(PawnLocation, FloorResult, false);
						if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
						{
							remainingTime += subTimeTickRemaining;
							ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
							return;
						}
					}

					HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

					// If we've changed physics mode, abort.
					if (!HasValidData() || !IsFalling())
					{
						return;
					}

					// Limit air control based on what we hit.
					// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
					if (bHasAirControl)
					{
						const bool bCheckLandingSpot = false; // we already checked above.
						const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
						Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
					}

					const FVector OldHitNormal = Hit.Normal;
					const FVector OldHitImpactNormal = Hit.ImpactNormal;
					FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

					// Compute velocity after deflection (only gravity component for RootMotion)
					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
					{
						const FVector NewVelocity = (Delta / subTimeTickRemaining);
						Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
					}

					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
					{
						// Move in deflected direction.
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

						if (Hit.bBlockingHit)
						{
							// hit second wall
							LastMoveTimeSlice = subTimeTickRemaining;
							subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

							if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
							{
								remainingTime += subTimeTickRemaining;
								ProcessLanded(Hit, remainingTime, Iterations);
								return;
							}

							HandleImpact(Hit, LastMoveTimeSlice, Delta);

							// If we've changed physics mode, abort.
							if (!HasValidData() || !IsFalling())
							{
								return;
							}

							// Act as if there was no air control on the last move when computing new deflection.
							if (bHasAirControl && Hit.Normal.Z > VERTICAL_SLOPE_NORMAL_Z_CHARMOVEMENT)
							{
								const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
								Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
							}

							FVector PreTwoWallDelta = Delta;
							TwoWallAdjust(Delta, Hit, OldHitNormal);

							// Limit air control, but allow a slide along the second wall.
							if (bHasAirControl)
							{
								const bool bCheckLandingSpot = false; // we already checked above.
								const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

								// Only allow if not back in to first wall
								if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
								{
									Delta += (AirControlDeltaV * subTimeTickRemaining);
								}
							}

							// Compute velocity after deflection (only gravity component for RootMotion)
							if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
							{
								const FVector NewVelocity = (Delta / subTimeTickRemaining);
								Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
							}

							// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
							bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
							SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							if (Hit.Time == 0.f)
							{
								// if we are stuck then try to side step
								FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
								if (SideDelta.IsNearlyZero())
								{
									SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
								}
								SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
							}

							if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
							{
								remainingTime = 0.f;
								ProcessLanded(Hit, remainingTime, Iterations);
								return;
							}
							else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
							{
								// We might be in a virtual 'ditch' within our perch radius. This is rare.
								const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
								const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
								const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
								if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
								{
									Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
									Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
									Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
									Delta = Velocity * timeTick;
									SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
								}
							}
						}
					}
				}
			}

			if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
			{
				Velocity.X = 0.f;
				Velocity.Y = 0.f;
			}
		}
		/** L2:	Flag successfully kepts until NewFallVelocity gets to read it, now safe to cleanup */
		if (bShallCleanFlag)
		{
			bWantsWallSlide = false;
		}
	}
	else
	{
		//	No custom character, do super
		Super::PhysFalling(deltaTime, Iterations);
	}		
}

void UGGCharacterMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{	
	//	Checks if we want to wall slide
	if (IsFalling())
	{
		//	IsFalling() does not mean Z velocity < 0, manual check
		if (Velocity.Z < 0.f && Acceleration.Y * Hit.ImpactNormal.Y < 0.f)
		{			
			bWantsWallSlide = 1;
		}		
	}
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}

float UGGCharacterMovementComponent::GetMaxSpeed() const
{
	AGGCharacter* ggChar = static_cast<AGGCharacter*>(CharacterOwner);
	if (ggChar)
	{
		switch (MovementMode)
		{
		case MOVE_Walking:
		case MOVE_NavWalking:
			return ggChar->CanDash() ? MaxWalkSpeedCrouched : MaxWalkSpeed;
		case MOVE_Falling:
			return ggChar->CanDash() ? MaxWalkSpeedCrouched : MaxWalkSpeed;
		case MOVE_Swimming:
			return MaxSwimSpeed;
		case MOVE_Flying:
			return MaxFlySpeed;
		case MOVE_Custom:
			return MaxCustomMovementSpeed;
		case MOVE_None:
		default:
			return 0.f;
		}
	}
	return Super::GetMaxSpeed();
}

void UGGCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)	// Remote only
{
	if (!CharacterOwner)
	{
		return;
	}
	//	Cache information from flag first
	const bool TmpbWantsJump = ((Flags & FSavedMove_Character::FLAG_JumpPressed) != 0);;
	const bool TmpbWantsCrouch = ((Flags & FSavedMove_Character::FLAG_WantsToCrouch) != 0);
	const bool TmpbPressedWallJumpLeft = ((Flags & FSavedMove_Character::FLAG_Reserved_1) != 0);
	const bool TmpbPressedWallJumpRight = ((Flags & FSavedMove_Character::FLAG_Reserved_2) != 0);
	const bool TmpbPerformDashedAction = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);

	AGGCharacter* GGCharacterOwner = static_cast<AGGCharacter*>(CharacterOwner);
	
	//	Update jump mode
	if (!(GGCharacterOwner->bPressedJump && GGCharacterOwner->bPressedWallJumpLeft 
		&& GGCharacterOwner->bPressedWallJumpRight))
	{
		// We were in a state of doing nothing
		if (TmpbWantsJump && !TmpbPressedWallJumpLeft && !TmpbPressedWallJumpLeft)
		{
			// First detection of jump
			GGCharacterOwner->bModeWallJump = false;
		}
	}
	if (TmpbPressedWallJumpLeft || TmpbPressedWallJumpRight)
	{
		// At anytime we detect a wall jump being initiated
		GGCharacterOwner->bModeWallJump = true;
	}

	const bool bWasJumping = CharacterOwner->bPressedJump;
	// Reset JumpKeyHoldTime when player presses Jump key on server as well.
	if (!bWasJumping && TmpbWantsJump)
	{
		GGCharacterOwner->JumpKeyHoldTime = 0.0f;
	}
	
	const bool bWasWallJumping = GGCharacterOwner->bModeWallJump
		&& (GGCharacterOwner->bPressedWallJumpLeft || GGCharacterOwner->bPressedWallJumpRight);
	// Reset WallJumpLateralHoldTime when a new press occured
	if (!bWasWallJumping && (TmpbPressedWallJumpLeft || TmpbPressedWallJumpRight))
	{
		GGCharacterOwner->WallJumpLateralHoldTime = 0.f;
	}

	const bool bWasPerformingDashedAction = GGCharacterOwner->bPerformDashedAction;
	if (!bWasPerformingDashedAction && TmpbPerformDashedAction)
	{
		GGCharacterOwner->TimeDashedFor = 0.f;
	}
	// Update states from cached information
	CharacterOwner->bPressedJump = TmpbWantsJump;
	bWantsToCrouch = TmpbWantsCrouch;
	GGCharacterOwner->bPressedWallJumpLeft = TmpbPressedWallJumpLeft;
	GGCharacterOwner->bPressedWallJumpRight = TmpbPressedWallJumpRight;	
	GGCharacterOwner->bPerformDashedAction = TmpbPerformDashedAction;
}

class FNetworkPredictionData_Client* UGGCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	check(PawnOwner->Role < ROLE_Authority);

	if (!ClientPredictionData)
	{
		UGGCharacterMovementComponent* MutableThis = const_cast<UGGCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_GGCharacter(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

//	=========================================
//	FSavedMove_GGCharacter implementation

void FSavedMove_GGCharacter::Clear()
{
	Super::Clear();
	bPressedWallJumpRight = 0;
	bPressedWallJumpLeft = 0;
	bPerformDashedAction = 0;
}

uint8 FSavedMove_GGCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bPressedWallJumpLeft)
	{
		Result |= FLAG_Reserved_1;
	}
	if (bPressedWallJumpRight)
	{
		Result |= FLAG_Reserved_2;
	}
	if (bPerformDashedAction)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool FSavedMove_GGCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{	
	FSavedMove_GGCharacter* move = static_cast<FSavedMove_GGCharacter*>(NewMove.Get());
	
	return Super::CanCombineWith(NewMove, Character, MaxDelta) 
		&& bPressedWallJumpRight == move->bPressedWallJumpRight
		&& bPressedWallJumpLeft == move->bPressedWallJumpLeft
		&& bPerformDashedAction == move->bPerformDashedAction;
}

void FSavedMove_GGCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	AGGCharacter* GGCharacter = static_cast<AGGCharacter*>(Character);
	if (GGCharacter)
	{
		bPressedWallJumpLeft = GGCharacter->bPressedWallJumpLeft;
		bPressedWallJumpRight = GGCharacter->bPressedWallJumpRight;
		bPerformDashedAction = GGCharacter->bPerformDashedAction;
	}
}

void FSavedMove_GGCharacter::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	/*
	UGGCharacterMovementComponent* moveComp = static_cast<UGGCharacterMovementComponent*>(Character->GetCharacterMovement());
	if (moveComp)
	{
		moveComp->bWantsToWallJumpLeft = bWantsToWallJumpLeft;
		moveComp->bWantsToWallJumpRight = bWantsToWallJumpRight;
	}
	*/
}

//	=========================================
//	FNetworkPredictionData_Client_GGCharacter implementation

FSavedMovePtr FNetworkPredictionData_Client_GGCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_GGCharacter());
}