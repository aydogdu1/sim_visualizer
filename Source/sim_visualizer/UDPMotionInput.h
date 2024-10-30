// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Networking.h"
#include "HAL/Runnable.h"
#include "UDPMotionInput.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SIM_VISUALIZER_API UUDPMotionInput : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUDPMotionInput();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
private: 
  	FSocket* ListenerSocket;
    FUdpSocketReceiver* UDPReceiver;
	FIPv4Address ServerAddress;


	UPROPERTY(EditAnywhere, Category = "Network")
    FString ServerIP = TEXT("127.0.0.1");
    
    UPROPERTY(EditAnywhere, Category = "Network")
    int32 Port = 3001;

	TArray<float> ReceivedFloats;
    
    // İsteğe bağlı: Kaç float değeri beklediğimizi belirtelim
    UPROPERTY(EditAnywhere, Category = "Network")
    int32 ExpectedFloatCount = 6; // Örnek: pozisyon ve rotasyon

	void StartUDPReceiver();
    void ReceivedData(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint);

};
