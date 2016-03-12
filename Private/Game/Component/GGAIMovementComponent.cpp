// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGAIMovementComponent.h"
#include "Game/Actor/GGMinionBase.h"

UGGAIMovementComponent::UGGAIMovementComponent()
{
    bWantsInitializeComponent = true;
}

bool UGGAIMovementComponent::HasValidData()
{
    return MinionOwner && UpdatedComponent && GetOwnerRole() == ROLE_Authority;
}

void UGGAIMovementComponent::SyncBaseMovement()
{
    // can't follow a movement basse if we are not walking
    if (!HasValidData() || MovementMode != EMovementMode::MOVE_Walking)
    {
        return;
    }
    UPrimitiveComponent* Base = GetMinionOwner()->BasePlatform.PlatformPrimitive;
    if (Base && Base->Mobility == EComponentMobility::Movable)
    {
        FVector NewBaseLocation = Base->GetComponentLocation();
        FVector DeltaLocation = NewBaseLocation - OldBaseLocation;
        // ignore X axis movement
        DeltaLocation.X = 0.f;
        if (DeltaLocation.GetAbsMax() < 0.5f)
        {
            //  Base moved, follow it
            FHitResult MoveOnBaseHit(1.f);
            MoveUpdatedComponent(DeltaLocation, UpdatedComponent->GetComponentQuat(), true, &MoveOnBaseHit);
            OldBaseLocation = NewBaseLocation;
            
            // TODO: Handle unsuccessful base moves
        }
    }
}

void UGGAIMovementComponent::ConfineAcceleration(FVector& OutAcceleration)
{
    //  this is a 2d side scrolling movement component
    OutAcceleration.X = 0.f;
    OutAcceleration.Normalize();
    OutAcceleration.Y = OutAcceleration.Y * MaxAccelerationY;
    OutAcceleration.Z = OutAcceleration.Z * MaxAccelerationZ;
}

void UGGAIMovementComponent::CalcVelocity(FVector& OutVelocity, FVector& CurrentVelocity, const FVector& InAcceleration, float DeltaTime)
{
    OutVelocity.X = 0.f;
	// Get max speed
	float YMax = 0.f;
	float ZMin = 0.f;
	float ZMax = 0.f;
	switch(MovementMode)
	{
		case EMovementMode::MOVE_Walking:
			YMax = WalkSpeed;
			break;
		case EMovementMode::MOVE_Falling:
			ZMin = -TerminalFallSpeed;
			ZMax = JumpSpeed;
			break;
		case EMovementMode::MOVE_Flying:
			YMax = FlySpeed * FMath::Abs(InAcceleration.Y / FMath::Max(0.f, 
				FMath::Abs(InAcceleration.Y) + FMath::Abs(InAcceleration.Z))				
				);
			ZMax = FMath::Abs(FlySpeed - YMax);
			ZMin = -ZMax;
			break;
	}
    
	if (bUseGradualAcceleration)
    {
        OutVelocity = CurrentVelocity + InAcceleration * DeltaTime;
        if (InAcceleration.Y != 0.f)
        {
            OutVelocity.Y = FMath::Clamp(OutVelocity.Y, -YMax, YMax);
        }
        else
        {
            // decelerate
            OutVelocity.Y -= FMath::Sign(OutVelocity.Y) * MaxAccelerationY * DeltaTime;
        }
        
        if (OutVelocity.Y * CurrentVelocity.Y < 0.f)
        {
            // change in horizontal direction, nullify
            OutVelocity.Y = 0.f;
        }
        OutVelocity.Z = FMath::Clamp(OutVelocity.Z, ZMin, ZMax);
    }
    else
    {
        //  resolve horizontal
        OutVelocity.Y = FMath::Sign(InAcceleration.Y) * YMax;
        //  resolve vertical
        float z = OutVelocity.Z;
        float az = InAcceleration.Z;
        if (z * az < 0.f)
        {
            OutVelocity.Z = 0.f;
        }
        else if (az > 0.f)
        {
            OutVelocity.Z = ZMax;
        }
        else if (az < 0.f)
        {
            OutVelocity.Z = ZMin;
        }
    }
}

float GROUND_CHECK_DELTA_AIMOVEMENT = 10.f;
void UGGAIMovementComponent::CheckForGround(FHitResult& Result, ECollisionChannel Channel, float Direction) const
{
    FVector Start = UpdatedComponent->GetComponentLocation();
    Start.Y = Start.Y + Direction * GetMinionOwner()->GetHalfWidth();
    FVector End = Start;
    End.Z = End.Z - GROUND_CHECK_DELTA_AIMOVEMENT - MinionOwner->GetHalfHeight();
    GetWorld()->LineTraceSingleByChannel(Result, Start, End, Channel, GroundQueryParams);
}

void UGGAIMovementComponent::TickWalking(float DeltaTime)
{
    FVector NewVelocity = Velocity;
    CalcVelocity(NewVelocity, Velocity, Acceleration, DeltaTime);
    FVector MoveDelta = (NewVelocity + Velocity) * 0.5f * DeltaTime;
	if (MoveDelta.IsZero())
	{
		return;
	}
    FHitResult MoveResult;
    FQuat Quat = UpdatedComponent->GetComponentQuat();
    SafeMoveUpdatedComponent(MoveDelta, Quat, true, MoveResult);
    if (MoveResult.IsValidBlockingHit())
    {
        // handled incline
        float slid = SlideAlongSurface(MoveDelta, 1.f - MoveResult.Time, MoveResult.Normal, MoveResult);
        Velocity = NewVelocity * slid;
        // update floor
        float TraceDirection = slid > 0 ? FMath::Sign(MoveDelta.Y) : 0.f;
		FHitResult StepResult;
        CheckForGround(StepResult, SteppingChannel, TraceDirection);
        if (StepResult.bBlockingHit)
        {
            GetMinionOwner()->SetMovementBase(StepResult.GetComponent(), this);
        }
        else
        {
            FHitResult PlatformResult;
            CheckForGround(PlatformResult, PlatformChannel, FMath::Sign(MoveDelta.Y));
            if (PlatformResult.bBlockingHit)
            {
                GetMinionOwner()->SetMovementBase(PlatformResult.GetComponent(), this);
            }
        }
		if (slid == 0.f)
		{
			//GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Cyan, TEXT("Walking: Totla blockage"));
			GetMinionOwner()->OnReachWalkingBound();
		}
    }
    else
    {
        Velocity = NewVelocity;
        // 2 possibility, flat surface or decline
        /** Strategy: move actor downwards and check distance moved, if it exceeds */
        FHitResult StepResult;
		CheckForGround(StepResult, SteppingChannel, 0.f); //FMath::Sign(MoveDelta.Y));
        if (StepResult.IsValidBlockingHit())
        {
            FHitResult ZPushResult;
            SafeMoveUpdatedComponent(FVector(0.f,0.f, -GROUND_CHECK_DELTA_AIMOVEMENT * 15.f * DeltaTime), Quat, true, ZPushResult);
            GetMinionOwner()->SetMovementBase(StepResult.GetComponent(), this);
        
            //GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Cyan, TEXT("Walking: has ground - Step channel"));
        }
        else
        {
            FHitResult PlatformResult;
            CheckForGround(PlatformResult, PlatformChannel, 0.f); //FMath::Sign(MoveDelta.Y));
            if (PlatformResult.bBlockingHit)
            {
                FHitResult ZPushResult;
                SafeMoveUpdatedComponent(FVector(0.f,0.f, -GROUND_CHECK_DELTA_AIMOVEMENT * 15.f * DeltaTime), Quat, true, ZPushResult);
                GetMinionOwner()->SetMovementBase(PlatformResult.GetComponent(), this);
                
                //GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Cyan, TEXT("Walking: has ground - Platform channel"));
            }
            else
            {
                // cliff, reverse the movement
                SafeMoveUpdatedComponent(-MoveDelta, Quat, true, MoveResult);
                GetMinionOwner()->OnReachWalkingBound();
                
                Velocity = FVector::ZeroVector;
                //GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Cyan, TEXT("Walking: NO ground"));
            }
        }
    }
}

void UGGAIMovementComponent::ConfineWalkingMoveDelta(FVector& MoveDelta)
{

}

bool UGGAIMovementComponent::IsMovingOnGround()
{
	if (UpdatedComponent)
	{
		if (MovementMode == EMovementMode::MOVE_Walking)
		{
			return true;
		}
		if (MovementMode == EMovementMode::MOVE_Falling)
		{
			FHitResult hit;
			CheckForGround(hit, SteppingChannel, 0.f);
			if (hit.bBlockingHit)
			{
				return true;
			}
			else
			{
				CheckForGround(hit, PlatformChannel, 0.f);
				if (hit.bBlockingHit)
				{
					MovementMode = EMovementMode::MOVE_Walking;
					return true;
				}
			}
		}
	}
	return false;
}

void UGGAIMovementComponent::TickFalling(float DeltaTime)
{
    FVector NewVelocity = Velocity;
    if (Acceleration.Z <= 0.f)
    {
        Acceleration.Z = -FMath::Abs(GetWorld()->GetDefaultGravityZ() * GravityScale);
    }
    CalcVelocity(NewVelocity, Velocity, Acceleration, DeltaTime);
    FVector MoveDelta = (NewVelocity + Velocity) * 0.5f * DeltaTime;
    
    FHitResult MoveResult;
    FQuat Quat = UpdatedComponent->GetComponentQuat();
    
    SafeMoveUpdatedComponent(MoveDelta, Quat, true, MoveResult);
    if (MoveResult.bBlockingHit)
    {
        //  Check whether we are landing
        bool bGoingDown = MoveDelta.Z != 0;
        Velocity = FVector::ZeroVector;
        if (bGoingDown)
        {
            // set new base
            GetMinionOwner()->SetMovementBase(MoveResult.GetComponent(), this);
            // set movement mode to walking
            MovementMode = EMovementMode::MOVE_Walking;
            Velocity.Z = 0;
            OldBaseLocation = MoveResult.GetComponent()->GetComponentLocation();
        }
        
        //GEngine->AddOnScreenDebugMessage(0, 1.5f, FColor::Cyan, TEXT("Falling: Initial blocking hit"));
    }
    else if (MoveDelta.Z < 0.f)
    {
        // Check if we are landing on 1-way platforms
        FHitResult Hit;
        CheckForGround(Hit, PlatformChannel, 0.f);

        if (Hit.bBlockingHit)
        {
            /** we hit a ground that didn't stop us from movement, need to nudge back up */            
            float halfheight = GetMinionOwner()->GetHalfHeight();
            float ZDelta = Hit.ImpactPoint.Z + halfheight + 0.1f - UpdatedComponent->GetComponentLocation().Z;
            GetMinionOwner()->AddActorWorldOffset(FVector(0.f, 0.f, ZDelta));
            // set new base
            GetMinionOwner()->SetMovementBase(Hit.GetComponent(), this);
            // set movement mode to walking
            MovementMode = EMovementMode::MOVE_Walking;
            Velocity = FVector::ZeroVector;
            OldBaseLocation = Hit.GetComponent()->GetComponentLocation();            
        }
        else
        {
            Velocity = NewVelocity;       
        }
    }
    else
    {
		Velocity = NewVelocity;
        GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Cyan, TEXT("Falling: Jumping"));
    }
}

void UGGAIMovementComponent::TickFlying(float DeltaTime)
{
	FVector NewVelocity = Velocity;	
	CalcVelocity(NewVelocity, Velocity, Acceleration, DeltaTime);
	FVector MoveDelta = (NewVelocity + Velocity) * 0.5f * DeltaTime;

	FHitResult MoveResult;
	FQuat Quat = UpdatedComponent->GetComponentQuat();
	SafeMoveUpdatedComponent(MoveDelta, Quat, true, MoveResult);

	if (MoveResult.bBlockingHit)
	{
		// we reach the end of walkable area
		GetMinionOwner()->OnReachWalkingBound();
		//SafeMoveUpdatedComponent(-MoveDelta, Quat, false, MoveResult);
	}
	else
	{
		Velocity = NewVelocity;
	}
}

void UGGAIMovementComponent::GetTravelDirection()
{
    if (!HasValidData())
    {
        Acceleration = FVector::ZeroVector;
    }
    else
    {
        Acceleration = GetMinionOwner()->TravelDirection;
    }
    
}

void UGGAIMovementComponent::InitializeComponent()
{
    Super::InitializeComponent();
    GroundQueryParams.AddIgnoredActor(GetOwner());
}

void UGGAIMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!HasValidData())
    {
        return;
    }
    SyncBaseMovement();
    GetTravelDirection();
    if (!Acceleration.IsZero())
    {
        ConfineAcceleration(Acceleration);
    }
	switch (MovementMode)
	{
		case EMovementMode::MOVE_Walking:
			TickWalking(DeltaTime);
			break;
		case EMovementMode::MOVE_Falling:
			TickFalling(DeltaTime);
			break;
		case EMovementMode::MOVE_Flying:
			TickFlying(DeltaTime);
			break;
	}

    UpdateComponentVelocity();
}