// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "object_mover.generated.h"


UCLASS( ClassGroup=(ObjectMover), meta=(BlueprintSpawnableComponent) )
class SIM_VISUALIZER_API Uobject_mover : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	Uobject_mover();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector starting_position;
	
	UPROPERTY(EditAnywhere)
	FVector direction;
	UPROPERTY(EditAnywhere)
	float velocity; // cm/sn 
	UPROPERTY(EditAnywhere)
	int max_distance; //cm

		
};
