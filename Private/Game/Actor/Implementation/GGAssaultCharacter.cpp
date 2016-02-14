// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Actor/Implementation/GGAssaultCharacter.h"


void AGGAssaultCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();


}

void AGGAssaultCharacter::ReceiveDamage(int32 DamageData)
{
	Super::ReceiveDamage(DamageData);
	// ask damage receiving component to handle it
}