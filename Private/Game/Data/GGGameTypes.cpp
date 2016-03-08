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

uint8 FGGDamageDealingInfo::ConvertDeltaPosition(const FVector& InDeltaPosition)
{
	uint8 Out = 0;
	if (InDeltaPosition.Z > FGGDamageDealingInfo::HitMargin)
	{
		// case 123
		Out = 7;
	}
	else if (InDeltaPosition.Z < -FGGDamageDealingInfo::HitMargin)
	{
		// case 789
		Out = 1;
	}
	else
	{
		// case 456
		Out = 4;
	}
	if (InDeltaPosition.Y > FGGDamageDealingInfo::HitMargin)
	{
		return Out;
	}
	else if (InDeltaPosition.Y < -FGGDamageDealingInfo::HitMargin)
	{
		Out += 2;
	}
	else
	{
		Out += 1;
	}
	return Out;
}

int32 FGGDamageDealingInfo::PrecisionMultiplierToPercent(uint8 InMultiplier)
{
	int32 multiplier = InMultiplier;
	check(multiplier > -1 && multiplier < 4);
	// 0 1 2 3 maps to 0 1 2 4
	multiplier = (multiplier + (multiplier - 1) / 2);
	return (multiplier * 2 + 100);
}

int32 FGGDamageDealingInfo::UnitToValue(uint8 FromUnit)
{
	int32 value = FromUnit;
	check(value > -1 && value < 4);
	// 0 1 2 3 maps to 0 1 2 4
	value = (value + (value - 1) / 2);
	return value;
}

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

uint8 FGGDamageReceivingInfo::ConvertDeltaPosition(const FVector& InDeltaPosition)
{
	uint8 Out = 0;
	if (InDeltaPosition.Z > FGGDamageReceivingInfo::HitMargin)
	{
		// case 123
		Out = 7;
	}
	else if (InDeltaPosition.Z < -FGGDamageReceivingInfo::HitMargin)
	{
		// case 789
		Out = 1;
	}
	else
	{
		// case 456
		Out = 4;
	}
	if (InDeltaPosition.Y > FGGDamageReceivingInfo::HitMargin)
	{
		return Out;
	}
	else if (InDeltaPosition.Y < -FGGDamageReceivingInfo::HitMargin)
	{
		Out += 2;
	}
	else
	{
		Out += 1;
	}
	return Out;
}

int32 FGGDamageReceivingInfo::PrecisionMultiplierToPercent(uint8 InMultiplier)
{
	int32 multiplier = InMultiplier;
	check(multiplier > -1 && multiplier < 4);
	// 0 1 2 3 maps to 0 1 2 4
	multiplier = (multiplier + (multiplier - 1) / 2);
	return (multiplier * 2 + 100);
}

int32 FGGDamageReceivingInfo::UnitToValue(uint8 FromUnit)
{
	int32 value = FromUnit;
	check(value > -1 && value < 4);
	// 0 1 2 3 maps to 0 1 2 4
	value = (value + (value - 1) / 2);
	return value;
}

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