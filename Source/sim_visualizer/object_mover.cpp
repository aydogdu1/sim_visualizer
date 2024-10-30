// Fill out your copyright notice in the Description page of Project Settings.


#include "object_mover.h"

// Sets default values for this component's properties
Uobject_mover::Uobject_mover()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void Uobject_mover::BeginPlay()
{
	Super::BeginPlay();

	starting_position = GetRelativeLocation();
	direction.Normalize();

	
}


// Called every frame
void Uobject_mover::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FVector distance_to_start = (GetRelativeLocation() + (velocity*direction)) - starting_position;

	if (distance_to_start.Length() > max_distance) {direction = direction * -1;}

	SetRelativeLocation(GetRelativeLocation() + (DeltaTime*velocity*direction));

}

