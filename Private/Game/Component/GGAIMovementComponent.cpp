// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGAIMovementComponent.h"
#include "Game/Actor/GGMinionBase.h"

bool UGGAIMovementComponent::HasValidData()
{
    return MinionOwner && UpdatedComponent;
}

void UGGAIMovementComponent::SyncBaseMovement()
{
    // can't follow a movement basse if we are not walking
    if (!HasValidData() || MovementMode != EMovementMode::MOVE_Walking)
    {
        return;
    }
    UPrimitiveComponent* Base = MinionOwner->BasePlatform.PlatformPrimitive;
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
    if (bUseGradualAcceleration)
    {
        OutVelocity = CurrentVelocity + InAcceleration * DeltaTime;
        if (InAcceleration.Y != 0.f)
        {
            OutVelocity.Y = FMath::Clamp(OutVelocity.Y, -WalkSpeed, WalkSpeed);
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
        OutVelocity.Z = FMath::Clamp(OutVelocity.Z, -TerminalFallSpeed, JumpSpeed);
    }
    else
    {
        //  resolve horizontal
        OutVelocity.Y = FMath::Sign(InAcceleration.Y) * WalkSpeed;
        //  resolve vertical
        float z = OutVelocity.Z;
        float az = InAcceleration.Z;
        if (z * az < 0.f)
        {
            OutVelocity.Z = 0.f;
        }
        else if (az > 0.f)
        {
            OutVelocity.Z = JumpSpeed;
        }
        else if (az < 0.f)
        {
            OutVelocity.Z = -TerminalFallSpeed;
        }
    }
}
float GROUND_CHECK_DELTA = 2.f;
void UGGAIMovementComponent::CheckForGround(TArray<FHitResult> &Hits)
{
    FVector Start = UpdatedComponent->GetComponentLocation();
    FVector End = Start;
    End.Z -= GROUND_CHECK_DELTA + MinionOwner->GetHalfHeight();
    GetWorld()->LineTraceMultiByObjectType(Hits, Start, End, GroundQueryParams);
}

void UGGAIMovementComponent::TickWalking(float DeltaTime)
{
    FVector NewVelocity = Velocity;
    CalcVelocity(NewVelocity, Velocity, Acceleration, DeltaTime);
    FVector MoveDelta = (NewVelocity + Velocity) * 0.5f * DeltaTime;
    ConfineWalkingMoveDelta(MoveDelta);
    
    FHitResult MoveResult;
    FQuat Quat = UpdatedComponent->GetComponentQuat();
    
    SafeMoveUpdatedComponent(MoveDelta, Quat, true, MoveResult);
    if (!MoveResult.bBlockingHit)
    {
        Velocity = NewVelocity;
    }
    else
    {
        // Handle collision
   
        Velocity = FVector::ZeroVector;
    }
}

void UGGAIMovementComponent::ConfineWalkingMoveDelta(FVector& MoveDelta)
{
    //  We want to limit the move delta to ensure we don't walk off ground
    if (!HasValidData())
    {
        return;
    }
    TArray<FHitResult> Hits;
    FVector Start = UpdatedComponent->GetComponentLocation();
    //  cast from centre of capsule to 2cm undeaneath
    Start.Y += FMath::Sign(MoveDelta.Y) * MinionOwner->GetHalfWidth();
    Start += MoveDelta;
    
    FVector End = Start;
    Start.Z += GROUND_CHECK_DELTA * 0.5f;
    End.Z -= GROUND_CHECK_DELTA + MinionOwner->GetHalfHeight();
    
    GetWorld()->LineTraceMultiByObjectType(Hits, Start, End, GroundQueryParams);
    if (Hits.Num() > 0)
    {
        // check normal to adjust movement direction
        // get ground height
        float dz = Hits[0].ImpactPoint.Z - End.Z - GROUND_CHECK_DELTA;
        // greater than a 3cm per second ascend at 60fps, should catch most
        if (dz >  0.05f)
        {
            //climb
            MoveDelta.Z += dz;
        }
        else if (dz < 0.05f)
        {
            // ditto for descend, not to worry for magnitude as dz is at least -2.f
            MoveDelta.Z -= dz;
        }
        // there is ground in destination, check
        UPrimitiveComponent* FoundBase = Hits[0].Component.Get();
        if (FoundBase && FoundBase != GetMinionOwner()->BasePlatform.PlatformPrimitive)
        {
            // need to set as new movement base
            GetMinionOwner()->SetMovementBase(FoundBase, this);
            OldBaseLocation = GetMinionOwner()->GetActorLocation() - FoundBase->GetComponentLocation();
        }
    }
    else
    {
        // lazy, lets just stop moving this frame
        MoveDelta = FVector::ZeroVector;
    }
}

bool UGGAIMovementComponent::IsMovingOnGround()
{
    return (UpdatedComponent && MovementMode == EMovementMode::MOVE_Walking);
}

void UGGAIMovementComponent::TickFalling(float DeltaTime)
{
    FVector NewVelocity = Velocity;
    if (Acceleration.Z <= 0.f)
    {
        Acceleration.Z = GetWorld()->GetDefaultGravityZ() * GravityScale;
    }
    CalcVelocity(NewVelocity, Velocity, Acceleration, DeltaTime);
    FVector MoveDelta = (NewVelocity + Velocity) * 0.5f * DeltaTime;
    
    FHitResult MoveResult;
    FQuat Quat = UpdatedComponent->GetComponentQuat();
    
    SafeMoveUpdatedComponent(MoveDelta, Quat, true, MoveResult);
    if (MoveResult.bBlockingHit)
    {
        //  Check whether we are landing
        bool bGoingDown = MoveDelta.Z;
        Velocity = FVector::ZeroVector;
        if (bGoingDown)
        {
            // set new base
            GetMinionOwner()->SetMovementBase(MoveResult.GetComponent(), this);
            // set movement mode to walking
            MovementMode = EMovementMode::MOVE_Walking;
            
            OldBaseLocation = GetMinionOwner()->GetActorLocation() - MoveResult.GetComponent()->GetComponentLocation();
        }
    }
    else if (MoveDelta.Z < 0.f)
    {
        // Check if we are landing on 1-way platforms
        FCollisionObjectQueryParams a;
        TArray<FHitResult> Hits;
        CheckForGround((Hits));
        if (Hits.Num() > 0 && Hits[0].bBlockingHit)
        {
            /** we hit a ground that didn't stop us from movement, need to nudge back up
            */
            float penetration = (1.f - Hits[0].Time) * GetMinionOwner()->GetHalfHeight();
            GetMinionOwner()->AddActorWorldOffset(FVector(0.f, 0.f, penetration));
            //set new base from hit result
            // set new base
            GetMinionOwner()->SetMovementBase(Hits[0].GetComponent(), this);
            // set movement mode to walking
            MovementMode = EMovementMode::MOVE_Walking;
            Velocity = FVector::ZeroVector;
            
            OldBaseLocation = GetMinionOwner()->GetActorLocation() - Hits[0].GetComponent()->GetComponentLocation();
        }
        else
        {
            Velocity = NewVelocity;
        }
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

void UGGAIMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!HasValidData())
    {
        return;
    }
    SyncBaseMovement();
    
    if (!Acceleration.IsZero())
    {
        ConfineAcceleration(Acceleration);
    }
    if (MovementMode == EMovementMode::MOVE_Walking)
    {
        TickWalking(DeltaTime);
    }
}