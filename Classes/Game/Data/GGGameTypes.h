// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PaperFlipbook.h"
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
		TYPES_COUNT
	};
}

USTRUCT(BlueprintType)
struct FGGDamageInformation
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
	FGGDamageInformation() {}

	static FGGDamageInformation DecompressFromData(int32 DamageData)
	{
		FGGDamageInformation info = FGGDamageInformation();
		const uint32 TypeBits = 15;
		info.Type = (EGGDamageType::Type)(DamageData & TypeBits);

		const uint32 ImpactDirectionBits = 15 << 4;
		info.ImpactDirection = (DamageData & ImpactDirectionBits) >> 4;

		const uint32 IndirectValueBits = 4095 << 8;
		info.IndirectValue = (DamageData & IndirectValueBits) >> 8;

		const uint32 DirectValueBits = 4095 << 20;
		info.IndirectValue = (DamageData & DirectValueBits) >> 20;

		return info;
	}
	static uint8 ConvertDeltaPosition(const FVector& InDeltaPosition)
	{
		uint8 Out = 0;
		if (InDeltaPosition.Z > FGGDamageInformation::HitMargin)
		{
			// case 123
			Out = 7;
		}
		else if (InDeltaPosition.Z < -FGGDamageInformation::HitMargin)
		{
			// case 789
			Out = 4;
		}
		else
		{
			// case 456
			Out = 1;
		}
		if (InDeltaPosition.Y > FGGDamageInformation::HitMargin)
		{
			return Out;
		}
		else if (InDeltaPosition.Y < -FGGDamageInformation::HitMargin)
		{
			Out += 2;
		}
		else
		{
			Out += 1;
		}
		return Out;
	}
	FVector2D GetImpactDirection()
	{
		if (ImpactDirection < 10)
		{
			return FGGDamageInformation::Directions[ImpactDirection];
		}
		UE_LOG(GGWarning, Warning, TEXT("Invalid direction contained in DamageData struct! Array out of bounds!"));
		return FVector2D();
	}
	int32 GetCompressedData()
	{
		int32 result = 0;
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

/**
******** BEGIN ANIMATION STATE MACHINE TYPES ********
*/
UENUM(BlueprintType)
namespace EGGAnimationStateEndType
{
	//  An entity may stay in the same animation state for longer than the duration of the animation itself, this enum denotes the choices of action in such scenario
	enum Type
	{
		Pause = 0,  // stay in the last frame of the animation, e.g. take damage, jumps
		Loop = 1,   // loop..
		Revert = 2, //  back to what we were previously doing, e.g. special idles, taunts
		Exit = 3,   //  we quit this state with existing information to determine what follows
		TYPES_COUNT
	};
}

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

UENUM(BlueprintType)
namespace EGGActionMode
{
	// used within Animator to represent which mode we are in, multiplied in determing the "specific category" state to be used
	enum Type
	{
		Mode0 = 0,
		Mode1 = 1,
		Mode2 = 2,
		Mode3 = 3,
		Mode4 = 4,
		Mode5 = 5,
		Mode6 = 6,
		Mode7 = 7,
		TYPES_COUNT
	};
}

/** A combination of ActionMode and blend space direction, use in Editor for configuration convinience */
UENUM(BlueprintType)
namespace EGGActionCategorySpecific
{
	// Specific states contains all combination of EGGActionMode and horizontal / vertical mode
	enum Type
	{
		Mode0_Horizontal = 0,
		Mode0_Vertical = 1,
		Mode1_Horizontal = 2,
		Mode1_Vertical = 3,
		Mode2_Horizontal = 4,
		Mode2_Vertical = 5,
		Mode3_Horizontal = 6,
		Mode3_Vertical = 7,
		Mode4_Horizontal = 8,
		Mode4_Vertical = 9,
		Mode5_Horizontal = 10,
		Mode5_Vertical = 11,
		Mode6_Horizontal = 12,
		Mode6_Vertical = 13,
		Mode7_Horizontal = 14,
		Mode7_Vertical = 15,
		NotSpecified = 16,   // Case for which we should have information to figure out from elsewhere, specifically reserved
		TYPES_COUNT = 17
	};
}

/** 1D Digital blend space state */
USTRUCT(BlueprintType)
struct FGGAnimationState
{
	GENERATED_BODY()
		/** Picker enum for convinience to replicate the blend spaces through an index, also indicates blend direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TEnumAsByte<EGGActionCategorySpecific::Type> SecondaryState;
	// Used to determine whether we should blend playback position when changing inter-state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		uint8 bMustPlayTillEnd : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TEnumAsByte<EGGAnimationStateEndType::Type> StateEndType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UPaperFlipbook* NeutralFlipbook;
	/** Leave empty (=null) if we do not wish to make this a blend-space state*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UPaperFlipbook* PositiveFlipbook;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UPaperFlipbook* NegativeFlipbook;

	FORCEINLINE bool ShouldBlendHorizontal()
	{
		return (SecondaryState % 2) == 0;
	}
};

USTRUCT(BlueprintType)
struct FGGAnimationStateArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TEnumAsByte<EGGActionCategory::Type> PrimaryState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TArray<FGGAnimationState> States;
};
/**
******** END ANIMATION STATE MACHINE TYPES ********
*/

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

UCLASS()
class GG_API UGGGameTypes : public UObject
{
	GENERATED_BODY()
};
