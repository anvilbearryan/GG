#pragma once

#include "PaperFlipbook.h"
#include "GGFunctionLibrary.generated.h"

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

    // Direct damage = the amount of red to red decrease in between taking damage
    UPROPERTY()
    int32 DirectValue;
    // Indirect damage = the amount of red to green decrease in between taking damage
    UPROPERTY()
    int32 IndirectValue;
    UPROPERTY()
    uint8 ImpactDirection;
    UPROPERTY()
    TEnumAsByte<EGGDamageType::Type> Type;
    
    FGGDamageInformation(){}
    
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
**** Game type for animation ****
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
namespace EGGActionCategorySpecific
{
    // Specific states contains enumeration for 3 possibilities with each containing 5 possible direction named in WSAD space
    enum Type
    {
        Mode0_S = 0,
        Mode0_SD,
        Mode0_D,
        Mode0_DW,
        Mode0_W,
        Mode1_S,
        Mode1_SD,
        Mode1_D,
        Mode1_DW,
        Mode1_W,
        Mode2_S,
        Mode2_SD,
        Mode2_D,
        Mode2_DW,
        Mode2_W,
        TYPES_COUNT
    };
}

USTRUCT(BlueprintType)
struct FGGAnimationState
{
    GENERATED_BODY()
    
    UPROPERTY()
        TEnumAsByte<EGGActionCategorySpecific::Type> SecondaryState;
    UPROPERTY()
        UPaperFlipbook* PlaybackFlipbook;
    UPROPERTY()
        TEnumAsByte<EGGAnimationStateEndType::Type> StateEndType;
    UPROPERTY()
        uint32 bMustPlayTillEnd : 1;
};

USTRUCT(BlueprintType)
struct FGGAnimationStateArray
{
    GENERATED_BODY()
    
    UPROPERTY()
        TEnumAsByte<EGGActionCategory::Type> PrimaryState;
    UPROPERTY()
        TArray<FGGAnimationState> States;
};

UCLASS()
class GG_API UGGFunctionLibrary: public UObject
{
	GENERATED_BODY()

public:
	/** Query UWorld for overlap and add any new result to the supplied array */
	static bool WorldOverlapMultiActorByChannel(
		UWorld* World, 
		const FVector & Pos, 
		ECollisionChannel TraceChannel, 
		const FCollisionShape & CollisionShape, 
		TArray<AActor*>& OutOverlaps
		);
};