// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Framework/GGPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Game/Framework/GGGamePlayerController.h"

void AGGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGGPlayerState, PlayerScore, COND_OwnerOnly);
}

void AGGPlayerState::AddToPlayerScore(int32 value)
{
	if (value == 0)
	{
		return;
	}
	PlayerScore += value;
	if (GetNetMode() == NM_ListenServer)
	{
		OnRep_PlayerScore();
	}
}

void AGGPlayerState::OnRep_PlayerScore()
{
	AGGGamePlayerController* controller = static_cast<AGGGamePlayerController*>(
		GetWorld()->GetFirstPlayerController());
	if (controller && controller->IsLocalController())
	{
		controller->UpdateScoreDisplay(PlayerScore);
	}
}