#pragma once

#include "GGFunctionLibrary.generated.h"


UCLASS()
class GG_API UGGFunctionLibrary: public UObject
{
	GENERATED_BODY()

public:
	/** Query UWorld for overlap and add any new result to the supplied array */
	static TArray<FOverlapResult> OverlapResults;
	template<typename AllocatorType>
	static FORCEINLINE bool WorldOverlapMultiActorByChannel(
		UWorld* World,
		const FVector & Pos,
		ECollisionChannel TraceChannel,
		const FCollisionShape & CollisionShape,
		TArray<AActor*, AllocatorType>& OutOverlaps
		)
	{
		if (World == nullptr)
		{
			return false;
		}

		OverlapResults.Reserve(24);
		OverlapResults.Reset();

		const FQuat IdentityQuat;
#if WITH_EDITOR
		switch (CollisionShape.ShapeType)
		{
		case ECollisionShape::Line:
			DrawDebugLine(World, Pos - CollisionShape.GetExtent() * 0.5f, Pos + CollisionShape.GetExtent() * 0.5f,
				FColor::Red, false, -1.f, 0, 12.5f);
			break;
		case ECollisionShape::Box:
			DrawDebugBox(World, Pos, CollisionShape.GetExtent(), FColor::Red);
			break;
		case ECollisionShape::Sphere:
			DrawDebugSphere(World, Pos, CollisionShape.GetSphereRadius(), 16, FColor::Red);
			break;
		case ECollisionShape::Capsule:
			DrawDebugCapsule(World, Pos, CollisionShape.GetCapsuleHalfHeight(), CollisionShape.GetCapsuleRadius(),
				FQuat(), FColor::Red, false, -1.f, 0, 12.5f);
			break;
		}
#endif

		if (World->OverlapMultiByChannel(OverlapResults, Pos, IdentityQuat, TraceChannel, CollisionShape))
		{
			bool bHasNewEntity = false;
			for (FOverlapResult& result : OverlapResults)
			{
				AActor* OverlapActor = result.GetActor();
				if (OverlapActor && !OutOverlaps.Contains(OverlapActor))
				{
					bHasNewEntity = true;
					OutOverlaps.Add(OverlapActor);
				}
			}
			return bHasNewEntity;
		}
		else
		{
			return false;
		}
	}
};