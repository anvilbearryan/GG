// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGGDamageReceiveComponent::UGGDamageReceiveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = true;
}

void UGGDamageReceiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGGDamageReceiveComponent, Hp_Current);
	DOREPLIFETIME(UGGDamageReceiveComponent, Hp_Max);
}

void UGGDamageReceiveComponent::InitializeHpState()
{
    Hp_Current = Hp_Max;
    Hp_CurrentEstimate = Hp_Current;
    Hp_Recoverable = Hp_Max;
	SetComponentTickEnabled(false);
}

// Called every frame
void UGGDamageReceiveComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

    float RegenerationThisTick = (StandardHpRegenPerSecond_Base + StandardHpRegenPerSecond_Buff + StandardHpRegenPerSecond_Debuff) * DeltaTime;
    
    Hp_CurrentEstimate += RegenerationThisTick;
    
	if (Hp_CurrentEstimate >= Hp_Recoverable)
    {
        Hp_Current = Hp_Recoverable;
		SetComponentTickEnabled(false);
        if (Hp_Current == Hp_Max)
        {
            OnMaxedHp.Broadcast();
        }
    }
    else
    {
        Hp_Current = Hp_CurrentEstimate;
    }
}

void UGGDamageReceiveComponent::ApplyDamageInformation(FGGDamageReceivingInfo& information)
{
	int32 DirectDamage = information.GetDirectDamage();
	DirectDamage -= Defense_Subtractive;
	DirectDamage = FMath::RoundToInt((DirectDamage * Defense_Multiplicative) / 100.f);

	int32 IndirectDamage = information.GetIndirectDamage();
	IndirectDamage -= Defense_Subtractive;
	IndirectDamage = FMath::RoundToInt((IndirectDamage * Defense_Multiplicative) / 100.f);
		
	Cache_LastReceivedDamage = information;
    //  Set as decimal part of estimated hp
	Hp_CurrentEstimate -= FMath::FloorToFloat(Hp_CurrentEstimate);;
    
    Hp_Recoverable = Hp_Current - information.GetIndirectDamage();
    Hp_Current = Hp_Recoverable - information.GetDirectDamage();
    
    if (Hp_Current <= 0)
    {
        Hp_Current = 0;
        Hp_CurrentEstimate = 0.f;
        OnZeroedHp.Broadcast();
        return;
    }
    
    //  Add back the integral part of current hp
    Hp_CurrentEstimate += Hp_Current;
    
    if (Hp_Current < Hp_Recoverable)
    {
        //  enable regen through tick
		SetComponentTickEnabled(true);
    }
}

void UGGDamageReceiveComponent::HealHp(uint16 value)
{
	Hp_Current += value;
}

int32 UGGDamageReceiveComponent::GetCurrentHp() const
{
	return Hp_Current;
}

