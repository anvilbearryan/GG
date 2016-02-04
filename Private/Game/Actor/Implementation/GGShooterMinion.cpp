// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGShooterMinion.h"
#include "Game/Actor/GGCharacter.h"


void AGGShooterMinion::TickPatrol()
{
    Super::TickPatrol();
    
}

void AGGShooterMinion::TickPrepareAttack()
{
    Super::TickPrepareAttack();
    
    bool IsInAttackRange = IsTargetInSuppliedRange(AttackRange);
    if (IsInAttackRange)
    {
        // we may need to do a turn first
        if (IsFacingTarget())
        {
            // can attack
        }
    }
}

void AGGShooterMinion::TickEvade()
{
    Super::TickEvade();
}

bool AGGShooterMinion::IsTargetInSuppliedRange(const FVector2D& Range) const
{
    if (Target == nullptr)
    {
        return false;
    }
    FVector TargetPosition = Target->GetActorLocation();
    FVector MyPosition = GetActorLocation();
    
    float dx = TargetPosition.Y - MyPosition.Y;
    float dy = TargetPosition.Z - MyPosition.Z;
    
    return Range.X > FMath::Abs(dx) && Range.Y > FMath::Abs(dy);
}

bool AGGShooterMinion::IsFacingTarget() const
{
    FVector Forward = GetPlanarForwardVector();
    FVector RelativePosition = Target->GetActorLocation() - GetActorLocation();
    return FVector::DotProduct(RelativePosition, Forward) > 0.f;
}
