// Fill out your copyright notice in the Description page of Project Settings.


#include "UDPMotionInput.h"

// Sets default values for this component's properties
UUDPMotionInput::UUDPMotionInput()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUDPMotionInput::BeginPlay()
{
	Super::BeginPlay();
	StartUDPReceiver();


	// ...
	
}


// Called every frame
void UUDPMotionInput::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UUDPMotionInput::StartUDPReceiver()
{
    FIPv4Address::Parse(ServerIP, ServerAddress);
    FIPv4Endpoint Endpoint(ServerAddress, Port);

    ListenerSocket = FUdpSocketBuilder(TEXT("UDPSocket"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    if (ListenerSocket)
    {
        UDPReceiver = new FUdpSocketReceiver(ListenerSocket, FTimespan::FromMilliseconds(100), TEXT("UDPReceiver"));
        UDPReceiver->OnDataReceived().BindUObject(this, &UUDPMotionInput::ReceivedData);
        UDPReceiver->Start();
    }
}

void UUDPMotionInput::ReceivedData(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint)
{
    if (!DataPtr.IsValid()) return;

    // Float array boyutunu hesapla
    int32 FloatCount = DataPtr->TotalSize() / sizeof(float);
    
    if (FloatCount > 0)
    {
        // Float array'i hazırla
        ReceivedFloats.SetNum(FloatCount);
        
        // Doğrudan float değerlerini kopyala
        FMemory::Memcpy(ReceivedFloats.GetData(), DataPtr->GetData(), DataPtr->TotalSize());

        // Async task olarak game thread'de çalıştıralım
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            // Yeterli sayıda float değeri var mı kontrol et
            if (ReceivedFloats.Num() >= 6) // pozisyon ve rotasyon için minimum 6 değer
            {
                FVector NewLocation(
                    ReceivedFloats[0],
                    ReceivedFloats[1],
                    ReceivedFloats[2]
                );
                
                FRotator NewRotation(
                    ReceivedFloats[3],
                    ReceivedFloats[4],
                    ReceivedFloats[5]
                );

                SetRelativeLocation(NewLocation);
                SetRelativeRotation(NewRotation);
            }
        });
    }
}
void UUDPMotionInput::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    if (UDPReceiver)
    {
        UDPReceiver->Stop();
        delete UDPReceiver;
        UDPReceiver = nullptr;
    }

    if (ListenerSocket)
    {
        ListenerSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
    }
}