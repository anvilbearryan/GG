// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcCollisionBoxComponent.h"
#include "Game/Actor/GGCharacter.h"

UGGNpcCollisionBoxComponent::UGGNpcCollisionBoxComponent()
{
	bWantsInitializeComponent = true;
	bGenerateOverlapEvents = true;
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = false;
}

void UGGNpcCollisionBoxComponent::InitializeComponent()
{
	Super::InitializeComponent();
	OnComponentBeginOverlap.AddDynamic(this, &UGGNpcCollisionBoxComponent::BeginOverlapToggle);
	OnComponentEndOverlap.AddDynamic(this, &UGGNpcCollisionBoxComponent::EndOverlapToggle);
	SetActive(false);
}

void UGGNpcCollisionBoxComponent::BeginOverlapToggle(AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	AGGCharacter* OtherCharacter = Cast<AGGCharacter>(OtherActor);
	//UE_LOG(GGMessage, Log, TEXT("Begin overlap"));
	if (OtherCharacter && OtherCharacter->IsLocallyControlled()) 
	{	
		//UE_LOG(GGMessage, Log, TEXT("Begin overlap inner"));
		//SetComponentTickEnabled(true);
		SetActive(true);
		// continually check for overlap so we don't miss any damage dealing chances
		OverlappeddCharacter = OtherCharacter;
		OverlappedCharacterHitbox = OtherComp;		
	}
}

void UGGNpcCollisionBoxComponent::EndOverlapToggle(AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex)
{
	AGGCharacter* OtherCharacter = Cast<AGGCharacter>(OtherActor);
	if (OtherCharacter && OtherCharacter->IsLocallyControlled())
	{
		OverlappeddCharacter = nullptr;
		OverlappedCharacterHitbox = nullptr;
		//SetComponentTickEnabled(false);
		SetActive(false);
	}
}

void UGGNpcCollisionBoxComponent::OnOverlapCharacter(AGGCharacter* InCharacter)
{
	if (IsActive() && InCharacter)
	{
		// take damage
		FGGDamageReceivingInfo info;
		info.Direct_BaseMultiplier = DirectCollisionDamageBase;
		info.Indirect_BaseMultiplier = IndirectCollisionDamageBase;
		// TODO: set variance

		info.ImpactDirection = FGGDamageReceivingInfo::ConvertDeltaPosition(
			InCharacter->GetActorLocation() - GetComponentLocation());
		InCharacter->LocalReceiveDamage(info);
	}
}

void UGGNpcCollisionBoxComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (OverlappedCharacterHitbox.IsValid())
	{
		// double-check the overlap
		if (IsOverlappingComponent(OverlappedCharacterHitbox.Get()))
		{
			OnOverlapCharacter(OverlappeddCharacter.Get());
		}
	}
	else
	{
		SetActive(false);
	}
}