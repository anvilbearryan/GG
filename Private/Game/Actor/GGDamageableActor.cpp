// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "GGDamageableActor.h"


// Sets default values
AGGDamageableActor::AGGDamageableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGGDamageableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGGDamageableActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

