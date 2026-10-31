#include "PalmeraSons.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PalmeraConfig.h"
#include "PalmeraJoueur.h"
#include "PalmeraMonde.h"
#include "PalmeraPartie.h"
#include "PalmeraPolice.h"
#include "PalmeraVoiture.h"
#include "Sound/SoundBase.h"

using namespace Palmera;

namespace
{
	USoundBase* Charger(const TCHAR* Nom)
	{
		return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Sons/%s.%s"), Nom, Nom));
	}

	const TCHAR* Stations[] = { TEXT("Radio éteinte"), TEXT("Radio Coucher de Soleil"), TEXT("Palma FM"), TEXT("Vague Chill") };
	const TCHAR* Morceaux[] = { nullptr, TEXT("S_Radio1"), TEXT("S_Radio2"), TEXT("S_Radio3") };
}

APalmeraSons::APalmeraSons()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));
}

APalmeraSons* APalmeraSons::Lire(const UObject* Contexte)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (Monde)
	{
		for (TActorIterator<APalmeraSons> It(Monde); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

UAudioComponent* APalmeraSons::Boucle(const TCHAR* Nom)
{
	UAudioComponent* A = UGameplayStatics::SpawnSound2D(this, Charger(Nom), 0.0f, 1.0f, 0.0f, nullptr, true, false);
	if (A)
	{
		A->bAutoDestroy = false;
		A->bIsUISound = false;
	}
	return A;
}

void APalmeraSons::BeginPlay()
{
	Super::BeginPlay();

	if (FParse::Param(FCommandLine::Get(), TEXT("silence")) && GEngine)
	{
		if (FAudioDeviceHandle Audio = GEngine->GetMainAudioDevice())
		{
			Audio->SetTransientPrimaryVolume(0.0f);
		}
	}

	MoteurBas = Boucle(TEXT("S_MoteurBas"));
	MoteurMoyen = Boucle(TEXT("S_MoteurMoyen"));
	MoteurHaut = Boucle(TEXT("S_MoteurHaut"));
	Vent = Boucle(TEXT("S_Vent"));
	Crissement = Boucle(TEXT("S_Crissement"));
	Sirene = Boucle(TEXT("S_Sirene"));
	Vagues = Boucle(TEXT("S_Vagues"));
	Ville = Boucle(TEXT("S_Ville"));
	Pluie = Boucle(TEXT("S_Pluie"));
}

void APalmeraSons::StationSuivante()
{
	Station = (Station + 1) % 4;
	if (Radio)
	{
		Radio->Stop();
		Radio = nullptr;
	}
	if (Morceaux[Station])
	{
		Radio = UGameplayStatics::SpawnSound2D(this, Charger(Morceaux[Station]), 0.42f, 1.0f, 0.0f, nullptr, true, false);
		if (Radio)
		{
			Radio->bAutoDestroy = false;
		}
	}
	if (APalmeraPartie* Partie = APalmeraPartie::Lire(this))
	{
		Partie->Annoncer(Stations[Station], 2.0);
	}
}

FString APalmeraSons::NomStation() const
{
	return Stations[Station];
}

void APalmeraSons::Klaxon()
{
	Jouer(TEXT("S_Klaxon"), 0.5f);
}

void APalmeraSons::Jouer(const TCHAR* Nom, float Volume)
{
	UGameplayStatics::PlaySound2D(this, Charger(Nom), Volume);
}

void APalmeraSons::DireVoix(int32 Numero, float Hauteur)
{
	if (VoixEnCours)
	{
		VoixEnCours->Stop();
		VoixEnCours = nullptr;
	}
	const FString Nom = FString::Printf(TEXT("V_%04d"), Numero);
	if (USoundBase* Son = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Voix/%s.%s"), *Nom, *Nom)))
	{
		VoixEnCours = UGameplayStatics::SpawnSound2D(this, Son, 1.0f, Hauteur, 0.0f, nullptr, true, false);
		if (VoixEnCours)
		{
			VoixEnCours->bAutoDestroy = false;
		}
	}
}

void APalmeraSons::Tick(float DeltaSeconds){
	Super::Tick(DeltaSeconds);

	const APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	if (!J || !J->GetPawn() || !MoteurBas)
	{
		return;
	}
	const APalmeraVoiture* V = J->VoitureActuelle();
	const FVector Pos = J->GetPawn()->GetActorLocation() / Metre;

	// Trois boucles de moteur fondues l'une dans l'autre selon le régime
	double Bas = 0.0, Moyen = 0.0, Haut = 0.0, HauteurSon = 1.0, Souffle = 0.0, Glisse = 0.0;
	if (V)
	{
		const double R = FMath::Clamp((V->Regime() - 900.0) / 6500.0, 0.0, 1.0);
		const double Charge = 0.55 + 0.45 * V->GazActuel();
		Bas = FMath::Clamp(1.0 - R * 2.4, 0.0, 1.0) * 0.5 * Charge;
		Moyen = FMath::Clamp(1.0 - FMath::Abs(R - 0.45) * 2.6, 0.0, 1.0) * 0.55 * Charge;
		Haut = FMath::Clamp((R - 0.45) * 2.4, 0.0, 1.0) * 0.6 * Charge;
		HauteurSon = FMath::Lerp(0.75, 1.65, R);
		const double Vitesse = V->GetVelocity().Size() / Metre;
		Souffle = FMath::Clamp(Vitesse / 75.0, 0.0, 0.8);
		Glisse = FMath::Clamp((V->Derapage() - 3.0) / 8.0, 0.0, 0.7);
		if (V->ChocRecent() > 5.0 && GetWorld()->GetTimeSeconds() - DernierChoc > 0.4)
		{
			DernierChoc = GetWorld()->GetTimeSeconds();
			Jouer(TEXT("S_Choc"), float(FMath::Clamp(V->ChocRecent() / 20.0, 0.25, 1.0)));
		}
	}
	MoteurBas->SetVolumeMultiplier(float(Bas));
	MoteurMoyen->SetVolumeMultiplier(float(Moyen));
	MoteurHaut->SetVolumeMultiplier(float(Haut));
	MoteurBas->SetPitchMultiplier(float(HauteurSon));
	MoteurMoyen->SetPitchMultiplier(float(HauteurSon * 0.86));
	MoteurHaut->SetPitchMultiplier(float(HauteurSon * 0.72));
	Vent->SetVolumeMultiplier(float(Souffle));
	Vent->SetPitchMultiplier(float(0.8 + Souffle * 0.6));
	Crissement->SetVolumeMultiplier(float(Glisse));

	// Les vagues se rapprochent de la plage, la ville reprend le dessus dans les rues
	const double VersPlage = FMath::Clamp(1.0 - (Pos.X - BordOuest + 60.0) / 340.0, 0.0, 1.0);
	Vagues->SetVolumeMultiplier(float(0.03 + VersPlage * 0.6));
	Ville->SetVolumeMultiplier(float(0.22 * (1.0 - VersPlage * 0.8)));
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		Pluie->SetVolumeMultiplier(float(It->Pluie * 0.55));
	}

	double Sirenes = 0.0;
	for (TActorIterator<APalmeraPolice> It(GetWorld()); It; ++It)
	{
		if (It->DistancePlusProche < 1e8)
		{
			Sirenes = FMath::Clamp(1.0 - It->DistancePlusProche / 340.0, 0.08, 0.75);
		}
	}
	Sirene->SetVolumeMultiplier(float(Sirenes));

	// La radio ne s'entend qu'au volant, et baisse quand quelqu'un parle
	if (Radio)
	{
		const bool bParole = VoixEnCours && VoixEnCours->IsPlaying();
		VolumeRadio = FMath::FInterpTo(VolumeRadio, V ? (bParole ? 0.1 : 0.42) : 0.0, double(DeltaSeconds), 4.0);
		Radio->SetVolumeMultiplier(float(FMath::Max(VolumeRadio, 0.0001)));
	}
}
