// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraPublisher.h"

#if WITH_EDITOR
	#include "Editor.h"
	#include "Editor/EditorEngine.h"
	#include "IAssetViewport.h"
#endif




void CamRenderTarget::Initialize(UTextureRenderTarget2D* _CamRenderTargetPtr, FString _shmNameFString){
	if (!Initialized){

		if (!CamRenderTargetPtr){
			//bunu daha iyi bir yolla yapıp kontrol edebilirsin belki
			shmName = TCHAR_TO_ANSI(*_shmNameFString);
			CamRenderTargetPtr = _CamRenderTargetPtr;

		}

		//wee need to have size for opening the shared memory
		if (CamRenderTargetPtr && (!camSizeInitialized)){
			CamWidth = CamRenderTargetPtr->GetSurfaceWidth(); 
			CamHeight = CamRenderTargetPtr->GetSurfaceHeight();

			if (!((CamWidth>0) and (CamHeight>0))){return;}
			else {
				ShmSize = CamWidth * CamHeight * 4;
				camSizeInitialized = true;
				}
		}

		if (!shmMemoInitialized){
			if (!InitShmMemo()){return;}
			else {shmMemoInitialized = true;}
		}

		Initialized = true;
	}

}
bool CamRenderTarget::InitShmMemo(){

	FString DebugMessage;

	//shared memory oluştur
	shmFd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
	if (shmFd == -1) {
		DebugMessage = TEXT("Could not opened shared memory. ") +  FString(strerror(errno));        
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
		return false;
	}

	//shared memo boyutunu ayarla
	if (ftruncate(shmFd, ShmSize) == -1) {
		DebugMessage = TEXT("Failed to set size of shared memory. ") + FString(strerror(errno));        
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
		return false;
	}

	// memoyu bu processin adres uzayına ekle
	ShmMemo = mmap(0, ShmSize, PROT_WRITE, MAP_SHARED, shmFd, 0);
	if (ShmMemo == MAP_FAILED) {
		DebugMessage = TEXT("Failed to map shared memory, error: ") + FString::FromInt(ShmSize) + FString(strerror(errno));    
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
		return false;
	}

	return true;

}

void CamRenderTarget::DeInitShmMemo(){
	if (shmMemoInitialized){
		munmap(ShmMemo, ShmSize);
		close(shmFd);
		shm_unlink(shmName);
		shmMemoInitialized = false;
	}
}

//eğer objeyi destruct etmeden 0'lamak istersek
void CamRenderTarget::deInitialize(){
	DeInitShmMemo();
	Initialized = false;
}

void CamRenderTarget::WriteToShmMemo(){
	
	if (CamRenderTargetPtr){
		auto RenderTargetResource = CamRenderTargetPtr->GameThread_GetRenderTargetResource();
	

		TArray<FColor> ColorBuffer;
		if (RenderTargetResource)
		{
			RenderTargetResource->ReadPixels(ColorBuffer);
			uint8 frameBuffer[ShmSize];

			//performans için kesinlikle kapanması lazım burasının
			// DebugMessage = TEXT("captured frame number: ") + FString::FromInt(LastFrame.ColorBuffer.Num());
			// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, *DebugMessage);

			if (ShmSize == ColorBuffer.Num()*4){
				for (uint32 i = 0; i < ColorBuffer.Num(); i++)
				{

					frameBuffer[(i*4) + 0] = ColorBuffer[i].B;
					frameBuffer[(i*4) + 1] = ColorBuffer[i].G;
					frameBuffer[(i*4) + 2] = ColorBuffer[i].R;
					frameBuffer[(i*4) + 3] = ColorBuffer[i].A;
				}

				// //shared memoya bas TODO: timestamp de basılmalı latency için
				memcpy(ShmMemo, frameBuffer, sizeof(frameBuffer));

		}
		}

	}
}



// Sets default values for this component's properties
UCameraPublisher::UCameraPublisher()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCameraPublisher::BeginPlay()
{
	Super::BeginPlay();
	InitShmMemo();
	StartFrameGrab();
	leftCamRenderTarget.Initialize(LeftCamRenderTarget2D, FString("/leftCam"));
	//DiscoverCameraComponents();
	
}

// Called every frame
void UCameraPublisher::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//if'e bakıp fonksiyonlara baksak daha optimize olabilir, function call overheadinden kurtuluyoruz. (ya da kendisini sonlandıran bir
	// time callbacki en hızlı olan olacaktır)

	StartFrameGrab();
	InitShmMemo();

	//bu tick işini çoklu threade alıp hızlandırmayı deneyebilirsin
	leftCamRenderTarget.Initialize(LeftCamRenderTarget2D, FString("/leftCam"));
	leftCamRenderTarget.WriteToShmMemo();


	FString DebugMessage;

	// DebugMessage = TEXT("Tick:  ") + FString::FromInt(FrameGrabber.IsValid());
	// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, *DebugMessage);

	if (FrameGrabber.IsValid() and shmMemoReady){
		FrameGrabber->CaptureThisFrame(FFramePayloadPtr());
		TArray<FCapturedFrameData> Frames = FrameGrabber->GetCapturedFrames();

		// DebugMessage = TEXT("captured frame: ") + FString::FromInt(Frames.Last().ColorBuffer.Num());
		// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, *DebugMessage);

		//shared memoya yazacak şekilde arraye dönüştür
		if (Frames.Num())
		{
			FCapturedFrameData& LastFrame = Frames.Last();

			uint8 frameBuffer[LastFrame.ColorBuffer.Num()*4];

			//performans için kesinlikle kapanması lazım burasının
			// DebugMessage = TEXT("captured frame number: ") + FString::FromInt(LastFrame.ColorBuffer.Num());
			// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, *DebugMessage);

			for (int32 i = 0; i < LastFrame.ColorBuffer.Num(); i++)
			{

				frameBuffer[(i*4) + 0] = LastFrame.ColorBuffer[i].B;
				frameBuffer[(i*4) + 1] = LastFrame.ColorBuffer[i].G;
				frameBuffer[(i*4) + 2] = LastFrame.ColorBuffer[i].R;
				frameBuffer[(i*4) + 3] = LastFrame.ColorBuffer[i].A;
			}

			// //shared memoya bas TODO: timestamp de basılmalı latency için
			memcpy(ShmMemo, frameBuffer, sizeof(frameBuffer));

			//TODO: okuyacak processe sinyal yollayabilirsin callback tarzı senkronlamak için

		}

	}
}




//make this parametric for width, height and name of the memory
//you may return boolean for error handling
void UCameraPublisher::InitShmMemo(){

	if (!shmMemoReady and (Width > 0) and (Height > 0)){
		FString DebugMessage;

		shmName = "/ue5_right_cam";
		ShmSize = Width*Height*4;  //her piksel başına 4 kanal düşüyor

		//shared memory oluştur
		shmFd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
		if (shmFd == -1) {
			DebugMessage = TEXT("Could not opened shared memory. ") +  FString(strerror(errno));        
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
			return;
		}

		//shared memo boyutunu ayarla
		if (ftruncate(shmFd, ShmSize) == -1) {
			DebugMessage = TEXT("Failed to set size of shared memory. ") + FString(strerror(errno));        
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
			return;
		}

    	// memoyu bu processin adres uzayına ekle
		ShmMemo = mmap(0, ShmSize, PROT_WRITE, MAP_SHARED, shmFd, 0);
		if (ShmMemo == MAP_FAILED) {
			DebugMessage = TEXT("Failed to map shared memory, error: ") + FString::FromInt(ShmSize) + FString(strerror(errno));    
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
			return;
		}

		shmMemoReady = true;

	}
	

}

void UCameraPublisher::StartFrameGrab() {

	//tipini bulup headera ekle
	// GameWindow = GEngine->GameViewport->GetWindow().Get();
	// FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddUObject(this, &UCameraPublisher::OnBackBufferReady_RenderThread);
	if (!fGrabReady){
		FString DebugMessage;
		TSharedPtr<FSceneViewport> SceneViewport;

		DebugMessage = TEXT("Frame grab is not initialized, trying again...");   
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);

	
			
		#if WITH_EDITOR
		if (GIsEditor)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE)
				{
					FSlatePlayInEditorInfo* SlatePlayInEditorSession = GEditor->SlatePlayInEditorMap.Find(Context.ContextHandle);
					if (SlatePlayInEditorSession)
					{
						if (SlatePlayInEditorSession->DestinationSlateViewport.IsValid())
						{
							TSharedPtr<IAssetViewport> DestinationLevelViewport = SlatePlayInEditorSession->DestinationSlateViewport.Pin();
							SceneViewport = DestinationLevelViewport->GetSharedActiveViewport();
						}
						else if (SlatePlayInEditorSession->SlatePlayInEditorWindowViewport.IsValid())
						{
							SceneViewport = SlatePlayInEditorSession->SlatePlayInEditorWindowViewport;
						}
					}
				}
			}
		}
		#else
		//game viewportu çek.	
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);

		if (GameEngine)
			{
			DebugMessage = TEXT("Game Engine received");   
			GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, *DebugMessage);

			// Get the viewport
			SceneViewport = GameEngine->SceneViewport;
			}

		#endif
			// Check if the Viewport is valid and cast it to FSceneViewport
			if (SceneViewport)
				{
					//ilk olarak ekranımızın boyutlarını öğrenelim
					const FVector2D ViewportSize = FVector2D(SceneViewport->GetSizeXY());

					Width = ViewportSize.X;
					Height = ViewportSize.Y;

					DebugMessage = TEXT("Width: ") + FString::FromInt(Width)  + TEXT("Height: ") + FString::FromInt(Height);    
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, *DebugMessage) ;

					if ((!(Width > 0)) or (!(Height > 0))){
						return;
					}

					// Capture Start
					FrameGrabber = MakeShareable(new FFrameGrabber(SceneViewport.ToSharedRef(), SceneViewport->GetSize()));

					if (!FrameGrabber.IsValid()) {return;}

					FrameGrabber->StartCapturingFrames();
					DebugMessage = TEXT("Capturing frame has started: ");    
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, *DebugMessage);
					fGrabReady = true;

				}
				
	}

}


void UCameraPublisher::DeInitShmMemo(){
	if (shmMemoReady){
		munmap(ShmMemo, ShmSize);
		close(shmFd);
		//Hafızayı temizlemek için
		shm_unlink(shmName);
		shmMemoReady = false;
	}

}

void UCameraPublisher::ReleaseFrameGrabber()
{
	if (FrameGrabber.IsValid())
	{
		FrameGrabber->StopCapturingFrames();
		FrameGrabber->Shutdown();
		FrameGrabber.Reset();
	}
}


void UCameraPublisher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseFrameGrabber();
	DeInitShmMemo();
	leftCamRenderTarget.deInitialize();
    Super::EndPlay(EndPlayReason);
}




void UCameraPublisher::DiscoverCameraComponents(){
	TArray<UCameraComponent*> CameraComponents;
    GetOwner()->GetComponents<UCameraComponent>(CameraComponents);
	
    for (UCameraComponent* Camera : CameraComponents)
    {
        if (Camera)
        {
            // Get the name of the camera component
            FString CameraName = Camera->GetName();
			
			FString DebugMessage =TEXT("Discovered Camera Name: ") + CameraName;

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *DebugMessage);
			//SetupFrameGrabberForCamera(Camera);

			//burada bir const int ile her bir kamera adına bir index ata, vektörü önceden allocate edip o indexe koy kamerayı
			// böylece istediğin kameraya ulaşabileceksin istediğin zaman

            // // Example classification by name
            // if (CameraName.Contains("FrontCamera"))
            // {
            //     // This is the front camera
            //     SetupFrameGrabberForCamera(Camera, "FrontCamera");
            // }
            // else if (CameraName.Contains("RearCamera"))
            // {
            //     // This is the rear camera
            //     SetupFrameGrabberForCamera(Camera, "RearCamera");
            // }
            // else if (CameraName.Contains("LeftCamera"))
            // {
            //     // This is the left camera
            //     SetupFrameGrabberForCamera(Camera, "LeftCamera");
            // }
            // // Add more classifications as needed
        }
    }
}



// // Helper function to set up a frame grabber for a specific camera
// void UCameraPublisher::SetupFrameGrabberForCamera(UCameraComponent* Camera)
// {
//     // Create a Render Target for this Camera
//     UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
//     RenderTarget->InitAutoFormat(Width, Height);
//     RenderTarget->ClearColor = FLinearColor::Black;
//     RenderTarget->UpdateResource();

//     // Create SceneCaptureComponent2D and attach it to the Camera
//     USceneCaptureComponent2D* SceneCapture = NewObject<USceneCaptureComponent2D>(GetOwner());
//     SceneCapture->SetupAttachment(Camera);
//     SceneCapture->SetRelativeLocation(FVector::ZeroVector);
//     SceneCapture->FOVAngle = Camera->FieldOfView;
//     SceneCapture->TextureTarget = RenderTarget;
//     SceneCapture->bCaptureEveryFrame = true;
//     SceneCapture->bCaptureOnMovement = false;
//     SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
//     SceneCapture->RegisterComponent();

//     // Create a FrameGrabber for this SceneCapture's Render Target
//     TSharedPtr<FFrameGrabber> FrameGrabber = MakeShareable(new FFrameGrabber(SceneCapture->TextureTarget->GetRenderTargetResource(), FIntPoint(Width, Height)));
//     FrameGrabbers.Add(FrameGrabber);
//     FrameGrabber->StartCapturingFrames();
// }

// void UCameraPublisher::OnBackBufferReady_RenderThread(SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer)
// 	{
// 		// Check if the Slate window matches the game window
// 		if (GameWindow == &SlateWindow)
// 		{
	
// 				// Get the viewport client and the texture to capture
// 				const UCaptureGameViewportClient* ViewportClient = static_cast<UCaptureGameViewportClient*>(GetWorld()->GetGameViewport());

// 				if (ViewportClient){
// 					auto renderTarget = ViewportClient->MyRenderTarget->GetResource()->GetTexture2DRHI();
// 					if (renderTarget){
// 						auto Texture = renderTarget->GetResource()->GetTexture2DRHI();

// 						// Determine the texture to use based on the capture options
// 						GameTexture = Texture;
			
// 						// Get the video data from the screen
// 						// Get the immediate RHICmdList
// 						FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

// 						SCOPED_DRAW_EVENT(RHICmdList, CaptureEvent);


// 						uint32 Stride = 4*Width;
// 						// Lock the game texture and get the texture data
// 						uint8* TextureData = static_cast<uint8*>(RHICmdList.LockTexture2D(GameTexture->GetTexture2D(), 0, EResourceLockMode::RLM_ReadOnly, Stride, false));

// 						//memcpy(ShmMemo, TextureData, 4*Width*Height);

// 						// Unlock the game texture
// 						RHICmdList.UnlockTexture2D(GameTexture, 0, false);		

// 					}

			
// 				}

					
// 		}
// 	}


