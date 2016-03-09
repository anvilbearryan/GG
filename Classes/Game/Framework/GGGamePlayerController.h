// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GGGamePlayerController.generated.h"


UCLASS()
class GG_API AGGGamePlayerController : public APlayerController
{
	GENERATED_BODY()

	static const float REQUEST_TIMEOUT_SECONDS; 
	FTimerHandle SpawnTimerHandle;
	uint8 bSentSpawnRequest : 1;
	uint8 bServerReceivedSpawnRequest : 1;

public:	
	// ********************************
	
	// Level start interface
	UFUNCTION(Client, Reliable)
		void ClientDisplaySpawnUI();
	void ClientDisplaySpawnUI_Implementation();
	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow")
		void DisplaySpawnUI();

	UFUNCTION(BlueprintCallable, Category ="GameFlow")
		void PickClassToSpawn(int32 choice);
	UFUNCTION()
		void OnSpawnRequestTimeOut();

	UFUNCTION(Server, WithValidation, Reliable)
		void ServerSpawnRequestFromClient(uint8 InCharacterClass, uint32 InCharacterSaveData);
	bool ServerSpawnRequestFromClient_Validate(uint8 InCharacterClass, uint32 InCharacterSaveData);
	void ServerSpawnRequestFromClient_Implementation(uint8 InCharacterClass, uint32 InCharacterSaveData);

	// For HUD initialization, called manually on listen server
	virtual void OnRep_Pawn() override;
	void LocalOnPossessedCharacter();
	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow", meta=(DisplayName ="LocalOnPossessCharacter"))
		void LocalOnPossessedCharacter_BP();

	// ********************************

	// Respawn interface (TODO)
	UFUNCTION(Server, WithValidation, Reliable)
		void ServerRequestRespawn();
	bool ServerRequestRespawn_Validate();
	void ServerRequestRespawn_Implementation();

	UFUNCTION(Client, Reliable)
		void ClientOnRespawn();
	void ClientOnRespawn_Implementation();
	UFUNCTION(BlueprintImplementableEvent, Category = "GameFlow", meta = (DisplayName = "ClientOnRespawn"))
		void ClientOnRespawn_BP();

	// ********************************

	// UI update
	UFUNCTION(BlueprintImplementableEvent, Category = "GameUI")
		void UpdateHealthDisplay(int32 NewCurrentHp, int32 NewMaxHp);
	UFUNCTION(BlueprintImplementableEvent, Category = "GameUI")
		void UpdateScoreDisplay(int32 NewScore);

	// ********************************

	// Damage Receiving / Dealing interface
	UFUNCTION(BlueprintImplementableEvent, Category=DamageEvents)
		void OnLocalCharacterDealDamage();
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnLocalCharacterReceiveDamage(int32 InDamageReceived);
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnRemoteCharacterDealDamage();
	UFUNCTION(BlueprintImplementableEvent, Category = DamageEvents)
		void OnRemoteCharacterReceiveDamage(int32 InDamageReceived);

	// ********************************

	// General utility
	UFUNCTION(Client, Unreliable)
		void ClientPlaySound2D(USoundCue* sound);
	void ClientPlaySound2D_Implementation(USoundCue* sound);
};
