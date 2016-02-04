// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Game/Actor/GGMinionBase.h"
#include "GGShooterMinion.generated.h"

/**
 * 
 */
UCLASS()
class GG_API AGGShooterMinion : public AGGMinionBase
{
	GENERATED_BODY()

public:
    virtual void TickPatrol() override;

    /** Attack preparation information */
    FVector2D AttackRange;
    virtual void TickPrepareAttack() override;
    
    /** Evasion phase information*/
    FVector2D EvadeRange;
    virtual void TickEvade() override;
	
    bool IsTargetInSuppliedRange(const FVector2D& Range) const;
    
    bool IsFacingTarget() const;
    
};
