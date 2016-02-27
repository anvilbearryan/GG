// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Data/GGProjectileData.h"



void UGGProjectileData::RecalculateCaches()
{
	Gravity_Internal = GravityDirection.GetSafeNormal() * GravityStrength;
}

