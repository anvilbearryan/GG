// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Framework/GGGamePlayerController.h"
#include "Game/Framework/GGModeInGame.h"
#include "Game/Component/GGDamageReceiveComponent.h"
#include "Game/Actor/GGCharacter.h"

// ********************************

// Level start interface
const float AGGGamePlayerController::REQUEST_TIMEOUT_SECONDS = 3.5f;
void AGGGamePlayerController::ClientDisplaySpawnUI_Implementation()
{
	DisplaySpawnUI();
}

void AGGGamePlayerController::PickClassToSpawn(int32 choice)
{
	if (bSentSpawnRequest || GetPawn() != nullptr)
	{
		return;
	}
	bSentSpawnRequest = true;
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, 
		 &AGGGamePlayerController::OnSpawnRequestTimeOut, AGGGamePlayerController::REQUEST_TIMEOUT_SECONDS);
	// TODO: inject information of character save into RPC (now "0")
	ServerSpawnRequestFromClient((uint8) choice, 0);
}

void AGGGamePlayerController::OnSpawnRequestTimeOut()
{
	bSentSpawnRequest = false;
}

bool AGGGamePlayerController::ServerSpawnRequestFromClient_Validate(
	uint8 InCharacterClass, uint32 InCharacterSaveData)
{
	return true;
}

void AGGGamePlayerController::ServerSpawnRequestFromClient_Implementation(
	uint8 InCharacterClass, uint32 InCharacterSaveData)
{
	if (bServerReceivedSpawnRequest)
	{
		return;
	}
	bServerReceivedSpawnRequest = true;
	AGGModeInGame* gameMode = GetWorld()->GetAuthGameMode<AGGModeInGame>();
	if (gameMode)
	{
		gameMode->HandleClientSpawnRequest(this, InCharacterClass, InCharacterSaveData);
	}
}

void AGGGamePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	LocalOnPossessedCharacter();		
}

void AGGGamePlayerController::LocalOnPossessedCharacter()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	LocalOnPossessedCharacter_BP();
	
	AGGCharacter* locPawn = static_cast<AGGCharacter*>(GetPawn());
	UGGDamageReceiveComponent* locHealth = !!locPawn ? locPawn->HealthComponent.Get() : nullptr;
	if (locHealth)
	{
		UpdateHealthDisplay(locHealth->GetCurrentHp(), locHealth->Hp_Max);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("NO HEALTH COMPONENT"));
	}
	UpdateScoreDisplay(0);
}

// ********************************

// Respawn interface
bool AGGGamePlayerController::ServerRequestRespawn_Validate()
{
	return true;
}

void AGGGamePlayerController::ServerRequestRespawn_Implementation()
{
	ClientOnRespawn();
}

void AGGGamePlayerController::ClientOnRespawn_Implementation()
{
	
	ClientOnRespawn_BP();
}

// ********************************

// UI update