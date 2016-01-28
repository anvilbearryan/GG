// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "GGAnimatorComponent.generated.h"

/**
* Animator component handles network synced state transitions through replicating a byte with compressed information
*/

class UPaperFlipbookComponent;

UCLASS(Blueprintable, ClassGroup = (Animation))
class GG_API UGGAnimatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UGGAnimatorComponent();
    
    virtual void InitializeComponent() override;
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    
    virtual void ManualTick(float DeltaTime);
    
    //  User configured array to store possible states, internally we save a sorted copy with holes for missing states
    UPROPERTY( EditAnywhere, Category ="GG|Animation")
        TArray<FGGAnimationStateArray> AvailableStates;
    UPROPERTY(VisibleAnywhere, Category = "GG|Animation")
        UPaperFlipbookComponent* PlaybackComponent;
    
protected:
    //UPROPERTY(VisibleAnywhere, Category = "GG|Animation")
    TArray<FGGAnimationStateArray> SortedStates;
    
public:
    //  This flag tells us which and the direction (last bit) of the incoming state
    UPROPERTY(Category ="GG|Animation", VisibleAnywhere, ReplicatedUsing=OnRep_CompressedState)
        uint8 Rep_CompressedState;
    
    FGGAnimationState* AnimationState_Previous;
    int32 PrimaryStateIndex_Previous;
    int32 SecondaryStateIndex_Previous;
    
    FGGAnimationState* AnimationState_Current;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category ="GG|Animation", meta=(DisplayName="PrimaryStateIndex"))
    int32 PrimaryStateIndex_Current;
    UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category ="GG|Animation", meta=(DisplayName="SecondaryStateIndex"))
    int32 SecondaryStateIndex_Current;
    
    FGGAnimationState* AnimationState_Queued;
    int32 PrimaryStateIndex_Queued;
    int32 SecondaryStateIndex_Queued;
    
    /**
    * Entry method for exiting a state, either due to current state being a locomotion like state or we reached the end of the current state
    */
    UFUNCTION(BlueprintNativeEvent, Category="GG|Animation")
        void PollStateFromOwningActor();
        virtual void PollStateFromOwningActor_Implementation();
    
    /** Update the state index using values from enum. Returns whether change has occured */
    UFUNCTION(BlueprintImplementableEvent, Category="GG|Animation")
        bool CheckOwnerForStateChange();
    
    UFUNCTION()
        virtual uint8 GetCompressedState() const;

    //  Update route for client controlled Animators, calls UpdateFromCOmpressedState on server
    UFUNCTION(unreliable, server, WithValidation, Category ="GG|Animation")
        void ServerSetFromCompressedState(uint8 CompressedState);
        bool ServerSetFromCompressedState_Validate(uint8 CompressedState);
        void ServerSetFromCompressedState_Implementation(uint8 CompressedState);
    //  Called on SimulatedProxy when server updates CompressedState, calls UpdateFromCOmpressedState
    UFUNCTION()
        virtual void OnRep_CompressedState();
    //  Catch up steps taken by non-owners using the new Rep_CompressedState
    UFUNCTION()
        virtual void UpdateFromCompressedState();
    
    UFUNCTION()
    virtual void TransitToAnimationState(FGGAnimationState &ToState, int32 PrimaryStateIndex, int32 SecondaryStateIndex);
    
    UFUNCTION()
    virtual void TickCurrentBlendSpace();
    
    UFUNCTION(BlueprintNativeEvent, Category ="GG|Animation")
        void OnReachEndOfState();
        virtual void OnReachEndOfState_Implementation();
    
    /** Utility method to handle changes done in blueprint */
    UFUNCTION(BlueprintCallable, Category ="GG|Animation")
        void ReflectIndexChanges();
};
