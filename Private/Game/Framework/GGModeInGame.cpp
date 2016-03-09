// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Framework/GGModeInGame.h"
#include "Game/Actor/GGCharacter.h"
#include "Game/Framework/GGGamePlayerController.h"
#include "Global/GGGameInstance.h"

void AGGModeInGame::PostLogin(APlayerController* InController)
{
	Super::PostLogin(InController);
	if (InController) 
	{
		static_cast<AGGGamePlayerController*>(InController)->ClientDisplaySpawnUI();;
	}
}

void AGGModeInGame::HandleClientSpawnRequest(APlayerController* InController, uint8 InCharacterClass, uint32 InCharacterSaveData)
{
	UWorld* world = GetWorld();
	if (world)
	{
		TAssetSubclassOf<AGGCharacter>* locCharacterAssetClassPtr = nullptr;
		// assault
		if (InCharacterClass == 0)
		{
			locCharacterAssetClassPtr = &AssaultBP;
		} // recon
		else if (InCharacterClass == 1)
		{
			locCharacterAssetClassPtr = &ReconBP;
		} // engineer
		else if (InCharacterClass == 2)
		{
			locCharacterAssetClassPtr = &EngineerBP;
		} // demolition
		else if (InCharacterClass == 3)
		{
			locCharacterAssetClassPtr = &DemolitionBP;
		}
		auto& locCharacterAssetClassRef = *locCharacterAssetClassPtr;
		if (locCharacterAssetClassRef.IsPending())
		{
			SpawnRequestQueue.Add(
				FSpawnRequestDetail(InController, locCharacterAssetClassPtr, InCharacterSaveData));
			// if we have no pending load requests, process async load immediately
			if (SpawnRequestQueue.Num() == 1) 
			{				
				InitiateCharacterAssetLoadingSequence();
			}
		}
		else
		{			
			SpawnCharacterForController(InController, locCharacterAssetClassRef.Get());			
		}		
	}
}

void AGGModeInGame::InitiateCharacterAssetLoadingSequence()
{
	UE_LOG(GGMessage, Log, TEXT("Starting async load"));
	UGGGameInstance* locGameInstance = static_cast<UGGGameInstance*>(GetGameInstance());
	FStreamableManager& StreamableManager = locGameInstance->StreamableManager;
	StreamableManager.RequestAsyncLoad(SpawnRequestQueue[0].CharacterClass->ToStringReference(),
		FStreamableDelegate::CreateUObject(this, &AGGModeInGame::OnLoadedCharacterAsset));
}

void AGGModeInGame::OnLoadedCharacterAsset()
{
	check(SpawnRequestQueue.Num() > 0);
	FSpawnRequestDetail& RequestToProcess = SpawnRequestQueue[0];	
	SpawnCharacterForController(RequestToProcess.Controller.Get(), RequestToProcess.CharacterClass->Get());
	SpawnRequestQueue.RemoveAt(0, 1, false);
	if (SpawnRequestQueue.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			this, &AGGModeInGame::InitiateCharacterAssetLoadingSequence);
	}
}

void AGGModeInGame::SpawnCharacterForController(APlayerController* InController, UClass* InClass)
{
	if (InController == nullptr)
	{
		UE_LOG(GGWarning, Warning, TEXT("CharacterAsset loaded but controller requesting spawn no longer exists"));
		return;
	}
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FTransform spawnTransform;
	spawnTransform.SetLocation(GetCharacterSpawnPosition(InController));

	AGGCharacter* character = GetWorld()->SpawnActor<AGGCharacter>(InClass, spawnTransform, spawnParams);
	if (character)
	{
		InController->Possess(character);
		if (InController->IsLocalPlayerController())
		{
			InController->OnRep_Pawn();
		}
	}
}

FVector AGGModeInGame::GetCharacterSpawnPosition(APlayerController* InController) const
{
	return CheckpointPosition;
}

void AGGModeInGame::OnPlayerKilledByMinion(const class APlayerState* playerState, const class AGGMinionBase* minion)
{

}

/* 
*	We may wish to give him hp/mp/score reward depending on the player's status.
*	While a reward could have been put within the minion, this increases the complexity to implement situational 
*	drops. Moreover, it brings unnecessary increase in number of components/variables repicated simultaneously.
*/
void AGGModeInGame::OnMinionKilledByPlayer(const class AGGMinionBase* minion, const class APlayerState* playerState)
{
	// analyze PlayerState and branch
}



