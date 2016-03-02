// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Game/Component/GGCharacterMovementComponent.h"
#include "PaperFlipbookComponent.h"

FName AGGCharacter::BodyFlipbookComponentName(TEXT("BodyFlipbookComponent"));
const FVector AGGCharacter::Right = FVector(0.f, 1.f, 0.f);
const FVector AGGCharacter::Left = FVector(0.f, -1.f, 0.f);

AGGCharacter::AGGCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UGGCharacterMovementComponent>
		(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;	
    
    BodyFlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(AGGCharacter::BodyFlipbookComponentName);
    if (RootComponent)
    {
        BodyFlipbookComponent->AttachTo(RootComponent);
    }
}

void AGGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGGCharacter, NormalWallJumpMaxHoldTimeLateral);
}

void AGGCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();   
}

void AGGCharacter::Tick( float DeltaTime )
{
    Super::Tick(DeltaTime);
	
	if (BodyFlipbookComponent && !bLockedFacing)
	{
		// turning sync
		float YVel = GetVelocity().Y;
		if (BodyFlipbookComponent->RelativeScale3D.X * YVel < 0.f)
		{
			BodyFlipbookComponent->SetRelativeScale3D(FVector(FMath::Sign(YVel), 1.f, 1.f));
		}
	}
}

//**************************

// ****		Input		****
void AGGCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);
	// bind movement and jump inputs
	check(InputComponent);
	InputComponent->BindAxis("MoveRight", this, &AGGCharacter::MoveRight);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AGGCharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &AGGCharacter::StopJumping);

	InputComponent->BindAction("Dash", IE_Pressed, this, &AGGCharacter::Dash);
	InputComponent->BindAction("Dash", IE_Released, this, &AGGCharacter::StopDashing);
}

void AGGCharacter::MoveRight(float AxisValue)
{
	if (bUseEnforcedMovement)
	{
		AddMovementInput(EnforcedMovementDirection, EnforcedMovementStrength);
	}
	else
	{
		AddMovementInput(Right, AxisValue);
	}
}

void AGGCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	//  We changes axis values from input based on wall jump condition
	if (ScaleValue != 0.f && WorldDirection != FVector::ZeroVector)
	{
		// cache non-zero input for possible retrievle
		LastActualMovementInput = WorldDirection * ScaleValue;
		if (CanWallJump())
		{
			if (bPressedWallJumpLeft)
			{
				ScaleValue = -1.f;
			}
			else if (bPressedWallJumpRight)
			{
				ScaleValue = 1.f;
			}
		}
	}
	else if (WallJumpLateralHoldTime < NormalWallJumpMinHoldTimeLateral)
	{
		if (bPressedWallJumpLeft)
		{
			ScaleValue = -1.f;
		}
		else if (bPressedWallJumpRight)
		{
			ScaleValue = 1.f;
		}
	}
	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
}

//**************************

// ****	(Wall) Jump		****
void AGGCharacter::Jump()
{
	if (!GetCharacterMovement() || bUseEnforcedMovement)
	{
		//	no character movement component
		return;
	}
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		Super::Jump();
		bModeWallJump = false;
	}
	else
	{	
		//	We are not on ground => falling
		UGGCharacterMovementComponent* mvc = static_cast<UGGCharacterMovementComponent*>(GetCharacterMovement());				
		if (mvc->IsTouchingWall(Left))
		{
			//	If dash key is down, we do a dashed wall jump, since its a new full move set TimeDashedFor = 0
			if (bIsDashKeyDown)
			{
				bPerformDashedAction = true;
				TimeDashedFor = 0.f;
			}
			bModeWallJump = true;
			//	wall jump towards right
			bPressedWallJumpRight = 1;
			bPressedWallJumpLeft = 0;
			WallJumpLateralHoldTime = 0.f;
			Super::Jump();
		}		
		else if (mvc->IsTouchingWall(Right))
		{
			//	ditto
			if (bIsDashKeyDown)
			{
				bPerformDashedAction = true;
				TimeDashedFor = 0.f;
			}
			bModeWallJump = true;
			//	wall jump towards left
			bPressedWallJumpLeft = 1;
			bPressedWallJumpRight = 0;
			WallJumpLateralHoldTime = 0.f;
			Super::Jump();
		}
	}	
}

void AGGCharacter::StopJumping()
{
	bPressedJump = false;
	bPressedWallJumpLeft = false;
	bPressedWallJumpRight = false;
	JumpKeyHoldTime = 0.0f;
	WallJumpLateralHoldTime = 0.f;
}

bool AGGCharacter::CanJumpInternal_Implementation() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}	
	const bool bCanHoldToJumpHigher = (GetJumpMaxHoldTime() > 0.0f) && IsJumpProvidingForce();
	
	return 
		//	Remove crouch prevention
		(bCanHoldToJumpHigher || GetCharacterMovement()->IsMovingOnGround())
		//	Whether we have actual jump capability 
		&& GetCharacterMovement()->IsJumpAllowed();
}

bool AGGCharacter::CanWallJump() const
{
	return CanWallJumpInternal();
}

bool AGGCharacter::CanWallJumpInternal_Implementation() const
{	
	return WallJumpLateralHoldTime < (bPerformDashedAction ? 
			GetDashWallJumpMaxHoldTimeLateral() : GetNormalWallJumpMaxHoldTimeLateral());
}

bool AGGCharacter::IsJumpProvidingForce() const
{
	return bPressedJump 
		&& JumpKeyHoldTime >= 0.0f 
		&& JumpKeyHoldTime < (bModeWallJump ? GetWallJumpMaxHoldTimeVertical() : GetJumpMaxHoldTime());
}

//**************************

// ****		Dash		****
void AGGCharacter::Dash()
{	
	if (bUseEnforcedMovement)
	{
		return;
	}
	bIsDashKeyDown = true;
	if (GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround())
	{		
		bPerformDashedAction = true;
		TimeDashedFor = 0.f;
		Crouch();
	}
}

void AGGCharacter::StopDashing()
{
	bIsDashKeyDown = false;
	//	Only ground dash can be stopped
	if (bIsCrouched)
	{
		bPerformDashedAction = false;
	}
	UnCrouch();
}

bool AGGCharacter::CanDash() const
{
	return CanDashInternal();
}

bool AGGCharacter::CanDashInternal_Implementation() const
{
	return bPerformDashedAction && TimeDashedFor < GetDashMaxDuration();
}

//**************************

// ****	Network movement****
void AGGCharacter::CheckJumpInput(float DeltaTime)
{
	if (bPressedJump)
	{
		UGGCharacterMovementComponent* MoveComp = 
			static_cast<UGGCharacterMovementComponent*>(GetCharacterMovement());
		if (MoveComp)
		{
			const bool bWasJumping = JumpKeyHoldTime > 0.0f;
			/** 
			* Increment our timer first so calls to IsJumpProvidingForce() will return true at first press. 
			* Subsequently, jumping and the timer increment is handled by the UGGCharacterMovementComponent.
			*/
			if (!bWasJumping)
			{
				if (bPressedWallJumpLeft || bPressedWallJumpRight)
				{
					const bool bDidWallJump = CanWallJump()
						&& MoveComp->DoWallJump();
					if (bDidWallJump)
					{
						//Just started a wall jump
						//OnWallJump();
					}
				}
				else
				{
					const bool bDidJump = CanJump() && MoveComp->DoJump(bClientUpdating);
					if (bDidJump)
					{
						OnJumped();
					}
				}				
			}						
		}
	}
	// Forced forward travel only on ground
	if (bPerformDashedAction)
	{
		//	Update timer if we are performing a dashed action
		TimeDashedFor += DeltaTime;
		//	If we are on ground dashing
		if (bIsCrouched && GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround())
		{
			if (GetPendingMovementInputVector().Y == 0.f)
			{
				//	Must move forward!
				AddMovementInput(GetPlanarForwardVector());
			}			
		}		
	}
}

void AGGCharacter::ClearJumpInput()
{
	// Don't disable bPressedJump right away if it's still held	
	if (bModeWallJump) 
	{
		if (JumpKeyHoldTime >= GetWallJumpMaxHoldTimeVertical())
		{
			bPressedJump = false;			
		}
		if ((bPressedWallJumpLeft || bPressedWallJumpRight)
			 && WallJumpLateralHoldTime >
			(bPerformDashedAction ? DashWallJumpMaxHoldTimeLateral : NormalWallJumpMaxHoldTimeLateral))
		{
			bPressedWallJumpLeft = false;
			bPressedWallJumpRight = false;
		}
		if (!(bPressedJump || bPressedWallJumpLeft || bPressedWallJumpRight))
		{
			bModeWallJump = false;
		}
	}
	else 
	{
		if (bPressedJump && (JumpKeyHoldTime >= GetJumpMaxHoldTime()))
		{
			bPressedJump = false;
		}
	}

	if (bPerformDashedAction && TimeDashedFor >= GetDashMaxDuration())
	{
		bPerformDashedAction = false;
		TimeDashedFor = 0.f;
		UnCrouch();
	}
}

//**************************

// ****	Movement utility****
float AGGCharacter::GetWallJumpMaxHoldTimeVertical() const
{
	return WallJumpMaxHoldTimeVertical;
}

float AGGCharacter::GetNormalWallJumpMaxHoldTimeLateral() const
{
	return NormalWallJumpMaxHoldTimeLateral;
}

float AGGCharacter::GetDashWallJumpMaxHoldTimeLateral() const
{
	return DashWallJumpMaxHoldTimeLateral;
}

float AGGCharacter::GetDashMaxDuration() const
{
	return DashMaxDuration;
}

//**************************

// ****		Damage		****
void AGGCharacter::LocalReceiveDamage(const FGGDamageReceivingInfo& InDamageInfo)
{
	if (IsLocallyControlled())
	{
		ReceiveDamage(InDamageInfo);
		if (Role == ROLE_Authority)
		{
			MulticastReceiveDamage(InDamageInfo.GetCompressedData());
		}
		else
		{
			ServerReceiveDamage(InDamageInfo.GetCompressedData());
		}
	}
}

bool AGGCharacter::ServerReceiveDamage_Validate(uint32 CompressedData)
{
    return true;
}

void AGGCharacter::ServerReceiveDamage_Implementation(uint32 CompressedData)
{    
	const FGGDamageReceivingInfo DamageInfo = FGGDamageReceivingInfo(CompressedData);
	ReceiveDamage(DamageInfo);
	MulticastReceiveDamage(CompressedData);
}

void AGGCharacter::MulticastReceiveDamage_Implementation(uint32 CompressedData)
{
    if (Role == ROLE_SimulatedProxy)
    {
		const FGGDamageReceivingInfo DamageInfo = FGGDamageReceivingInfo(CompressedData);
        ReceiveDamage(DamageInfo);
    }
}

void AGGCharacter::ReceiveDamage(const FGGDamageReceivingInfo& InDamageInfo)
{    
	// chance here to do damage reduction and pasdive calculation
    
}

//**************************

// ****	General Utility	****
FVector AGGCharacter::GetPlanarForwardVector() const
{
	if (BodyFlipbookComponent)
	{
		return BodyFlipbookComponent->RelativeScale3D.X < 0.f ? Left : Right;
	}
	return Right;
}

FTransform AGGCharacter::GetBodyTransform() const
{
	return !!BodyFlipbookComponent ? BodyFlipbookComponent->ComponentToWorld : FTransform();
}