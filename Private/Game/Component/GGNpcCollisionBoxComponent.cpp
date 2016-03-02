// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGNpcCollisionBoxComponent.h"
#include "Game/Actor/GGCharacter.h"

UGGNpcCollisionBoxComponent::UGGNpcCollisionBoxComponent()
{
	bGenerateOverlapEvents = true;

}

void UGGNpcCollisionBoxComponent::OnOverlapCharacter(AGGCharacter* InCharacter)
{
	if (InCharacter)
	{
		// take damage
		InCharacter->LocalReceiveDamage(0);
	}
}