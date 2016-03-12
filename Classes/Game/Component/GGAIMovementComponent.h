// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/MovementComponent.h"
#include "GGAIMovementComponent.generated.h"

/**
 * 
 */
class AGGMinionBase;
UCLASS(Blueprintable, ClassGroup="GG|Movement", meta=(BlueprintSpawnableComponent))
class GG_API UGGAIMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	
private:
    UPROPERTY(Category="GGAI|General", EditDefaultsOnly)
    float MaxAccelerationY;
    UPROPERTY(Category="GGAI|General", EditDefaultsOnly)
    float MaxAccelerationZ;
	UPROPERTY(Category = "GGAI|General", EditDefaultsOnly)
		uint32 bUseGradualAcceleration : 1;
	UPROPERTY(Category = "GGAI|General", EditDefaultsOnly, meta = (DisplayName = "Initial movement mode"))
		TEnumAsByte<EMovementMode> MovementMode;

    UPROPERTY(Category="GGAI|Pedals", EditDefaultsOnly)
		float WalkSpeed;
    UPROPERTY(Category="GGAI|Pedals", EditDefaultsOnly)
		float JumpSpeed;
    UPROPERTY(Category="GGAI|Pedals", EditDefaultsOnly)
		float TerminalFallSpeed;
    UPROPERTY(Category="GGAI|Pedals", EditDefaultsOnly)
		float GravityScale;
	
	UPROPERTY(Category = "GGAI|Rotors", EditDefaultsOnly)
		float FlySpeed;
public:
	UPROPERTY(Category="GGAI|General", EditDefaultsOnly)
		TEnumAsByte<ECollisionChannel> SteppingChannel;
	UPROPERTY(Category = "GGAI|General", EditDefaultsOnly)
		TEnumAsByte<ECollisionChannel> PlatformChannel;	

    UGGAIMovementComponent();
    
    AGGMinionBase* MinionOwner;
	
    FVector Acceleration;
    // used to ensure we follow the movement base whenever possible
    FVector OldBaseLocation;
    // used for ground check
	FCollisionQueryParams GroundQueryParams;
    
protected:
    virtual bool HasValidData();
    
    UFUNCTION(BlueprintCallable, Category="GGAI|Movement")
    AGGMinionBase* GetMinionOwner() const;
    /** Before we do anything, follow the base first */
    virtual void SyncBaseMovement();
    
    /** Clamps Acceleration to MaxAcceleration*/
    virtual void ConfineAcceleration(FVector& OutAcceleration);
    
    virtual void CalcVelocity(FVector& OutVelocity, FVector& CurrentVelocity, const FVector& InAcceleration, float DeltaTime);
    
    virtual void CheckForGround(FHitResult &Result, ECollisionChannel Channel, float Direction) const;
    
    virtual void TickWalking(float DeltatTime);
    
    /** Adjust MoveDelta so that we don't fall down an edge / try to bump into a wall */
    virtual void ConfineWalkingMoveDelta(FVector& MoveDelta);
    
    /** Helper functions */
public:    
	// not const as we update movement mode if we found ground from falling
    bool IsMovingOnGround();

protected:
    virtual void TickFalling(float DeltaTime);
    
	virtual void TickFlying(float DeltaTime);

public:
    virtual void InitializeComponent() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetTravelDirection();
};

FORCEINLINE AGGMinionBase* UGGAIMovementComponent::GetMinionOwner() const
{
    return MinionOwner;
}