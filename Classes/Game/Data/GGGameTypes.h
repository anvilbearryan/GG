// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GGGameTypes.generated.h"

UENUM(BlueprintType)
namespace EGGShape
{
	enum Type
	{
		//	Just for exposing to BP...
		Line = 0,
		Box,
		Sphere,
		Capsule,
		TYPES_COUNT
	};
}
/** Placeholder struct for configuring FCollisionShape in Blueprints */
USTRUCT(BlueprintType)
struct FGGCollisionShapeParser
{
	GENERATED_BODY()

		FGGCollisionShapeParser() {}
	UPROPERTY(EditAnywhere, Category = "GGCollisionShape")
		TEnumAsByte<EGGShape::Type> Shape;
	UPROPERTY(EditAnywhere, Category = "GGCollisionShape")
		FVector HalfExtent;

	FORCEINLINE FCollisionShape ConvertToEngineShape()
	{
		switch (Shape)
		{
		case EGGShape::Box:
			return FCollisionShape::MakeBox(HalfExtent);
		case EGGShape::Sphere:
			return FCollisionShape::MakeSphere(HalfExtent.GetAbsMax());
		case EGGShape::Capsule:
			return FCollisionShape::MakeCapsule(FMath::Max(HalfExtent.X, HalfExtent.Y), HalfExtent.Z);
		}
		// other cases are un-defined
		UE_LOG(GGWarning, Warning, TEXT("No sensible FCollisionShape can be constructed from this struct"));
		return FCollisionShape();
	}
};

/**
***** Game types for damage ****
*/
UENUM(BlueprintType)
namespace EGGDamageType
{
	enum Type
	{
		//	Just for exposing to BP...
		Standard = 0,
		Slash = 1,
		TYPES_COUNT
	};
}

USTRUCT(BlueprintType)
struct FGGDamageDealingInfo
{
	GENERATED_BODY()

	// make it 10 with member0 == member5 so that they corresponds to numpad values and make sense
	static const FVector2D Directions[10];
	static const float HitMargin;
	// Direct damage = the amount of red to red decrease in between taking damage
	UPROPERTY(EditAnywhere)
		int32 DirectValue;
	// Indirect damage = the amount of red to green decrease in between taking damage
	UPROPERTY(EditAnywhere)
		int32 IndirectValue;
	UPROPERTY()
		uint8 ImpactDirection;
	UPROPERTY(EditAnywhere)
		TEnumAsByte<EGGDamageType::Type> Type;
	UPROPERTY()
		APlayerState* CauserPlayerState;
	
	static uint8 ConvertDeltaPosition(const FVector& InDeltaPosition)
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
	FGGDamageDealingInfo() {}
	FGGDamageDealingInfo(uint32 DamageData, APlayerState* InPlayerState)
	{		
		const uint32 TypeBits = 15;
		Type = (EGGDamageType::Type)(DamageData & TypeBits);

		const uint32 ImpactDirectionBits = 15 << 4;
		ImpactDirection = (DamageData & ImpactDirectionBits) >> 4;

		const uint32 IndirectValueBits = 4095 << 8;
		IndirectValue = (DamageData & IndirectValueBits) >> 8;

		const uint32 DirectValueBits = 4095 << 20;
		IndirectValue = (DamageData & DirectValueBits) >> 20;

		CauserPlayerState = InPlayerState;
	}
	FVector2D GetImpactDirection()
	{
		if (ImpactDirection < 10)
		{
			return FGGDamageDealingInfo::Directions[ImpactDirection];
		}
		UE_LOG(GGWarning, Warning, TEXT("Invalid direction contained in DamageData struct! Array out of bounds!"));
		return FVector2D();
	}
	uint32 GetCompressedData() const
	{
		uint32 result = 0;
		result |= DirectValue;
		result = result << 12;
		result |= IndirectValue;
		result = result << 12;
		result |= ImpactDirection;
		result = result << 4;
		result |= Type;
		return result;
	}
};

USTRUCT(BlueprintType)
struct FGGDamageReceivingInfo
{
	GENERATED_BODY()

	// make it 10 with member0 == member5 so that they corresponds to numpad values and make sense
	static const FVector2D Directions[10];
	static const float HitMargin;
	
	// the default value of all types of damage if unspecified
	static const int32 BasicDamageLevel = 100;
	// Direct damage = the amount of red to red decrease in between taking damage
	UPROPERTY(EditAnywhere)
		int32 DirectValue;
	// Indirect damage = the amount of red to green decrease in between taking damage
	UPROPERTY(EditAnywhere)
		int32 IndirectValue;
	UPROPERTY()
		uint8 ImpactDirection;
	UPROPERTY(EditAnywhere)
		TEnumAsByte<EGGDamageType::Type> Type;

	static uint8 ConvertDeltaPosition(const FVector& InDeltaPosition)
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
	FGGDamageReceivingInfo() 
	{
		DirectValue = FGGDamageReceivingInfo::BasicDamageLevel;
		IndirectValue = 0;
		Type = EGGDamageType::Standard;
	}
	FGGDamageReceivingInfo(uint32 DamageData)
	{
		const uint32 TypeBits = 15;
		Type = (EGGDamageType::Type)(DamageData & TypeBits);

		const uint32 ImpactDirectionBits = 15 << 4;
		ImpactDirection = (DamageData & ImpactDirectionBits) >> 4;

		const uint32 IndirectValueBits = 4095 << 8;
		IndirectValue = (DamageData & IndirectValueBits) >> 8;

		const uint32 DirectValueBits = 4095 << 20;
		IndirectValue = (DamageData & DirectValueBits) >> 20;
	}
	FVector2D GetImpactDirection() const
	{
		if (ImpactDirection < 10)
		{
			return FGGDamageReceivingInfo::Directions[ImpactDirection];
		}
		UE_LOG(GGWarning, Warning, TEXT("Invalid direction contained in DamageData struct! Array out of bounds!"));
		return FVector2D();
	}
	uint32 GetCompressedData() const
	{
		uint32 result = 0;
		result |= DirectValue;
		result = result << 12;
		result |= IndirectValue;
		result = result << 12;
		result |= ImpactDirection;
		result = result << 4;
		result |= Type;
		return result;
	}
};

UENUM(BlueprintType)
namespace EGGActionCategory
{
	// Categories describes the general behaviour of any unit in games
	enum Type
	{
		Locomotion = 0,  //
		Attack = 1,   //
		Defend = 2, //
		Evade = 3,   //
		Damaged = 4,
		Death = 5,
		Special = 6,
		Reserved = 7,
		TYPES_COUNT
	};
}

/**
******** BEGIN ENEMY AI TYPES ********
*/

/** Possible states of an enemy's player sensing behaviour */
UENUM(BlueprintType)
namespace EGGAISensingState
{
	enum Type
	{
		Inactive = 0,
		Active = 1,
		Alert = 2,
		TYPE_COUNT
	};
}

/** Generalized phases of AI action */
UENUM(BlueprintType)
namespace EGGAIActionState
{
	enum Type
	{
		Inactive = 0,
		Patrol = 1,
		PrepareAttack = 2,
		Attack = 3,
		Evade = 4,
		TYPE_COUNT
	};
}

/**
******** END ENEMY AI TYPES ********
*/

/**
******** BEGIN 2D GEOMETRY TYPES ********
*/

USTRUCT(BlueprintType)
struct FGGBox2D
{
	GENERATED_BODY()
		FVector2D Centre;
	FVector2D HalfExtent;

	FGGBox2D() {}
	FGGBox2D(const FVector2D &InCentre, const FVector2D &InHalfExtent)
	{
		Centre = InCentre;
		HalfExtent = InHalfExtent;
	}

	FORCEINLINE FVector2D GetTopLeft() const
	{
		return FVector2D(Centre.X - HalfExtent.X, Centre.Y + HalfExtent.Y);
	}
	FORCEINLINE FVector2D GetTopRight() const
	{
		return FVector2D(Centre.X + HalfExtent.X, Centre.Y + HalfExtent.Y);
	}
	FORCEINLINE FVector2D GetBotLeft() const
	{
		return FVector2D(Centre.X - HalfExtent.X, Centre.Y - HalfExtent.Y);
	}
	FORCEINLINE FVector2D GetBotRight() const
	{
		return FVector2D(Centre.X + HalfExtent.X, Centre.Y - HalfExtent.Y);
	}

	FORCEINLINE bool ContainsPoint(const FVector2D &InPoint) const
	{
		/** explicitly separated into 4 AND conditions allows early termination, tiny immature optimisation ftw */
		return
			InPoint.X <= Centre.X + HalfExtent.X && InPoint.X >= Centre.X - HalfExtent.X
			&&
			InPoint.Y <= Centre.Y + HalfExtent.Y && InPoint.Y >= Centre.Y - HalfExtent.Y;
	}

	FORCEINLINE bool ContainsBox(const FGGBox2D &InBox) const
	{
		return ContainsPoint(InBox.GetBotLeft()) && ContainsPoint(InBox.GetTopRight());
	}

	FORCEINLINE bool IsDisjoint(const FGGBox2D &InBox) const
	{
		return HalfExtent.X + InBox.HalfExtent.X < FMath::Abs(Centre.X - InBox.Centre.X)
			&&
			HalfExtent.Y + InBox.HalfExtent.Y < FMath::Abs(Centre.Y - InBox.Centre.Y);
	}
};

/**
******** END 2D GEOMETRY TYPES ********
*/

class UGGPooledSpriteComponent;
class UGGProjectileData;
/** Contains information necessary in updating projectile's state and handling events*/
USTRUCT()
struct FLaunchedProjectile
{
	GENERATED_BODY()

	// safe guard in case sprite component is destroyed somehow
	UGGPooledSpriteComponent* SpriteBody;
	// this is stored to make delayed launch possible
	FVector LaunchDirection;
	FVector ContinualAcceleration;
	// since we are using PaperSpriteComponent, they do not store velocity by default unlike an AActor
	FVector CurrentVelocity;
	UGGProjectileData* ProjectileData;
	float Lifespan;
	float SpawnTime;
	int8 CurrentCollisionCount;

	FLaunchedProjectile()
	{
		SpriteBody = nullptr;
		LaunchDirection = FVector::ZeroVector;
		ContinualAcceleration = FVector::ZeroVector;
		ProjectileData = nullptr;
		CurrentCollisionCount = 0;
		Lifespan = 0;
		SpawnTime = 0;
	}

	FLaunchedProjectile(UGGPooledSpriteComponent* body, const FVector& direction, UGGProjectileData* data, float time);	
};

UCLASS()
class GG_API UGGGameTypes : public UObject
{
	GENERATED_BODY()
};
