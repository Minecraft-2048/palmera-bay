#include "PalmeraPhotographe.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PalmeraConfig.h"
#include "PalmeraMonde.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"

APalmeraPhotographe::APalmeraPhotographe()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

bool APalmeraPhotographe::EstDemande()
{
	FString Valeur;
	return FParse::Value(FCommandLine::Get(), TEXT("-photo="), Valeur);
}

void APalmeraPhotographe::PlacerCamera(const FString& Vue)
{
	TArray<FString> Morceaux;
	Vue.ParseIntoArray(Morceaux, TEXT(","));
	if (Morceaux.Num() < 5)
	{
		return;
	}
	const FVector Position = Palmera::EnMetres(FCString::Atod(*Morceaux[0]), FCString::Atod(*Morceaux[1]), FCString::Atod(*Morceaux[2]));
	const FRotator Rotation(FCString::Atod(*Morceaux[4]), FCString::Atod(*Morceaux[3]), 0.0);
	if (!Camera)
	{
		Camera = GetWorld()->SpawnActor<ACameraActor>(Position, Rotation);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	Camera->SetActorLocationAndRotation(Position, Rotation);
	Camera->GetCameraComponent()->SetFieldOfView(Morceaux.Num() >= 6 ? FCString::Atof(*Morceaux[5]) : 70.0f);
}

void APalmeraPhotographe::Preparer(int32 Index)
{
	const FPrise& P = Prises[Index];
	if (!P.Vue.IsEmpty() && P.Vue != TEXT("joueur"))
	{
		PlacerCamera(P.Vue);
	}
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		if (P.Heure >= 0.0)
		{
			It->Heure = P.Heure;
		}
		if (P.Pluie >= 0.0)
		{
			It->Pluie = It->PluieCible = P.Pluie;
			It->bMeteoAuto = false;
		}
	}
	Calme = 0.0;
	Total = 0.0;
	TempsImages = 0.0;
	NbImages = 0;
	Etape = 0;
}

void APalmeraPhotographe::BeginPlay()
{
	Super::BeginPlay();

	FString Chemin, TexteCam, Serie;
	FParse::Value(FCommandLine::Get(), TEXT("-photo="), Chemin);
	FParse::Value(FCommandLine::Get(), TEXT("-attente="), Attente);
	FParse::Value(FCommandLine::Get(), TEXT("-cam="), TexteCam, false);

	if (FParse::Value(FCommandLine::Get(), TEXT("-serie="), Serie))
	{
		// La liste donne les vues ; les photos sont rangées à côté de celle de -photo
		TArray<FString> Lignes;
		FFileHelper::LoadFileToStringArray(Lignes, *Serie);
		for (const FString& Ligne : Lignes)
		{
			TArray<FString> M;
			Ligne.ParseIntoArray(M, TEXT(";"));
			if (M.Num() >= 3)
			{
				FPrise P;
				P.Chemin = FPaths::Combine(FPaths::GetPath(Chemin), M[0] + TEXT(".png"));
				P.Vue = M[1];
				P.Heure = FCString::Atod(*M[2]);
				P.Pluie = M.Num() >= 4 ? FCString::Atod(*M[3]) : 0.0;
				Prises.Add(P);
			}
		}
	}
	if (Prises.Num() == 0)
	{
		FPrise P;
		P.Chemin = Chemin;
		P.Vue = TexteCam;
		Prises.Add(P);
	}
	Preparer(0);
}

void APalmeraPhotographe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Camera)
	{
		if (APlayerController* Joueur = GetWorld()->GetFirstPlayerController())
		{
			if (Joueur->GetViewTarget() != Camera)
			{
				Joueur->SetViewTarget(Camera);
			}
		}
	}

	Total += DeltaSeconds;
	const int32 Restants = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
	Calme = Restants > 0 ? 0.0 : Calme + DeltaSeconds;
	// Mesure de fluidité sur les dernières secondes, une fois l'image stabilisée
	if (Calme > Attente * 0.5)
	{
		TempsImages += DeltaSeconds;
		NbImages++;
	}

	if (Etape == 0 && (Calme >= Attente || Total > 1500.0))
	{
		const double Moyenne = TempsImages / FMath::Max(NbImages, 1);
		UE_LOG(LogTemp, Display, TEXT("PALMERA photo : %s (restants %d, attente totale %.0f s, %.1f images/s, %.1f ms)"), *Prises[Courante].Chemin, Restants, Total, 1.0 / FMath::Max(Moyenne, 0.0001), Moyenne * 1000.0);
		FFileHelper::SaveStringToFile(FString::Printf(TEXT("%s : %.1f images/s, %.1f ms\n"), *FPaths::GetBaseFilename(Prises[Courante].Chemin), 1.0 / FMath::Max(Moyenne, 0.0001), Moyenne * 1000.0),
			*(FPaths::GetPath(Prises[Courante].Chemin) / TEXT("fluidite.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
		FScreenshotRequest::RequestScreenshot(Prises[Courante].Chemin, FParse::Param(FCommandLine::Get(), TEXT("avecinterface")), false);
		Etape = 1;
		Total = 0.0;
	}
	else if (Etape == 1 && Total > 2.0)
	{
		Courante++;
		if (Courante < Prises.Num())
		{
			Preparer(Courante);
		}
		else
		{
			FPlatformMisc::RequestExit(false, TEXT("Photo terminee"));
		}
	}
}
