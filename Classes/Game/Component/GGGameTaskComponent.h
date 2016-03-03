// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GGGameTaskComponent.generated.h"

template <typename T>
struct FShortGameTask
{
	//GENERATED_BODY()

	float CurrentTime;
	float TotalDuration;

	TWeakObjectPtr<T> ConcernedObject;
	void (T::*FunctionPtr)(float);

	bool IsActiveTask() const
	{
		return ConcernedObject.IsValid() && CurrentTime < TotalDuration;
	}

	void TickTask(float DeltaTime)
	{
		CurrentTime += DeltaTime;
		T* object = ConcernedObject.Get();
		(object->(*FunctionPtr))(CurrentTime);
	}
};

/** Our tick task component, keeps an array of current active tasks to execute */
UCLASS()
class GG_API UGGGameTaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGGGameTaskComponent();

	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;	
	
};
