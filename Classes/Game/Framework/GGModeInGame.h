// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameMode.h"
#include "GGModeInGame.generated.h"

class AGGCharacter;
class APlayerController;
/** Struct representing the information of a spawn character request from client*/
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
	/* 
	*	Character spawning sepcification - BPs representing the character we shall spawn
	*/
	UPROPERTY(EditDefaultsOnly, Category=GameSetting)
		TAssetSubclassOf<AGGCharacter> AssaultBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> ReconBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> EngineerBP;
	UPROPERTY(EditDefaultsOnly, Category = GameSetting)
		TAssetSubclassOf<AGGCharacter> DemolitionBP;

	/*
	*	The world positions characters are spawned at
	*/
	UPROPERTY(BlueprintReadWrite)
		FVector CheckpointPosition;

	/*
	*	As character assets are loaded async, we need to store a queue of the spawn requests we have received
	*	to know what next to be processed.
	*/
	TArray<FSpawnRequestDetail, TInlineAllocator<4>> SpawnRequestQueue;

	/*
	*	AGameMode interface
	*/
	virtual void PostLogin(APlayerController* InController) override;

	/*
	*	Character spawning interface
	*/
	void HandleClientSpawnRequest(APlayerController* InController, uint8 InCharacterClass, uint32 InCharacterSaveData);

	UFUNCTION()
		void InitiateCharacterAssetLoadingSequence();
	UFUNCTION()
		void OnLoadedCharacterAsset();

	void SpawnCharacterForController(APlayerController* InController, UClass* InClass);

	FVector GetCharacterSpawnPosition(APlayerController* InController) const;
	
	/*
	*	Runtime game events
	*/
	void OnPlayerKilledByMinion(const class APlayerState* playerState, const class AGGMinionBase* minion);

	void OnMinionKilledByPlayer(const class AGGMinionBase* minion, const class APlayerState* playerState);

};
