// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Game/Data/GGGameTypes.h"
#include "GGDamageableActor.generated.h"

class UGGNpcDamageReceiveComponent;
UCLASS()
class GG_API AGGDamageableActor : public AActor
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UGGNpcDamageReceiveComponent> HealthComponent;
	//********************************
	//	Movement states    	
	FGGDamageDealingInfo Cache_DamageReceived;
public:	
	// Sets default values for this actor's properties
	AGGDamageableActor();

	/** Method that is called even on late join clients, we bind the delegates here although only the server "should" be aware of those */
	virtual void PostInitializeComponents() override;

	//********************************
	//	Damage interface	
	UFUNCTION(NetMulticast, unreliable)
		void MulticastReceiveDamage(uint32 Delta, APlayerState* InCauser);
	void MulticastReceiveDamage_Implementation(uint32 Delta, APlayerState* InCauser);

	virtual void ReceiveDamage(FGGDamageDealingInfo InDamageInfo);

	virtual void CommenceDamageReaction(const FGGDamageDealingInfo& InDamageInfo);
	UFUNCTION()
		virtual void OnCompleteDamageReaction();
	virtual void CommenceDeathReaction();
	UFUNCTION()
		virtual void OnCompleteDeathReaction();
};
