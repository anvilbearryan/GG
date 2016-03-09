// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Game/Component/GGCharacterMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "Game/Component/GGFlipbookFlashHandler.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Game/Framework/GGGamePlayerController.h"

FName AGGCharacter::BodyFlipbookComponentName(TEXT("BodyFlipbookComponent"));
FName AGGCharacter::FlipbookFlashHandlerName(TEXT("FlipbookFlashHandler"));
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

	FlipbookFlashHandler = CreateDefaultSubobject<UGGFlipbookFlashHandler>(AGGCharacter::FlipbookFlashHandlerName);	
}

void AGGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGGCharacter, NormalWallJumpMaxHoldTimeLateral);
}

void AGGCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (BodyFlipbookComponent == nullptr)
	{
		UE_LOG(GGWarning, Warning, TEXT("No Flipbook component on character"));
	}
	if (FlipbookFlashHandler == nullptr)
	{
		UE_LOG(GGWarning, Warning, TEXT("No FlipbookFlashHandler on character"));
	}
	else
	{
		FlipbookFlashHandler->SetActive(false);
	}

	HealthComponent = FindComponentByClass<UGGDamageReceiveComponent>();
	if (HealthComponent.IsValid())
	{
		HealthComponent.Get()->InitializeHpState();
	}
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
	if (!GetCharacterMovement() || bUseEnforcedMovement || bActionInputDisabled)
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
	if (bUseEnforcedMovement || bActionInputDisabled)
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
void AGGCharacter::LocalReceiveDamage(FGGDamageReceivingInfo& InDamageInfo)
{
	if (IsLocallyControlled() && 
		!FlipbookFlashHandler->IsActive()) // ugly damage immunity check
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
	FGGDamageReceivingInfo DamageInfo = FGGDamageReceivingInfo(CompressedData);
	ReceiveDamage(DamageInfo);
	MulticastReceiveDamage(CompressedData);
}

void AGGCharacter::MulticastReceiveDamage_Implementation(uint32 CompressedData)
{
    if (Role == ROLE_SimulatedProxy)
    {
		FGGDamageReceivingInfo DamageInfo = FGGDamageReceivingInfo(CompressedData);
        ReceiveDamage(DamageInfo);
    }
}

void AGGCharacter::ReceiveDamage(FGGDamageReceivingInfo& InDamageInfo)
{    
	// damage applying calculation
	UGGDamageReceiveComponent* locHealthComp = HealthComponent.Get();
	if (locHealthComp)
	{		
		locHealthComp->ApplyDamageInformation(InDamageInfo);
		if (locHealthComp->GetCurrentHp() > 0) 
		{
			CommenceDamageReaction(InDamageInfo);
		}
		else
		{
			CommenceDeathReaction(InDamageInfo);
		}
	}	
}

void AGGCharacter::MulticastHealCharacter_Implementation(uint16 value)
{
	UGGDamageReceiveComponent* locHealth = HealthComponent.Get();
	if (value > 0 && locHealth)
	{
		locHealth->HealHp(value);
		if (IsLocallyControlled())
		{
			AGGGamePlayerController* locController = static_cast<AGGGamePlayerController*>(Controller);			
			if (locController)
			{
				locController->UpdateHealthDisplay(locHealth->GetCurrentHp(), locHealth->Hp_Max);				
			}
		}
	}
}

//**************************

// ****	Damage reaction	****
void AGGCharacter::CommenceDamageReaction(const FGGDamageReceivingInfo& InDamageInfo)
{	
	// ********************************
	// State reaction
	if (SecondsDisabledOnReceiveDamage > 0.f)
	{
		bUseEnforcedMovement = true;
		bActionInputDisabled = true;
		bLockedFacing = true;
		StopJumping();
		StopDashing();
		GetWorld()->GetTimerManager().SetTimer(DamageReactHandle, this,
			&AGGCharacter::OnCompleteDamageReaction, SecondsDisabledOnReceiveDamage);
	}
	else
	{
		OnCompleteDamageReaction();
	}
	// ********************************
	// Model reaction
	if (FlipbookFlashHandler)
	{
		// Flash sprite
		FlipbookFlashHandler->SetFlashSchedule(BodyFlipbookComponent, SecondsImmuneOnReceiveDamage);
	}
	// ********************************
	// UI reaction
	if (IsLocallyControlled())
	{
		AGGGamePlayerController* locController = Cast<AGGGamePlayerController>(Controller);
		UGGDamageReceiveComponent* locHealth = HealthComponent.Get();
		if (locController && locHealth)
		{
			locController->UpdateHealthDisplay(locHealth->GetCurrentHp(), locHealth->Hp_Max);
			locController->OnLocalCharacterReceiveDamage(InDamageInfo.GetDirectDamage());
		}
	}
	else
	{
		AGGGamePlayerController* locController = Cast<AGGGamePlayerController>(
			GetWorld()->GetFirstPlayerController());
		if (locController)
		{
			locController->OnRemoteCharacterReceiveDamage(InDamageInfo.GetDirectDamage());
		}
	}
}

void AGGCharacter::OnCompleteDamageReaction()
{
	bLockedFacing = false;
	bUseEnforcedMovement = false;
	bActionInputDisabled = false;
	EnforcedMovementStrength = 0.f;
}

void AGGCharacter::CommenceDeathReaction(const FGGDamageReceivingInfo& InDamageInfo)
{

}

void AGGCharacter::OnCompleteDeathReaction()
{

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