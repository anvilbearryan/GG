// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGCharacterSensingComponent.h"
#include "Game/Framework/GGGameState.h"


// Sets default values for this component's properties
UGGCharacterSensingComponent::UGGCharacterSensingComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    bWantsBeginPlay = true;
    PrimaryComponentTick.bCanEverTick = true;
    
    // ...
}

// Called every frame
void UGGCharacterSensingComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
    Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
    switch(SensingState)
    {
        case EGGAISensingState::Inactive:
        {
            TimeUntilNextActiveSenseCheck -= DeltaTime;
            if (TimeUntilNextActiveSenseCheck < 0.f)
            {
                //  Check active
                FVector MyLocation = GetOwner()->GetActorLocation();
                AGGGameState* GameState = static_cast<AGGGameState*>(GetWorld()->GetGameState());
                TArray<AGGCharacter*>& CharacterList = GameState->GetCharacterList();
                for (int32 i = 0; i < CharacterList.Num(); i++)
                {
                    //  Note we didn't check CharacterList pointed characters, the GameState should not add them in first place if its not valid.
                    FVector location = CharacterList[i]->GetActorLocation();
                    float dy = FMath::Abs(MyLocation.Y - location.Y);
                    float dz = FMath::Abs(MyLocation.Z - location.Z);
                    if (dy < ActiveZone.X && dz < ActiveZone.Y)
                    {
                        SensingState = EGGAISensingState::Active;
                        Target = CharacterList[i];
                        OnActivate.ExecuteIfBound();
                        break;
                    }
                }
                TimeUntilNextActiveSenseCheck = ActiveSensingInterval;
            }
        }
            break;
        case EGGAISensingState::Active:
        {    AGGCharacter* character= Target.Get(false);
            if (character == nullptr)
            {
                SensingState = EGGAISensingState::Inactive;
                Target.Reset();
                OnDeactivate.ExecuteIfBound();
                TimeUntilNextActiveSenseCheck = ActiveSensingInterval;
            }
            else
            {
                FVector MyLocation = GetOwner()->GetActorLocation();
                FVector location = character->GetActorLocation();
                float dy = FMath::Abs(MyLocation.Y - location.Y);
                float dz = FMath::Abs(MyLocation.Z - location.Z);
                if (dy < AlertZone.X && dz < AlertZone.Y)
                {
                    SensingState = EGGAISensingState::Alert;
                    OnAlert.ExecuteIfBound();
                }
                else if (dy > ActiveZone.X || dz > ActiveZone.Y)
                {
                    SensingState = EGGAISensingState::Inactive;
                    Target.Reset();
                    OnDeactivate.ExecuteIfBound();
                }
            }
        }    //  Check if ANY target gets in alert range
            break;
        case EGGAISensingState::Alert:
        {    // Check if target flied away~
            AGGCharacter* character= Target.Get(false);
            if (character == nullptr)
            {
                SensingState = EGGAISensingState::Inactive;
                Target.Reset();
                OnDeactivate.ExecuteIfBound();
                TimeUntilNextActiveSenseCheck = ActiveSensingInterval;
            }
            else
            {
                FVector MyLocation = GetOwner()->GetActorLocation();
                FVector location = character->GetActorLocation();
                float dy = FMath::Abs(MyLocation.Y - location.Y);
                float dz = FMath::Abs(MyLocation.Z - location.Z);
                if (dy > AlertZone.X || dz > AlertZone.Y)
                {
                    SensingState = EGGAISensingState::Active;
                    OnUnalert.ExecuteIfBound();
                }
            }
        }
            break;
    }
    
}

