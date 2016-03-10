// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GGGameTypes.generated.h"


/**
***** Damage logic types ****
*/
UENUM(BlueprintType)
enum class EGGDamageType : uint8
{
	//	Just for exposing to BP...
	Standard = 0,
	Slash = 1,
	TYPES_COUNT

};

USTRUCT(BlueprintType)
struct FGGDamageDealingInfo
{
	GENERATED_BODY()
private:
	// make it 10 with member0 == member5 so that they corresponds to numpad values and make sense
	static const FVector2D Directions[10];
	static const float HitMargin;
	static const int32 DirectDamageBaseMultiplier = 35;
	static const int32 IndirectDamageBaseMultiplier = 10;
public:
	UPROPERTY()
		uint8 ImpactDirection;
	UPROPERTY()
		EGGDamageType Type;	
	
	uint8 Direct_BaseMultiplier;	
	uint8 Direct_PrecisionMultiplier;	
	uint8 Direct_Unit;
		
	uint8 Indirect_BaseMultiplier;
	uint8 Indirect_PrecisionMultiplier;
	uint8 Indirect_Unit;

	UPROPERTY()
		APlayerState* CauserPlayerState;

	static uint8 ConvertDeltaPosition(const FVector& InDeltaPosition);
	static int32 PrecisionMultiplierToPercent(uint8 InMultiplier);
	static int32 UnitToValue(uint8 FromUnit);

	FGGDamageDealingInfo()
	{
		Direct_BaseMultiplier = 0;
		Direct_PrecisionMultiplier = 0;
		Direct_Unit = 0;

		Indirect_BaseMultiplier = 0;
		Indirect_PrecisionMultiplier = 0;
		Indirect_Unit = 0;

		Type = EGGDamageType::Standard;
	}
	FGGDamageDealingInfo(uint32 DamageData, APlayerState* InPlayerState)
	{		
		const uint32 TypeBits = 15;
		Type = EGGDamageType(DamageData & TypeBits);

		const uint32 ImpactDirectionBits = 15 << 4;
		ImpactDirection = (DamageData & ImpactDirectionBits) >> 4;

		const uint32 IndirectBits = ((4095 << 8) & DamageData) >> 8;
		Indirect_BaseMultiplier = (IndirectBits & 255);
		Indirect_PrecisionMultiplier = (IndirectBits & (3 << 8)) >> 8;
		Indirect_Unit = (IndirectBits & (3 << 10)) >> 10;

		const uint32 DirectBits = ((4095 << 20) & DamageData) >> 20;
		Direct_BaseMultiplier = (DirectBits & 255);
		Direct_PrecisionMultiplier = (DirectBits & (3 << 8)) >> 8;
		Direct_Unit = (DirectBits & (3 << 10)) >> 10;

		CauserPlayerState = InPlayerState;
	}
	FORCEINLINE FVector2D GetImpactDirection() const
	{
		if (ImpactDirection < 10)
		{
			return FGGDamageDealingInfo::Directions[ImpactDirection];
		}
		UE_LOG(GGWarning, Warning, TEXT("Invalid direction contained in DamageData struct! Array out of bounds!"));
		return FVector2D();
	}
	FORCEINLINE uint32 GetCompressedData() const
	{
		uint32 result = 0;
		result |= Direct_BaseMultiplier;
		result |= (Direct_PrecisionMultiplier << 8);
		result |= (Direct_Unit << 10);
		result = result << 12;
		
		result |= Indirect_BaseMultiplier;
		result |= (Indirect_PrecisionMultiplier << 8);
		result |= (Indirect_Unit << 10);
		result = result << 12;

		result |= ImpactDirection;
		result = result << 4;
		result |= (uint8) Type;
		/*
		UE_LOG(GGMessage, Log, TEXT("FDmaageDealing: Attempting to Convert the following DirectDmgInfo:"));
		UE_LOG(GGMessage, Log, TEXT("DirectBaseMultiplier: %d"), Direct_BaseMultiplier);
		UE_LOG(GGMessage, Log, TEXT("Direct_PrecisionMultiplier: %d"), Direct_PrecisionMultiplier);
		UE_LOG(GGMessage, Log, TEXT("Direct_Unit: %d"), Direct_Unit);
		UE_LOG(GGMessage, Log, TEXT("FDmaageDealing: Resulting compressed bits: %d"), result);		
		*/
		return result;
	}
	FORCEINLINE int32 GetDirectDamage() const
	{
		return (FGGDamageDealingInfo::DirectDamageBaseMultiplier * Direct_BaseMultiplier
			* FGGDamageDealingInfo::PrecisionMultiplierToPercent(Direct_PrecisionMultiplier)) / 100
			+ FGGDamageDealingInfo::UnitToValue(Direct_Unit);
	}
	FORCEINLINE int32 GetIndirectDamage() const
	{
		return (FGGDamageDealingInfo::IndirectDamageBaseMultiplier * Indirect_BaseMultiplier
			* FGGDamageDealingInfo::PrecisionMultiplierToPercent(Indirect_PrecisionMultiplier)) / 100
			+ FGGDamageDealingInfo::UnitToValue(Indirect_Unit);
	}
};

USTRUCT(BlueprintType)
struct FGGDamageReceivingInfo
{
	GENERATED_BODY()
private:
	// make it 10 with member0 == member5 so that they corresponds to numpad values and make sense
	static const FVector2D Directions[10];
	static const float HitMargin;
	static const int32 DirectDamageBaseMultiplier = 35;
	static const int32 IndirectDamageBaseMultiplier = 10;

public:	
	UPROPERTY()
		uint8 ImpactDirection;
	UPROPERTY(EditAnywhere)
		EGGDamageType Type;

	uint8 Direct_BaseMultiplier;
	uint8 Direct_PrecisionMultiplier;
	uint8 Direct_Unit;

	uint8 Indirect_BaseMultiplier;
	uint8 Indirect_PrecisionMultiplier;
	uint8 Indirect_Unit;

	static uint8 ConvertDeltaPosition(const FVector& InDeltaPosition);
	static int32 PrecisionMultiplierToPercent(uint8 InMultiplier);
	static int32 UnitToValue(uint8 FromUnit);

	FGGDamageReceivingInfo()
	{
		Type = EGGDamageType::Standard;
		Direct_BaseMultiplier = 0;
		Direct_PrecisionMultiplier = 0;
		Direct_Unit = 0;

		Indirect_BaseMultiplier = 0;
		Indirect_PrecisionMultiplier = 0;
		Indirect_Unit = 0;
	}
	FGGDamageReceivingInfo(uint32 DamageData)
	{
		const uint32 TypeBits = 15;
		Type = EGGDamageType(DamageData & TypeBits);

		const uint32 ImpactDirectionBits = 15 << 4;
		ImpactDirection = (DamageData & ImpactDirectionBits) >> 4;

		const uint32 IndirectBits = ((4095 << 8) & DamageData) >> 8;
		Indirect_BaseMultiplier = (IndirectBits & 255);
		Indirect_PrecisionMultiplier = (IndirectBits & (3 << 8)) >> 8;
		Indirect_Unit = (IndirectBits & (3 << 10)) >> 10;

		const uint32 DirectBits = ((4095 << 20) & DamageData) >> 20;
		Direct_BaseMultiplier = (DirectBits & 255);
		Direct_PrecisionMultiplier = (DirectBits & (3 << 8)) >> 8;
		Direct_Unit = (DirectBits & (3 << 10)) >> 10;
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
		result |= Direct_BaseMultiplier;
		result |= (Direct_PrecisionMultiplier << 8);
		result |= (Direct_Unit << 10);
		result = result << 12;

		result |= Indirect_BaseMultiplier;
		result |= (Indirect_PrecisionMultiplier << 8);
		result |= (Indirect_Unit << 10);
		result = result << 12;

		result |= ImpactDirection;
		result = result << 4;
		result |= (uint8) Type;
		return result;
	}

	FORCEINLINE int32 GetDirectDamage() const
	{
		return (FGGDamageReceivingInfo::DirectDamageBaseMultiplier * Direct_BaseMultiplier
			* FGGDamageReceivingInfo::PrecisionMultiplierToPercent(Direct_PrecisionMultiplier)) / 100
			+ FGGDamageReceivingInfo::UnitToValue(Direct_Unit);
	}
	FORCEINLINE int32 GetIndirectDamage() const
	{
		return (FGGDamageReceivingInfo::IndirectDamageBaseMultiplier * Indirect_BaseMultiplier
			* FGGDamageReceivingInfo::PrecisionMultiplierToPercent(Indirect_PrecisionMultiplier)) / 100
			+ FGGDamageReceivingInfo::UnitToValue(Indirect_Unit);
	}
};

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

/**
******** Entity States ********
*/
/** States of player character */
UENUM(BlueprintType)
enum class EGGActionCategory : uint8
{
	// Categories describes the general behaviour of any unit in games
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

/** Possible states of an enemy's player sensing behaviour */
UENUM(BlueprintType)
enum class EGGAISensingState : uint8
{
	Inactive = 0,
	Active = 1,
	Alert = 2,
	TYPE_COUNT
};

/** Generalized phases of AI action */
UENUM(BlueprintType)
enum class EGGAIActionState : uint8
{
	Inactive = 0,
	Patrol = 1,
	PrepareAttack = 2,
	Attack = 3,
	Evade = 4,
	TYPE_COUNT
};

/**
******** 2D Geometry Types ********
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

UENUM(BlueprintType)
enum class EGGShape : uint8
{
	//	Just for exposing to BP...
	Line = 0,
	Box,
	Sphere,
	Capsule,
	TYPES_COUNT
};

/** Placeholder struct for configuring FCollisionShape in Blueprints */
USTRUCT(BlueprintType)
struct FGGCollisionShapeParser
{
	GENERATED_BODY()

		FGGCollisionShapeParser() {}
	UPROPERTY(EditAnywhere, Category = "GGCollisionShape")
		EGGShape Shape;
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
******** In-game numeric value types ********
*/
UENUM(BlueprintType)
enum class EGGLootTypes : uint8
{
	//	Just for exposing to BP...
	None,
	Hp,
	Mp,
	Score,
	TYPES_COUNT
};


UCLASS()
class GG_API UGGGameTypes : public UObject
{
	GENERATED_BODY()
};
