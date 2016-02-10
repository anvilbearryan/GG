// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/GGCameraman.h"


// Sets default values
AGGCameraman::AGGCameraman()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}



void AGGCameraman::SetFollowTarget(const AActor * InTarget)
{
	if (TargetFollowing.Get() != InTarget)
	{
		// calls OnTargetChange event
	}
	TargetFollowing = InTarget;
	if (InTarget == nullptr)
	{
		// null target means we should are not in following mode, but non-null target doesn't mean we shuold follow immediately
		bInFollowMode = false;
	}
}

void AGGCameraman::InitiateFollow()
{
	AActor* Target = TargetFollowing.Get();
	if (Target == nullptr)
	{
		return;
	}
	// snap to target, then adjust for bounds
	FVector TargetLocation3D = Target->GetActorLocation();
	FVector2D TargetLocation2D = FVector2D(TargetLocation3D.Y, TargetLocation3D.Z);
	
	FGGBox2D ProposedSetting = FGGBox2D(TargetLocation2D, ViewHalfExtent);	
/*
	// need to find out the adjustment needed if we are to center our cameraman to the target, based on bounds information contained
	bool TopLeftContained, TopRightContained, BotLeftContained, BotRightContained;
	int32 TLIndex = -1;
	int32 TRIndex = -1;
	int32 BLIndex = -1;
	int32 BRIndex = -1;
	for (int32 i = 0; i < AreaBounds.Num(); i++)
	{
		FGGBox2D& Box = AreaBounds[i];
		if (TopLeftContained && TopRightContained && BotLeftContained && BotRightContained)
		{
			break;
		}
		if (!Box.IsDisjoint(ProposedSetting))
		{
			if (!TopLeftContained && Box.ContainsPoint(ProposedSetting.GetTopLeft()))
			{
				TopLeftContained = true;
				TLIndex = i;
			}
			if (!TopRightContained && Box.ContainsPoint(ProposedSetting.GetTopRight()))
			{
				TopRightContained = true;
				TRIndex = i;
			}
			if (!BotLeftContained && Box.ContainsPoint(ProposedSetting.GetBotLeft()))
			{
				BotLeftContained = true;
				BLIndex = i;
			}
			if (!BotRightContained && Box.ContainsPoint(ProposedSetting.GetBotRight()))
			{
				BotRightContained = true;
				BRIndex = i;
			}
		}
	}	
	// represent information above in form of Manhattan "nudge direction"
	FVector2D NudgeDirection = FVector2D();
	NudgeDirection.X = (!TopRightContained || !BotRightContained) ? -1.f : ((!TopLeftContained || !BotLeftContained) : 1.f : 0.f);
	NudgeDirection.X = (!TopRightContained || !TopLeftContained) ? -1.f : ((!BotLeftContained || !BotRightContained) : 1.f : 0.f);
	if (NudgeDirection.IsZero())
	{
		UE_LOG(GGMessage, Warning, TEXT("Camera trying to focus on a target that is totally outside the level, abort"));
		return;
	}
	FVector2D RequiredAdjustment;
	if (TLIndex != -1)
	{
		// get bot right distance from the quad of TLIndex
		FVector2D botRightAdj = AreaBounds[TLIndex].GetBotRight() - ProposedSetting.GetBotRight();
		/**
		* explanation:
		* Multiple NudgeDirection to ensure correct direction, the "Max" picks the absolute of current adjustment,
		* and above adj, signed in NudgeDirection so that opposite / zero adjustment required cases would result
		* in negative/zeri result hence won't affect the current obtained RequiredAdjustment.
		*/
/*		if (RequiredAdjustment.X != 0.f)
		{
			RequiredAdjustment.X = NudgeDirection.X * FMath::Max(botRightAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
		}
		else
		{
			RequiredAdjustment.X = FMath::Abs(botRightAdj.X) * NudgeDirection.X;
		}
		if (RequiredAdjustment.Y != 0.f)
		{
			RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(botRightAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
		}
		else
		{
			RequiredAdjustment.Y = FMath::Abs(botRightAdj.Y) * NudgeDirection.Y;
		}
	}
	if (TRIndex != -1)
	{
		// get bot left distance from the quad of TRIndex
		FVector2D botLeftAdj = AreaBounds[TRIndex].GetBotLeft() - ProposedSetting.GetBotLeft();
		/** ditto	*/
/*		if (RequiredAdjustment.X != 0.f)
		{
			RequiredAdjustment.X = NudgeDirection.X * FMath::Max(botLeftAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
		}
		else
		{
			RequiredAdjustment.X = FMath::Abs(botRightAdj.X) * NudgeDirection.X;
		}
		if (RequiredAdjustment.Y != 0.f)
		{
			RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(botLeftAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
		}
		else
		{
			RequiredAdjustment.Y = FMath::Abs(botRightAdj.Y) * NudgeDirection.Y;
		}
	}
	if (BRIndex != -1)
	{
		// get top left distance from the quad of BRIndex
		FVector2D topLeftAdj = AreaBounds[BRIndex].GetTopLeft() - ProposedSetting.GetTopLeft();
		/** ditto	*/
/*		if (RequiredAdjustment.X != 0.f)
		{
			RequiredAdjustment.X = NudgeDirection.X * FMath::Max(topLeftAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
		}
		else
		{
			RequiredAdjustment.X = FMath::Abs(botRightAdj.X) * NudgeDirection.X;
		}
		if (RequiredAdjustment.Y != 0.f) 
		{
			RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(topLeftAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
		}
		else
		{
			RequiredAdjustment.Y = FMath::Abs(botRightAdj.Y) * NudgeDirection.Y;
		}
	}
	if (BLIndex != -1)
	{
		// get top left distance from the quad of BRIndex
		FVector2D topRightAdj = AreaBounds[BRIndex].GetTopRight() - ProposedSetting.GetTopRight();
		/** ditto	*/
/*		if (RequiredAdjustment.X != 0.f)
		{
			RequiredAdjustment.X = NudgeDirection.X * FMath::Max(topRightAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
		}
		else
		{
			RequiredAdjustment.X = FMath::Abs(botRightAdj.X) * NudgeDirection.X;
		}
		if (RequiredAdjustment.Y != 0.f)
		{
			RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(topRightAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
		}
		else
		{
			RequiredAdjustment.Y = FMath::Abs(botRightAdj.Y) * NudgeDirection.Y;
		}
	}
	UE_LOG(GGMessage, Log, TEXT("RequiredAdjustment =");
	UE_LOG(GGMessage, Log, RequiredAdjustment.ToString());
	ProposedSetting.Centre = ProposedSetting.Centre + RequiredAdjustment;
	// apply result above
*/
	
	ProposedSetting.Centre = ProposedSetting.Centre + Geometry2DUtils::GetMinDisplacementFromBounds(ProposedSetting, AreaBounds);
	FVector ActorLoc = FVector(0.f, ProposedSetting.Centre.X, ProposedSetting.Centre.Y);
	SetActorLocation(ActorLoc, false, nullptr);
	bInFollowMode = true;
}

void AGGCameraman::PauseFollow()
{
	bInFollowMode = false;
}

void AGGCameraman::ResumeFollow()
{
	bInFollowMode = true;
}

// Called every frame
void AGGCameraman::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

namespace Geometry2DUtils {
	FVector2D GetMinDisplacementFromBounds(FGGBox2D &InBox, TArray<FGGBox2D> &InAreaBounds) 
	{
		bool TopLeftContained = false;
		bool TopRightContained = false;
		bool BotLeftContained = false; 
		bool BotRightContained = false;
		int32 TLIndex = -1;
		int32 TRIndex = -1;
		int32 BLIndex = -1;
		int32 BRIndex = -1;
		for (int32 i = 0; i < InAreaBounds.Num(); i++)
		{
			FGGBox2D& Box = InAreaBounds[i];
			if (TopLeftContained && TopRightContained && BotLeftContained && BotRightContained)
			{
				break;
			}
			if (!Box.IsDisjoint(InBox))
			{
				if (!TopLeftContained && Box.ContainsPoint(InBox.GetTopLeft()))
				{
					TopLeftContained = true;
					TLIndex = i;
				}
				if (!TopRightContained && Box.ContainsPoint(InBox.GetTopRight()))
				{
					TopRightContained = true;
					TRIndex = i;
				}
				if (!BotLeftContained && Box.ContainsPoint(InBox.GetBotLeft()))
				{
					BotLeftContained = true;
					BLIndex = i;
				}
				if (!BotRightContained && Box.ContainsPoint(InBox.GetBotRight()))
				{
					BotRightContained = true;
					BRIndex = i;
				}
			}
		}
		// represent information above in form of Manhattan "nudge direction"
		FVector2D NudgeDirection = FVector2D();
		if (!TopRightContained || !BotRightContained)
		{
			NudgeDirection.X = -1.f;
		}
		else if ((!TopLeftContained || !BotLeftContained)) {
			NudgeDirection.X = 1.f;
		}
		else
		{
			NudgeDirection.X = 0.f;
		}
		if (!TopRightContained || !TopLeftContained)
		{
			NudgeDirection.Y = -1.f;
		}
		else if ((!BotLeftContained || !BotRightContained))
		{
			NudgeDirection.Y = 1.f;
		}
		else
		{
			NudgeDirection.Y = 0.f;
		}
		  
		if (NudgeDirection.IsZero())
		{
			UE_LOG(GGMessage, Warning, TEXT("Camera trying to focus on a target that is totally outside the level, abort"));
			return FVector2D();
		}
		FVector2D RequiredAdjustment = FVector2D();
		if (TLIndex != -1)
		{
			// get bot right distance from the quad of TLIndex
			FVector2D botRightAdj = InAreaBounds[TLIndex].GetBotRight() - InBox.GetBotRight();
			/**
			* explanation:
			* Multiple NudgeDirection to ensure correct direction, the "Max" picks the absolute of current adjustment,
			* and above adj, signed in NudgeDirection so that opposite / zero adjustment required cases would result
			* in negative/zeri result hence won't affect the current obtained RequiredAdjustment.
			*/
			if (RequiredAdjustment.X != 0.f)
			{
				RequiredAdjustment.X = NudgeDirection.X * FMath::Max(botRightAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
			}
			else
			{
				RequiredAdjustment.X = FMath::Abs(botRightAdj.X) * NudgeDirection.X;
			}
			if (RequiredAdjustment.Y != 0.f)
			{
				RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(botRightAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
			}
			else
			{
				RequiredAdjustment.Y = FMath::Abs(botRightAdj.Y) * NudgeDirection.Y;
			}
		}
		if (TRIndex != -1)
		{
			// get bot left distance from the quad of TRIndex
			FVector2D botLeftAdj = InAreaBounds[TRIndex].GetBotLeft() - InBox.GetBotLeft();
			/** ditto	*/
			if (RequiredAdjustment.X != 0.f)
			{
				RequiredAdjustment.X = NudgeDirection.X * FMath::Max(botLeftAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
			}
			else
			{
				RequiredAdjustment.X = FMath::Abs(botLeftAdj.X) * NudgeDirection.X;
			}
			if (RequiredAdjustment.Y != 0.f)
			{
				RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(botLeftAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
			}
			else
			{
				RequiredAdjustment.Y = FMath::Abs(botLeftAdj.Y) * NudgeDirection.Y;
			}
		}
		if (BRIndex != -1)
		{
			// get top left distance from the quad of BRIndex
			FVector2D topLeftAdj = InAreaBounds[BRIndex].GetTopLeft() - InBox.GetTopLeft();
			/** ditto	*/
			if (RequiredAdjustment.X != 0.f)
			{
				RequiredAdjustment.X = NudgeDirection.X * FMath::Max(topLeftAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
			}
			else
			{
				RequiredAdjustment.X = FMath::Abs(topLeftAdj.X) * NudgeDirection.X;
			}
			if (RequiredAdjustment.Y != 0.f)
			{
				RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(topLeftAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
			}
			else
			{
				RequiredAdjustment.Y = FMath::Abs(topLeftAdj.Y) * NudgeDirection.Y;
			}
		}
		if (BLIndex != -1)
		{
			// get top left distance from the quad of BRIndex
			FVector2D topRightAdj = InAreaBounds[BRIndex].GetTopRight() - InBox.GetTopRight();
			/** ditto	*/
			if (RequiredAdjustment.X != 0.f)
			{
				RequiredAdjustment.X = NudgeDirection.X * FMath::Max(topRightAdj.X * NudgeDirection.X, FMath::Abs(RequiredAdjustment.X));
			}
			else
			{
				RequiredAdjustment.X = FMath::Abs(topRightAdj.X) * NudgeDirection.X;
			}
			if (RequiredAdjustment.Y != 0.f)
			{
				RequiredAdjustment.Y = NudgeDirection.Y * FMath::Max(topRightAdj.Y * NudgeDirection.Y, FMath::Abs(RequiredAdjustment.Y));
			}
			else
			{
				RequiredAdjustment.Y = FMath::Abs(topRightAdj.Y) * NudgeDirection.Y;
			}
		}
		
		//UE_LOG(LogTemp, Warning, TEXT("RequiredAdjustment is: %s"), FVector(0.f, RequiredAdjustment.X, RequiredAdjustment.Y).ToString());
		return RequiredAdjustment;
	}
}