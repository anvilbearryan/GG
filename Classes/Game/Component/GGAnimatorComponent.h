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
    /** Represent the category of action being performed, replicated */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category ="GG|Animation", meta=(DisplayName="PrimaryStateIndex"))
    int32 PrimaryStateIndex_Current;
    /** Represent the mode in which the action is being performed in, replicated */
    UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category ="GG|Animation", meta=(DisplayName="SecondaryStateIndex"))
    int32 SecondaryStateIndex_Current;
    /** Represent which blendspace the action is performed in, polled from outer and not replicated */
    UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category ="GG|Animation", meta=(DisplayName="BlendspaceIndex"))
    int32 BlendspaceIndex_Current;
    
    FGGAnimationState* AnimationState_Queued;
    int32 PrimaryStateIndex_Queued;
    int32 SecondaryStateIndex_Queued;
    
    /** Setter wrapper with enum to save messy BP castings */
    UFUNCTION(BlueprintCallable, Category ="GG|Animation")
        void PerformAction(TEnumAsByte<EGGActionCategory::Type> NewAction);
    /** Setter wrapper with enum to save messy BP castings */
    UFUNCTION(BlueprintCallable, Category ="GG|Animation")
        void AlterActionMode(TEnumAsByte<EGGActionMode::Type> NewActionMode);
    /** Updates the field BlendspaceIndex_Current through querying owner state */
    UFUNCTION(BlueprintImplementableEvent, Category="GG|Animation")
        void PollBlendspaceIndex();
    
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
    virtual void TickCurrentBlendSpace();
    
    UFUNCTION(BlueprintNativeEvent, Category ="GG|Animation")
        void OnReachEndOfState();
        virtual void OnReachEndOfState_Implementation();

protected:
    uint8 bActionStateDirty : 1;
    uint8 bActionModeStateDirty : 1;
    /** Utility method to action / action mode changes, on top of changing index we also check state specification to see if playback option etc needs to be changed */
    UFUNCTION()
        void ReflectStateChanges();

public:
    /** We assume specific state specification remain constatnt if we are just changing blendspace for pure index change */
    UFUNCTION(BlueprintCallable, Category ="GG|Animation")
        void ReflectIndexChanges();
};
