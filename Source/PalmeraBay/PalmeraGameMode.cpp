#include "PalmeraGameMode.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "PalmeraCirculation.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "PalmeraConfig.h"
#include "PalmeraCourse.h"
#include "Widgets/SWindow.h"
#include "PalmeraEffets.h"
#include "PalmeraHeros.h"
#include "PalmeraHistoire.h"
#include "PalmeraJoueur.h"
#include "PalmeraLieux.h"
#include "PalmeraMonde.h"
#include "PalmeraPartie.h"
#include "PalmeraPNJ.h"
#include "PalmeraPhotographe.h"
#include "PalmeraPietons.h"
#include "PalmeraPolice.h"
#include "PalmeraSons.h"
#include "PalmeraVille.h"
#include "PalmeraVoiture.h"

APalmeraGameMode::APalmeraGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APalmeraJoueur::StaticClass();
}

void APalmeraGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* Monde = GetWorld();
	Monde->SpawnActor<APalmeraMonde>();
	Monde->SpawnActor<APalmeraVille>();

	// Palmera Racing : la même ville, sans histoire ni circulation
	if (FParse::Param(FCommandLine::Get(), TEXT("course")))
	{
		if (GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
		{
			GEngine->GameViewport->GetWindow()->SetTitle(FText::FromString(TEXT("Palmera Racing")));
		}
		Monde->SpawnActor<APalmeraSons>();
		Monde->SpawnActor<APalmeraEffets>();
		Monde->SpawnActor<APalmeraCourse>();
		if (APalmeraPhotographe::EstDemande())
		{
			Monde->SpawnActor<APalmeraPhotographe>();
		}
		return;
	}

	// Le joueur commence sur la promenade, une voiture l'attend en bordure de rue
	const FVector Depart = Palmera::EnMetres(Palmera::BordOuest - 13.5, 41.0, 1.4);
	APalmeraHeros* Heros = Monde->SpawnActor<APalmeraHeros>(Depart, FRotator(0.0, 0.0, 0.0));

	const FTransform Place(FRotator(0.0, 90.0, 0.0), Palmera::EnMetres(Palmera::BordOuest - 9.0, 44.0, 1.2));
	APalmeraVoiture* Voiture = Monde->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Place);
	FFicheVoiture Fiche = APalmeraVoiture::FicheSport();
	Fiche.Peinture = FLinearColor(0.72f, 0.03f, 0.05f);
	Voiture->Configurer(Fiche);
	Voiture->FinishSpawning(Place);

	if (APalmeraJoueur* Joueur = Cast<APalmeraJoueur>(Monde->GetFirstPlayerController()))
	{
		Joueur->Demarrer(Heros);
	}
	Monde->SpawnActor<APalmeraCirculation>();
	Monde->SpawnActor<APalmeraPartie>();
	Monde->SpawnActor<APalmeraPolice>();
	Monde->SpawnActor<APalmeraSons>();
	Monde->SpawnActor<APalmeraPietons>();
	Monde->SpawnActor<APalmeraHistoire>();
	Monde->SpawnActor<APalmeraEffets>();
	Monde->SpawnActor<APalmeraLieux>();

	// Photo de contrôle : tout le casting aligné sur le trottoir
	if (FParse::Param(FCommandLine::Get(), TEXT("casting")))
	{
		static const TCHAR* Noms[] = { TEXT("Sam"), TEXT("Nina"), TEXT("Lou"), TEXT("Ines"), TEXT("Aya"), TEXT("Ferrand"), TEXT("Valmont"), TEXT("Carla"), TEXT("Bruno"), TEXT("Theo"),
			TEXT("Garde"), TEXT("Passant1"), TEXT("Passant2"), TEXT("Passant3"), TEXT("Passant4"), TEXT("Passante1"), TEXT("Passante2"), TEXT("Passante3"), TEXT("Passante4") };
		for (int32 I = 0; I < UE_ARRAY_COUNT(Noms); I++)
		{
			APalmeraPNJ* Modele = Monde->SpawnActor<APalmeraPNJ>(Palmera::EnMetres(-560.0 + I * 1.25, 109.0, 1.4), FRotator(0.0, 90.0, 0.0));
			FFichePNJ Essai;
			Essai.Modele = Noms[I];
			Modele->Configurer(Essai);
		}
	}

	// Photo de contrôle : tous les modèles côte à côte
	if (FParse::Param(FCommandLine::Get(), TEXT("exposition")))
	{
		const FLinearColor Teintes[] = { FLinearColor(0.72f, 0.72f, 0.74f), FLinearColor(0.42f, 0.03f, 0.03f), FLinearColor(0.08f, 0.1f, 0.28f), FLinearColor(0.85f, 0.85f, 0.83f), FLinearColor(0.2f, 0.22f, 0.24f), FLinearColor(0.85f, 0.56f, 0.02f) };
		for (int32 I = 0; I < 6; I++)
		{
			const FTransform Pose(FRotator(0.0, 90.0, 0.0), Palmera::EnMetres(-953.5 + I * 0.0, 150.0 + I * 8.0, 0.9));
			APalmeraVoiture* V = Monde->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Pose);
			FFicheVoiture F = APalmeraVoiture::FicheCivile(I);
			F.Peinture = Teintes[I];
			V->Configurer(F);
			V->FinishSpawning(Pose);
		}
	}
	if (APalmeraPhotographe::EstDemande())
	{
		Monde->SpawnActor<APalmeraPhotographe>();
	}
}

void APalmeraGameMode::ReleverSquelette(const FString& Modele, const FString& Fichier)
{
	const USkeletalMesh* Maillage = LoadObject<USkeletalMesh>(nullptr, *Modele);
	if (!Maillage)
	{
		return;
	}
	const FReferenceSkeleton& Ref = Maillage->GetRefSkeleton();
	TArray<FTransform> Global;
	Global.SetNum(Ref.GetNum());
	FString Sortie;
	for (int32 I = 0; I < Ref.GetNum(); I++)
	{
		const int32 Parent = Ref.GetParentIndex(I);
		Global[I] = Parent == INDEX_NONE ? Ref.GetRefBonePose()[I] : Ref.GetRefBonePose()[I] * Global[Parent];
		const FVector T = Global[I].GetLocation();
		const FQuat Q = Global[I].GetRotation();
		Sortie += FString::Printf(TEXT("%s %s %.5f %.5f %.5f %.7f %.7f %.7f %.7f\n"), *Ref.GetBoneName(I).ToString(), Parent == INDEX_NONE ? TEXT("-") : *Ref.GetBoneName(Parent).ToString(), T.X, T.Y, T.Z, Q.X, Q.Y, Q.Z, Q.W);
	}
	FFileHelper::SaveStringToFile(Sortie, *Fichier);
}
