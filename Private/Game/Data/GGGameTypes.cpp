// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Data/GGGameTypes.h"
#include "Game/Data/GGProjectileData.h"


const FVector2D FGGDamageDealingInfo::Directions[10] =
{ FVector2D(0.f,0.f),
//	1	2	3
FVector2D(1.f,1.f), FVector2D(0.f,1.f), FVector2D(-1.f, 1.f),
//	4	5	6
FVector2D(1.f,0.f), FVector2D(0.f,0.f), FVector2D(-1.f, 0.f),
//	7	8	9
FVector2D(1.f,-1.f), FVector2D(0.f,-1.f), FVector2D(-1.f, -1.f)
};
const float FGGDamageDealingInfo::HitMargin = 6.25f;

const FVector2D FGGDamageReceivingInfo::Directions[10] =
{ FVector2D(0.f,0.f),
//	1	2	3
FVector2D(1.f,1.f), FVector2D(0.f,1.f), FVector2D(-1.f, 1.f),
//	4	5	6
FVector2D(1.f,0.f), FVector2D(0.f,0.f), FVector2D(-1.f, 0.f),
//	7	8	9
FVector2D(1.f,-1.f), FVector2D(0.f,-1.f), FVector2D(-1.f, -1.f)
};
const float FGGDamageReceivingInfo::HitMargin = 6.25f;

FLaunchedProjectile::FLaunchedProjectile(UGGPooledSpriteComponent* body, const FVector& direction, UGGProjectileData* data, float time)
	: LaunchDirection(direction), CurrentCollisionCount(0)
{
	SpriteBody = body;
	ProjectileData = data;
	if (data != nullptr)
	{
		ContinualAcceleration = data->GetGravityVector();
		ContinualAcceleration.Y = FMath::Sign(LaunchDirection.Y) * ContinualAcceleration.Y;
		CurrentVelocity = LaunchDirection * data->LaunchSpeed;
		SpawnTime = time;
		Lifespan = data->Lifespan;
	}
}