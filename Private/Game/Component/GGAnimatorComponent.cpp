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
        for (int32 j = 0; j < EGGActionCategorySpecific::TYPES_COUNT; j++)
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
    for (int32 i = 0;i < EGGActionCategory::TYPES_COUNT; i++)
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

	/** With appropriate state selected, update our blendspace */
	TickCurrentBlendSpace();

	uint8 NewState = GetCompressedState();
	if (Rep_CompressedState != NewState)
	{
		Rep_CompressedState = NewState;
		ServerSetFromCompressedState(NewState);
	}
}

void UGGAnimatorComponent::PerformAction(TEnumAsByte<EGGActionCategory::Type> NewAction)
{
    //if (PrimaryStateIndex_Current != NewAction)
    //{
        PrimaryStateIndex_Previous = PrimaryStateIndex_Current;
        PrimaryStateIndex_Current = NewAction;
        bActionStateDirty = true;
    //}
}

void UGGAnimatorComponent::AlterActionMode(TEnumAsByte<EGGActionMode::Type> NewActionMode)
{
    int32 mode = NewActionMode;
    //if (SecondaryStateIndex_Current != mode)
    //{
        SecondaryStateIndex_Previous = SecondaryStateIndex_Current;
        SecondaryStateIndex_Current = mode;
        bActionModeStateDirty = true;
    //}
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

    TEnumAsByte<EGGActionCategory::Type> loc_Action((uint8)((state >> SecondaryState_BITS) & PrimaryState_MASK));
    TEnumAsByte<EGGActionMode::Type> loc_ActionMode((uint8)(state & SecondaryState_MASK));
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
}

const float BLENDSPACE_SENSATIVITY = 10.f;
void UGGAnimatorComponent::TickCurrentBlendSpace()
{
    if (PlaybackComponent && AnimationState_Current)
    {
        FGGAnimationState& ToState = *AnimationState_Current;
		float Position = PlaybackComponent->GetPlaybackPosition();
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
                if (ToState.PositiveFlipbook)
                {
                    PlaybackComponent->SetFlipbook(ToState.PositiveFlipbook);
                }
            }
            else if (velocity_v < -BLENDSPACE_SENSATIVITY)
            {
                if(ToState.NegativeFlipbook)
                {
                    PlaybackComponent->SetFlipbook(ToState.NegativeFlipbook);
                }
            }
            else
            {
                if (ToState.NeutralFlipbook)
                {
                    PlaybackComponent->SetFlipbook(ToState.NeutralFlipbook);
                }
            }
        }
        else
        {
            if (ToState.NeutralFlipbook)
            {
                PlaybackComponent->SetFlipbook(ToState.NeutralFlipbook);
            }
        }
		if (ToState.bMustPlayTillEnd)
		{
			PlaybackComponent->SetPlaybackPosition(Position, false);
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

        }
        else if (EndType == EGGAnimationStateEndType::Exit)
        {
            //  exit to locomotion default mode
            PerformAction(EGGActionCategory::Locomotion);
            AlterActionMode(EGGActionMode::Mode0);
        }
		UE_LOG(GGMessage, Log, TEXT("Animator reaches end of state"));
    }
}

void UGGAnimatorComponent::ReflectStateChanges()
{
	bActionStateDirty = false;
	bActionModeStateDirty = false;

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
			PlaybackComponent->PlayFromStart();
        }
    }
}

void UGGAnimatorComponent::ReflectIndexChanges()
{
    check(SortedStates.IsValidIndex(PrimaryStateIndex_Current) && SortedStates[PrimaryStateIndex_Current].States.IsValidIndex(SecondaryStateIndex_Current * 2 + BlendspaceIndex_Current));
    /** This should be the ONLY line in which we need to convert a ActionMode index together with blendspace index through multiplying by 2 and add blendspace index */
    //TODO: add method converting action mode + blendspace index to array index for easier management 
    AnimationState_Current = &SortedStates[PrimaryStateIndex_Current].States[SecondaryStateIndex_Current * 2 + BlendspaceIndex_Current];
}