// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Data/GGMeleeAttackData.h"

void UGGMeleeAttackData::RecalculateCaches()
{
	UE_LOG(GGMessage, Log, TEXT("CalculateCaches"));
	TimeMark_EndStartup = Startup;
	SumActiveDuration = 0.f;
	for (auto &Definition : ActiveDefinitions)
	{
		Definition.AttackShapeInternal = Definition.AttackShape.ConvertToEngineShape();
		SumActiveDuration += Definition.Duration;
	}
	
	TimeMark_EndActive = TimeMark_EndStartup + SumActiveDuration;
	TimeMark_FullDuration = TimeMark_EndActive + HardCooldown + SoftCooldown;
	TimeMark_MinDuration = TimeMark_EndActive + HardCooldown;

	TimeMark_BeginComboable = ComboWindow > 0.f ? ComboLag : TimeMark_MinDuration;
	TimeMark_EndComboable = ComboWindow > 0.f ? TimeMark_BeginComboable + ComboWindow : TimeMark_FullDuration;	
}


