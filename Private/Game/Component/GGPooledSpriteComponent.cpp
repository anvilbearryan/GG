// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Component/GGPooledSpriteComponent.h"

UGGPooledSpriteComponent::UGGPooledSpriteComponent() : Super()
{
	SetAbsolute(true, true, true);
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void UGGPooledSpriteComponent::PreCheckout()
{
	SetVisibility(true, true);
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
}

void UGGPooledSpriteComponent::PreCheckin()
{
	SetVisibility(false, true);
	SetSprite(nullptr);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);	
}