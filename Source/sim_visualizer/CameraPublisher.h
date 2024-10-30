// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//other libs
#include <sys/mman.h>    // For mmap
#include <fcntl.h>       // For O_CREAT, O_RDWR
#include <unistd.h>      // For ftruncate, close
#include <cstring>       // For memcpy
#include <sys/stat.h>    // For mode constants

//unreal
#include "CoreMinimal.h"
#include "FrameGrabber.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameEngine.h"

// #include "Engine/TextureRenderTarget2D.h"
// #include "CaptureGameViewportClient.h"





#include "CameraPublisher.generated.h"


class SIM_VISUALIZER_API CamRenderTarget{
	private:

		void* ShmMemo; //shared memorye yazmak için kullanılacak pointer
		//temizlemek için bu değişkenlerin hafızada tutulması gerekiyor
		int CamWidth;
		int CamHeight;
		int ShmSize;
		FString shmNameFString;
		const char* shmName; //fstring olabilir bu
		int shmFd;
		UTextureRenderTarget2D* CamRenderTargetPtr;

		bool shmMemoInitialized = false;
		bool camSizeInitialized = false;
		bool Initialized = false;

	public:

		//buraya kendisini kapatan bir timer taskı init etmelisin, eğer class
		// init olmadıysa sürekli init fonksiyonunu çağırsın
		void Initialize(UTextureRenderTarget2D* _CamRenderTargetPtr, FString _shmNameFString);
		void deInitialize();
		bool InitShmMemo();
		void DeInitShmMemo();
		void WriteToShmMemo();


		// aşağıdaki gibi renderTargetResource tutmak isteyebilirim direkt.
		// auto RenderTargetResource = renderTargetColor->GameThread_GetRenderTargetResource();

		// if (RenderTargetResource)
		// {
		// TArray<FColor> buffer8;
		// RenderTargetResource->ReadPixels(buffer8);

};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SIM_VISUALIZER_API UCameraPublisher : public UActorComponent
{
	GENERATED_BODY()

public:	

	// Sets default values for this component's properties
	UCameraPublisher();
	// void OnBackBufferReady_RenderThread(SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer);

	void SetupFrameGrabberForCamera(UCameraComponent* Camera);
	void DiscoverCameraComponents();

	void InitShmMemo();
	void DeInitShmMemo();
	void StartFrameGrab();
	void ReleaseFrameGrabber();

	//keşfedilen kamera komponentlerini tutacak array
	//TArray<FFrameGrabber*> FrameGrabbers;
	TSharedPtr<FFrameGrabber> FrameGrabber; //ismini front camera target cam diye değiştir


	//daha sonra bu objelerden bir array oluştur ve start ile finish için bu arrayi dönerek işlemlerini yap
	// bu for döngüsü içerisinde yine parametrik bir şekilde init shame memo gibi şeyler kullan
	//bunları bir struct içerisine al (UPROPERTY ile struct içerisine veri koymanın yolunu bulabilirsin), belki
	//init fonksiyonunda
	UPROPERTY(EditAnywhere)
	UTextureRenderTarget2D* LeftCamRenderTarget2D;
	CamRenderTarget leftCamRenderTarget;




protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;



public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	void* ShmMemo; //shared memorye yazmak için kullanılacak pointer
	//temizlemek için bu değişkenlerin hafızada tutulması gerekiyor
	int ShmSize;
	int Width;
	int Height;
	const char* shmName;
	int shmFd;
	bool gameEngineInitialized = false;

	bool shmMemoReady = false;
	bool fGrabReady = false;

	// SWindow* GameWindow;
	// FTexture2DRHIRef GameTexture;

	
	


		
};
