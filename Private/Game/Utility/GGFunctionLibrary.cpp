#include "GG.h"
#include "Game/Utility/GGFunctionLibrary.h"

bool UGGFunctionLibrary::WorldOverlapMultiActorByChannel(UWorld* World, const FVector & Pos, ECollisionChannel TraceChannel, const FCollisionShape & CollisionShape, TArray<AActor*>& OutOverlaps)
{
	if (World == nullptr)
	{
		return false;
	}

	TArray<FOverlapResult> OutHits;
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

	if (World->OverlapMultiByChannel(OutHits, Pos, IdentityQuat, TraceChannel, CollisionShape))
	{
		bool bHasNewEntity = false;
		for (FOverlapResult& result : OutHits)
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