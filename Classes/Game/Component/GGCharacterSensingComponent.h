// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Data/GGGameTypes.h"
#include "GGCharacterSensingComponent.generated.h"
/**
* A 2-D box GGCharacter sensing component using dynamic delegates for event binding
* TODO: Cache pointer to GameState Character array
*/
UCLASS(Blueprintable, ClassGroup = "GG|AI", meta=(BlueprintSpawnableComponent) )
class GG_API UGGCharacterSensingComponent : public UActorComponent
{
	GENERATED_BODY()

    DECLARE_DYNAMIC_DELEGATE(FSensingEvent);
public:
    UPROPERTY(Category ="GGAI|Sensing", EditAnywhere, BlueprintReadOnly)
		FVector2D ActiveZone;
    UPROPERTY(Category ="GGAI|Sensing", EditAnywhere, BlueprintReadOnly)
		FVector2D AlertZone;
    /** Active sensing is not done per frame to save cpu, however alertness is to ensure entites look responsive */
    UPROPERTY(Category ="GGAI|Sensing", EditAnywhere, BlueprintReadOnly)
		float ActiveSensingInterval;
    
    float TimeUntilNextActiveSenseCheck;
    
    FSensingEvent OnActivate;
    FSensingEvent OnAlert;
    FSensingEvent OnUnalert;
    FSensingEvent OnDeactivate;
    
    UPROPERTY(VisibleAnywhere, Transient, Category ="GGAI|Sensing")
		EGGAISensingState SensingState;
    TWeakObjectPtr<AGGCharacter> Target;
    
    // Sets default values for this component's properties
	UGGCharacterSensingComponent();

	// Called when the game starts=	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;
};
