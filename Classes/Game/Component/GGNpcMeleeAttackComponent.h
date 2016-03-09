// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Data/GGGameTypes.h"
#include "GGNpcMeleeAttackComponent.generated.h"

USTRUCT(BlueprintType)
struct FGGNpcHitBoxData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		FGGCollisionShapeParser AttackShape;	
	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		FVector HitboxCentre;
	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		float Begin;
	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		float End;
	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		int32 DirectDamageBase;
	UPROPERTY(EditAnywhere, Category = "GGAttack|NpcMelee")
		int32 IndirectDamageBase;
	UPROPERTY(EditAnywhere)
		EGGDamageType Type;

	FCollisionShape HitShapeInternal;

	FGGNpcHitBoxData() {}
};

class AActor;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GG_API UGGNpcMeleeAttackComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, Category="GGAttack|Specification")
		TArray<FGGNpcHitBoxData> AttackHitbox_0;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		TEnumAsByte<ECollisionChannel> DamageChannel;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 DirectWeaponDamageBase;
	UPROPERTY(EditAnywhere, Category = "GGAttack|Specification")
		int32 IndirectWeaponDamageBase;
private:
	TArray<AActor*, TInlineAllocator<4>> AffectedEntities;
	float CurrentTimeStamp;
	float FullAttackLength;
	uint8 InstructionId;

public:
	// Sets default values for this component's properties
	UGGNpcMeleeAttackComponent();
	
	virtual void InitializeComponent() override;

	virtual void UseAttack(uint8 InInstructionID);

	virtual void FinishAttack();

	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;
	
	
};
