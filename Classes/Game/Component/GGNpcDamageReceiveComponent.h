// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGNpcDamageReceiveComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GG_API UGGNpcDamageReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	int32 HpCurrent;
	UPROPERTY(Transient)
	int32 HpDebuffer;
	UPROPERTY(replicated, Transient)
	int32 HpMax;
	UPROPERTY(ReplicatedUsing = OnRep_IsAlive, Transient)
		uint8 bIsAlive_Rep : 1;
	uint8 bIsAlive_Local : 1;
public:	
	// Sets default values for this component's properties
	UGGNpcDamageReceiveComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeHpState();

	virtual void ApplyDamageInformation(FGGDamageDealingInfo & Information);	

	UFUNCTION()
	void OnRep_IsAlive();

	UFUNCTION(BlueprintCallable, Category = "GG|Damage")
	int32 GetCurrentHealth() const;
};
