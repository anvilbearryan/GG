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
        NotSpecified = 0,   // Case for which we should have information to figure out from elsewhere, specifically reserved
        Mode0_Horizontal = 1,
        Mode0_Vertical = 2,
        Mode1_Horizontal,
        Mode1_Vertical,
        Mode2_Horizontal,
        Mode2_Vertical,
        Mode3_Horizontal,
        Mode3_Vertical,
        Mode4_Horizontal,
        Mode4_Vertical,
        Mode5_Horizontal,
        Mode5_Vertical,
        Mode6_Horizontal,
        Mode6_Vertical,
        Mode7_Horizontal,
        Mode7_Vertical,
        TYPES_COUNT
    };
}

/** 1D Digital blend space state */
USTRUCT(BlueprintType)
struct FGGAnimationState
{
    GENERATED_BODY()
    /** Picker enum for convinience to replicate the blend spaces through an index, also indicates blend direction */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        TEnumAsByte<EGGActionCategorySpecific::Type> SecondaryState;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        uint8 bMustPlayTillEnd : 1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        TEnumAsByte<EGGAnimationStateEndType::Type> StateEndType;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        UPaperFlipbook* NeutralFlipbook;
    /** Leave empty (=null) if we do not wish to make this a blend-space state*/
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        UPaperFlipbook* PositiveFlipbook;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        UPaperFlipbook* NegativeFlipbook;
    
    FORCEINLINE bool ShouldBlendHorizontal()
    {
        return (SecondaryState % 2) == 1;
    }
};

USTRUCT(BlueprintType)
struct FGGAnimationStateArray
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
        TEnumAsByte<EGGActionCategory::Type> PrimaryState;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Animation")
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