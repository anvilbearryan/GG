// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Net/UnrealNetwork.h"
#include "Game/Component/GGAnimatorComponent.h"
#include "PaperFlipbookComponent.h"

//  Constants for compressed flags
const uint8 FLIP_MASK = 128;

const uint8 PrimaryState_BITS = 3;
const uint8 PrimaryState_MASK = 7;

const uint8 SecondaryState_BITS = 4;
const uint8 SecondaryState_MASK = 15;

UGGAnimatorComponent::UGGAnimatorComponent()
{
    bReplicates = true;
    bWantsInitializeComponent = true;
}

void UGGAnimatorComponent::InitializeComponent()
{
    Super::InitializeComponent();
    if (AvailableStates.Num() == 0 )
    {
        return;
    }
    //  sort secondary states in elements of a copy of AvailableStates
    TArray<FGGAnimationStateArray> loc_AvailableStates = AvailableStates;
    for (FGGAnimationStateArray& PrimArray : loc_AvailableStates)
    {
        // create a copy
        TArray<FGGAnimationState> StatesCache = PrimArray.States;
        PrimArray.States.Empty();
        for (int32 j = 0; j <= SecondaryState_MASK; j++)
        {
            PrimArray.States.AddDefaulted();
            for (auto state : StatesCache)
            {
                if (state.SecondaryState == j)
                {
                    PrimArray.States[j] = state;
                    break;
                }
            }
        }
    }
    
    //  read from the filled AvailableStates and fill SortedStates
    SortedStates.Empty();
    for (int32 i = 0;i <=PrimaryState_MASK; i++)
    {
        SortedStates.AddDefaulted();
        for (FGGAnimationStateArray& PrimArray : loc_AvailableStates)
        {
            if (PrimArray.PrimaryState == i)
            {
                SortedStates[i] = PrimArray;
                break;
            }
        }
    }
}

void UGGAnimatorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME_CONDITION(UGGAnimatorComponent, Rep_CompressedState, COND_SimulatedOnly);
}

void UGGAnimatorComponent::ManualTick(float DeltaTime)
{
    if (AnimationState_Current == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(1, DeltaTime, FColor::Red, TEXT("Null state"));

        ReflectIndexChanges();
    }
    if(!AnimationState_Current->bMustPlayTillEnd)
    {
        // Do work every tick since we may change state anytime
        //PollStateFromOwningActor();
        
        // Cache blendspace index before polling for new
        int32 loc_BlendSpaceIndex = BlendspaceIndex_Current;
        PollBlendspaceIndex();
        if (bActionStateDirty || bActionModeStateDirty)
        {
            ReflectStateChanges();
        }
        else if (loc_BlendSpaceIndex != BlendspaceIndex_Current)
        {
            // prevent unnecessary array fetching
            ReflectIndexChanges();
        }
        
        //ReflectIndexChanges();
        
        /** With appropriate state selected, update our blendspace */
        TickCurrentBlendSpace();
    }
}

void UGGAnimatorComponent::PollStateFromOwningActor_Implementation()
{
    /** We need to ensure net owned entity's flag is not altered by authority */
    if (!CheckOwnerForStateChange())
    {
        ReflectIndexChanges();
    }
    TickCurrentBlendSpace();
    /** Server update animation, we can actually enclose it with the above condition in current setup, but have not due
    *   to insignificant benefit plus this is more xeplicit (& more refined control)
    */
    AActor* loc_owner = GetOwner();
    if (loc_owner && loc_owner->Role == ROLE_Authority)
    {
        uint8 compressedState = GetCompressedState();
        if (compressedState != Rep_CompressedState)
        {
            Rep_CompressedState = compressedState;
        }
    }
    //  Local owner update animation
    else if (loc_owner && loc_owner->Role == ROLE_AutonomousProxy)
    {
        uint8 compressedState = GetCompressedState();
        if (compressedState != Rep_CompressedState)
        {
            Rep_CompressedState = compressedState;
            ServerSetFromCompressedState(compressedState);
        }
    }
}

void UGGAnimatorComponent::PerformAction(TEnumAsByte<EGGActionCategory::Type> NewAction)
{
    if (PrimaryStateIndex_Current != NewAction)
    {
        PrimaryStateIndex_Previous = PrimaryStateIndex_Current;
        PrimaryStateIndex_Current = NewAction;
        bActionStateDirty = true;
    }
}

void UGGAnimatorComponent::AlterActionMode(TEnumAsByte<EGGActionCategorySpecific::Type> NewActionMode)
{
    // Cannot just cast to int32 for action mode
    int32 mode = NewActionMode / 2;
    if (SecondaryStateIndex_Current != mode)
    {
        SecondaryStateIndex_Previous = SecondaryStateIndex_Current;
        SecondaryStateIndex_Current = mode;
        bActionModeStateDirty = true;
    }
}

uint8 UGGAnimatorComponent::GetCompressedState() const
{
    // TODO: Possible lag compensation, send pending state instead
    uint8 result = 0;
    if (PlaybackComponent == nullptr)
    {
        return result;
    }
    if (PlaybackComponent->RelativeScale3D.X < 0.f)
    {
        result |= FLIP_MASK;
    }
    check(PrimaryStateIndex_Current <= PrimaryState_MASK && PrimaryStateIndex_Current >= 0);
    result |= (PrimaryStateIndex_Current << SecondaryState_BITS);
    check(SecondaryStateIndex_Current <= SecondaryState_MASK && SecondaryStateIndex_Current >=0);
    result |= SecondaryStateIndex_Current;
    
    return result;
}

bool UGGAnimatorComponent::ServerSetFromCompressedState_Validate(uint8 CompressedState)
{
    return true;
}

void UGGAnimatorComponent::ServerSetFromCompressedState_Implementation(uint8 CompressedState)
{
    Rep_CompressedState = CompressedState;
    UpdateFromCompressedState();
}

void UGGAnimatorComponent::OnRep_CompressedState()
{
    UpdateFromCompressedState();
}

void UGGAnimatorComponent::UpdateFromCompressedState()
{
    //  extract information to human readable local variables
    uint8 state = Rep_CompressedState;
    bool bIsFacingLeft = (state & FLIP_MASK) != 0;
    /*
    int32 loc_PrimaryStateIndex = (state >> SecondaryState_BITS) & PrimaryState_MASK;
    
    int32 loc_SecondaryStateIndex = (state & SecondaryState_MASK);
    */
    TEnumAsByte<EGGActionCategory::Type> loc_Action((uint8)((state >> SecondaryState_BITS) & PrimaryState_MASK));
    TEnumAsByte<EGGActionCategorySpecific::Type> loc_ActionMode((uint8)(state & SecondaryState_MASK));
    PerformAction(loc_Action);
    AlterActionMode(loc_ActionMode);
    
    //  apply the extracted information, i.e. state transition
    // TODO: Possible to smoothen experience by wait till current state finishes through Queued state (unused currently)
    /** Handle flipping */
    if (PlaybackComponent)
    {
        if (bIsFacingLeft)
        {
            PlaybackComponent->SetRelativeScale3D(FVector(-1.f, 1.f, 1.f));
        }
        else
        {
            PlaybackComponent->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
        }
    }
    /*
    // Check flags received are valid
    if (SortedStates.IsValidIndex(loc_PrimaryStateIndex) && SortedStates[loc_PrimaryStateIndex].States.IsValidIndex(loc_SecondaryStateIndex * 2 + 1))
    {
        TransitToAnimationState(SortedStates[loc_PrimaryStateIndex].States[loc_SecondaryStateIndex + BlendspaceIndex_Current], loc_PrimaryStateIndex, loc_SecondaryStateIndex);
    }
    else
    {
#if WITH_EDITOR
        GEngine->AddOnScreenDebugMessage(-1, 2.25f, FColor::Cyan, TEXT("Invalid state index"));
#endif
    }
    */
}

const float BLENDSPACE_SENSATIVITY = 10.f;
void UGGAnimatorComponent::TransitToAnimationState(FGGAnimationState& ToState, int32 ParentIndex, int32 SecondaryStateIndex)
{
        // save the state we are currently in
        AnimationState_Previous = AnimationState_Current;
        PrimaryStateIndex_Previous = PrimaryStateIndex_Current;
        SecondaryStateIndex_Previous = SecondaryStateIndex_Current;
        
        AnimationState_Current = &ToState;
        PrimaryStateIndex_Current = ParentIndex;
        SecondaryStateIndex_Current = SecondaryStateIndex;
    
        if (PlaybackComponent)
        {
            TickCurrentBlendSpace();
            if (ToState.StateEndType == EGGAnimationStateEndType::Loop)
            {
                PlaybackComponent->SetLooping(true);
            }
            else
            {
                PlaybackComponent->SetLooping(false);
            }
        }
}

void UGGAnimatorComponent::TickCurrentBlendSpace()
{
    if (PlaybackComponent && AnimationState_Current)
    {
        FGGAnimationState& ToState = *AnimationState_Current;
        if (ToState.ShouldBlendHorizontal())
        {
            float velocity_h = GetOwner()->GetVelocity().Y * FMath::Sign(PlaybackComponent->RelativeScale3D.X);
            if (velocity_h > BLENDSPACE_SENSATIVITY)
            {
                PlaybackComponent->SetFlipbook(ToState.PositiveFlipbook);
            }
            else if (velocity_h < -BLENDSPACE_SENSATIVITY)
            {
                PlaybackComponent->SetFlipbook(ToState.NegativeFlipbook);
            }
            else
            {
                PlaybackComponent->SetFlipbook(ToState.NeutralFlipbook);
            }
        }
        else if (ToState.PositiveFlipbook != nullptr)
        {
            // blend vertical
            float velocity_v = GetOwner()->GetVelocity().Z;
            if (velocity_v > BLENDSPACE_SENSATIVITY)
            {
                PlaybackComponent->SetFlipbook(ToState.PositiveFlipbook);
            }
            else if (velocity_v < -BLENDSPACE_SENSATIVITY)
            {
                PlaybackComponent->SetFlipbook(ToState.NegativeFlipbook);
            }
            else
            {
                PlaybackComponent->SetFlipbook(ToState.NeutralFlipbook);
            }
        }
        else
        {
            PlaybackComponent->SetFlipbook(ToState.NeutralFlipbook);
        }
        PlaybackComponent->Play();
    }
}

void UGGAnimatorComponent::OnReachEndOfState_Implementation()
{
    if (AnimationState_Current)
    {
        EGGAnimationStateEndType::Type EndType = AnimationState_Current->StateEndType;
        if (EndType == EGGAnimationStateEndType::Revert)
        {
            /*
            TransitToAnimationState(*AnimationState_Previous, PrimaryStateIndex_Previous, SecondaryStateIndex_Previous);
            */
        } else if (EndType == EGGAnimationStateEndType::Exit)
        {
            //  replay this state while let it continue to work out whats next
            PlaybackComponent->PlayFromStart();
        }
    }
}

void UGGAnimatorComponent::ReflectStateChanges()
{
    AnimationState_Previous = AnimationState_Current;
    
    ReflectIndexChanges();
    
    if (PlaybackComponent && AnimationState_Current)
    {
        if (AnimationState_Current->StateEndType == EGGAnimationStateEndType::Loop)
        {
            PlaybackComponent->SetLooping(true);
        }
        else
        {
            PlaybackComponent->SetLooping(false);
        }
    }
}

void UGGAnimatorComponent::ReflectIndexChanges()
{
    check(SortedStates.IsValidIndex(PrimaryStateIndex_Current) && SortedStates[PrimaryStateIndex_Current].States.IsValidIndex(SecondaryStateIndex_Current * 2 + BlendspaceIndex_Current));
    AnimationState_Current = &SortedStates[PrimaryStateIndex_Current].States[SecondaryStateIndex_Current*2 + BlendspaceIndex_Current];
}