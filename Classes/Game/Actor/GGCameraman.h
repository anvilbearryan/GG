// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
//#include "Game/Utility/GGFunctionLibrary.h"
#include "Game/Data/GGGameTypes.h"
#include "GGCameraman.generated.h"

UCLASS()
class GG_API AGGCameraman : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGGCameraman();	

	/** Transient information defining the viewable bounds of the current room */
	TArray<FGGBox2D> AreaBounds;
	
	/** The dimension of the view space, defines the player's viewing area when coupled with actor location */
	FVector2D ViewHalfExtent;
	
	/** The actor we are following, generally the local player character */
	TWeakObjectPtr<AActor> TargetFollowing;
	uint32 bInFollowMode : 1;

	void SetFollowTarget(const AActor* InTarget);
	void InitiateFollow();
	void PauseFollow();
	void ResumeFollow();

	//void SequenceBeginRoomTransition(FGGBox2D& InToEntrance, TArray<FGGBox2D> &InToAreaBounds);

	// Actual following logic / Transition logic
	virtual void Tick( float DeltaSeconds ) override;

};

namespace Geometry2DUtils{
/** Utility method for working out the additional displacement required when doing room transitions */
GG_API FVector2D GetMinDisplacementFromBounds(FGGBox2D &InPoint, TArray<FGGBox2D> &InAreaBounds);
}