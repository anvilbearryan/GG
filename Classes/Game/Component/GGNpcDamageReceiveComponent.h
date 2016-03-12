// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGNpcDamageReceiveComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GG_API UGGNpcDamageReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	int32 Hp_Current;
	UPROPERTY(Transient)
	int32 HpDebuffer;

public:
	UPROPERTY(ReplicatedUsing = OnRep_IsAlive, Transient)
		uint8 bIsAlive_Rep : 1;
	uint8 bIsAlive_Local : 1;

	UPROPERTY(EditAnywhere, replicated, Category = "GG|Damage", meta = (DisplayName = "Max Hp"))
		int32 Hp_Max;
	UPROPERTY(EditAnywhere, replicated, Category = "GG|Damage", meta = (DisplayName = "Defense by value"))
		int32 Defense_Subtractive;
	UPROPERTY(EditAnywhere, replicated, Category = "GG|Damage", meta = (DisplayName = "Defense by percent"))
		int32 Defense_Multiplicative;

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
