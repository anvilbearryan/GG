// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Data/GGGameTypes.h"

const FVector2D FGGDamageInformation::Directions[10] =
{ FVector2D(0.f,0.f),
//	1	2	3
FVector2D(1.f,1.f), FVector2D(0.f,1.f), FVector2D(-1.f, 1.f),
//	4	5	6
FVector2D(1.f,0.f), FVector2D(0.f,0.f), FVector2D(-1.f, 0.f),
//	7	8	9
FVector2D(1.f,-1.f), FVector2D(0.f,-1.f), FVector2D(-1.f, -1.f)
};
const float FGGDamageInformation::HitMargin = 6.25f;