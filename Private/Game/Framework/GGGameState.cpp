// Fill out your copyright notice in the Description page of Project Settings.

#include "GG.h"
#include "Game/Framework/GGGameState.h"

TArray<AGGCharacter*>& AGGGameState::GetCharacterList()
{
    return CharacterList;
}


