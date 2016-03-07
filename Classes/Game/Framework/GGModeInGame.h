// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameMode.h"
#include "GGModeInGame.generated.h"

class AGGCharacter;
class APlayerController;

USTRUCT()
struct FSpawnRequestDetail
{
	GENERATED_BODY();

	TWeakObjectPtr<APlayerController> Controller;
	TAssetSubclassOf<AGGCharacter>* CharacterClass;
	uint32 SaveData;

	FSpawnRequestDetail() {}
	FSpawnRequestDetail(
		APlayerController* InController, TAssetSubclassOf<AGGCharacter>* InClass, uint32 InSaveData)
	{
		Controller = InController;
		CharacterClass = InClass;
		SaveData = InSaveData;
	}
};

UCLASS()
class GG_API AGGModeInGame : public AGameMode
{
	GENERATED_BODY()

public:
	// Speicfication
	UPROPERTY(EditDefaultsOnly, Category=GameSetting)
		TAssetSubclassOf<AGGCharacter> AssaultBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> ReconBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> EngineerBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> DemolitionBP;

	// Level state
	UPROPERTY(BlueprintReadWrite)
		FVector CheckpointPosition;

	TArray<FSpawnRequestDetail, TInlineAllocator<4>> SpawnRequestQueue;

	virtual void PostLogin(APlayerController* InController) override;

	void HandleClientSpawnRequest(APlayerController* InController, uint8 InCharacterClass, uint32 InCharacterSaveData);

	UFUNCTION()
		void InitiateCharacterAssetLoadingSequence();
	UFUNCTION()
		void OnLoadedCharacterAsset();

	void SpawnCharacterForController(APlayerController* InController, UClass* InClass);

	FVector GetCharacterSpawnPosition(APlayerController* InController) const;
	

};
