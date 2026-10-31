#include "PalmeraLieux.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PalmeraConfig.h"
#include "PalmeraHeros.h"
#include "PalmeraHistoire.h"
#include "PalmeraJoueur.h"
#include "PalmeraPartie.h"
#include "PalmeraPNJ.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace Palmera;

namespace
{
	// Toutes les salles sont bâties au même endroit, très au-dessus de la ville : une seule existe à la fois
	const FVector AncreMetres(-420.0, -2.0, 400.0);

	double Nombre(const TArray<TSharedPtr<FJsonValue>>& V, int32 I)
	{
		return V.IsValidIndex(I) ? V[I]->AsNumber() : 0.0;
	}

	FVector2D Point(const TSharedPtr<FJsonObject>& O)
	{
		const TArray<TSharedPtr<FJsonValue>>& V = O->GetArrayField(TEXT("pos"));
		return FVector2D(Nombre(V, 0), Nombre(V, 1));
	}

	FVector Ancre()
	{
		return AncreMetres * Metre;
	}
}

FVector2D FSalle::CentreLot() const
{
	const double Case = 98.0 / 3.0;
	return FVector2D((BlocI + 0.5) * Pas + (LotA - 1) * Case, (BlocJ + 0.5) * Pas + (LotB - 1) * Case);
}

FVector FSalle::Facade() const
{
	return FRotator(0.0, Lacet, 0.0).RotateVector(FVector(0.0, -1.0, 0.0));
}

FVector2D FSalle::Porte() const
{
	const FVector F = Facade();
	return CentreLot() + FVector2D(F.X, F.Y) * (Recul + 1.5);
}

const TArray<FSalle>& APalmeraLieux::Salles()
{
	static TArray<FSalle> Liste;
	if (Liste.Num() > 0)
	{
		return Liste;
	}
	FString Texte;
	if (!FFileHelper::LoadFileToString(Texte, *(FPaths::ProjectContentDir() / TEXT("Histoire/lieux.json"))))
	{
		return Liste;
	}
	TSharedPtr<FJsonObject> Racine;
	const TSharedRef<TJsonReader<>> Lecteur = TJsonReaderFactory<>::Create(Texte);
	if (!FJsonSerializer::Deserialize(Lecteur, Racine) || !Racine.IsValid())
	{
		return Liste;
	}
	for (const TSharedPtr<FJsonValue>& Valeur : Racine->GetArrayField(TEXT("lieux")))
	{
		const TSharedPtr<FJsonObject> O = Valeur->AsObject();
		FSalle S;
		S.Id = O->GetStringField(TEXT("id"));
		S.Nom = O->GetStringField(TEXT("nom"));
		S.Enseigne = O->GetStringField(TEXT("enseigne"));
		const TArray<TSharedPtr<FJsonValue>>& Lot = O->GetArrayField(TEXT("lot"));
		S.BlocI = int32(Nombre(Lot, 0));
		S.BlocJ = int32(Nombre(Lot, 1));
		S.LotA = int32(Nombre(Lot, 2));
		S.LotB = int32(Nombre(Lot, 3));
		S.Lacet = O->GetNumberField(TEXT("lacet"));
		S.Recul = O->GetNumberField(TEXT("recul"));
		const TArray<TSharedPtr<FJsonValue>>& Couleur = O->GetArrayField(TEXT("couleur"));
		S.Couleur = FLinearColor(float(Nombre(Couleur, 0)), float(Nombre(Couleur, 1)), float(Nombre(Couleur, 2)));
		const TArray<TSharedPtr<FJsonValue>>& Taille = O->GetArrayField(TEXT("taille"));
		S.Taille = FVector(Nombre(Taille, 0), Nombre(Taille, 1), Nombre(Taille, 2));
		for (const TSharedPtr<FJsonValue>& B : O->GetArrayField(TEXT("boites")))
		{
			const TArray<TSharedPtr<FJsonValue>>& V = B->AsArray();
			const FVector Centre(Nombre(V, 0), Nombre(V, 1), Nombre(V, 2));
			const FVector Demi(Nombre(V, 3), Nombre(V, 4), Nombre(V, 5));
			S.Boites.Add(FBox(Centre - Demi, Centre + Demi));
		}
		for (const TSharedPtr<FJsonValue>& L : O->GetArrayField(TEXT("lumieres")))
		{
			const TArray<TSharedPtr<FJsonValue>>& V = L->AsArray();
			FSalle::FLampe Lampe;
			Lampe.Position = FVector(Nombre(V, 0), Nombre(V, 1), Nombre(V, 2));
			Lampe.Force = float(Nombre(V, 3));
			Lampe.Couleur = FLinearColor(float(Nombre(V, 4)), float(Nombre(V, 5)), float(Nombre(V, 6)));
			Lampe.Rayon = float(Nombre(V, 7));
			S.Lampes.Add(Lampe);
		}
		for (const TSharedPtr<FJsonValue>& P : O->GetArrayField(TEXT("pnj")))
		{
			const TSharedPtr<FJsonObject> Fiche = P->AsObject();
			FSalle::FPersonne Personne;
			Personne.Modele = Fiche->GetStringField(TEXT("modele"));
			Personne.Position = Point(Fiche);
			Personne.Cap = Fiche->GetNumberField(TEXT("cap"));
			Personne.Phrase = Fiche->GetStringField(TEXT("phrase"));
			S.Personnes.Add(Personne);
		}
		for (const TSharedPtr<FJsonValue>& A : O->GetArrayField(TEXT("actions")))
		{
			const TSharedPtr<FJsonObject> Fiche = A->AsObject();
			FSalle::FAction Action;
			Action.Position = Point(Fiche);
			Action.Rayon = Fiche->GetNumberField(TEXT("rayon"));
			Action.Invite = Fiche->GetStringField(TEXT("invite"));
			Action.Message = Fiche->GetStringField(TEXT("message"));
			Action.Prix = int32(Fiche->GetNumberField(TEXT("prix")));
			Action.Gain = int32(Fiche->GetNumberField(TEXT("gain")));
			S.Actions.Add(Action);
		}
		Liste.Add(MoveTemp(S));
	}
	return Liste;
}

const FSalle* APalmeraLieux::SalleDuLot(int32 I, int32 J, int32 A, int32 B)
{
	for (const FSalle& S : Salles())
	{
		if (S.BlocI == I && S.BlocJ == J && S.LotA == A && S.LotB == B)
		{
			return &S;
		}
	}
	return nullptr;
}

APalmeraLieux* APalmeraLieux::Lire(const UObject* Contexte)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (!Monde)
	{
		return nullptr;
	}
	TActorIterator<APalmeraLieux> It(Monde);
	return It ? *It : nullptr;
}

APalmeraLieux::APalmeraLieux()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));
	FParse::Value(FCommandLine::Get(), TEXT("-lieu="), LieuEssai);
	bEssaiPortes = FParse::Param(FCommandLine::Get(), TEXT("essaiportes"));
}

void APalmeraLieux::Dire(const FString& Texte)
{
	Bulle = Texte;
	TempsBulle = 2.5 + Texte.Len() * 0.05;
}

void APalmeraLieux::Vider()
{
	for (USceneComponent* Morceau : Pieces)
	{
		if (Morceau)
		{
			Morceau->DestroyComponent();
		}
	}
	Pieces.Reset();
	for (APalmeraPNJ* Habitant : Habitants)
	{
		if (Habitant)
		{
			Habitant->Destroy();
		}
	}
	Habitants.Reset();
	if (Decor)
	{
		Decor->DestroyComponent();
		Decor = nullptr;
	}
	Actuelle = Aucune;
	TempsBulle = 0.0;
}

void APalmeraLieux::Entrer(int32 Index)
{
	const FSalle& S = Salles()[Index];
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	APalmeraHeros* Heros = J ? J->LireHeros() : nullptr;
	if (!Heros)
	{
		return;
	}
	Vider();
	Actuelle = Index;
	const FVector Origine = Ancre();

	const FString Nom = TEXT("SM_Int_") + S.Id;
	Decor = NewObject<UInstancedStaticMeshComponent>(this);
	Decor->SetupAttachment(RootComponent);
	Decor->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Modeles/Interieurs/%s/StaticMeshes/%s.%s"), *Nom, *Nom, *Nom)));
	Decor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Decor->NumCustomDataFloats = 7;
	Decor->RegisterComponent();
	Decor->AddInstance(FTransform(Origine), true);
	const float Blanc[7] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.5f };
	Decor->SetCustomData(0, Blanc, true);

	for (const FBox& B : S.Boites)
	{
		UBoxComponent* Boite = NewObject<UBoxComponent>(this);
		Boite->SetupAttachment(RootComponent);
		Boite->SetBoxExtent(B.GetExtent() * Metre);
		Boite->SetCollisionProfileName(TEXT("BlockAll"));
		Boite->RegisterComponent();
		Boite->SetWorldLocation(Origine + B.GetCenter() * Metre);
		Pieces.Add(Boite);
	}
	for (const FSalle::FLampe& L : S.Lampes)
	{
		UPointLightComponent* Lampe = NewObject<UPointLightComponent>(this);
		Lampe->SetupAttachment(RootComponent);
		Lampe->SetMobility(EComponentMobility::Movable);
		Lampe->SetIntensityUnits(ELightUnits::Lumens);
		Lampe->SetIntensity(L.Force);
		Lampe->SetLightColor(L.Couleur);
		Lampe->SetAttenuationRadius(L.Rayon);
		Lampe->SetCastShadows(false);
		Lampe->RegisterComponent();
		Lampe->SetWorldLocation(Origine + L.Position * Metre);
		Pieces.Add(Lampe);
	}
	FActorSpawnParameters Regles;
	Regles.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FSalle::FPersonne& P : S.Personnes)
	{
		const FVector Place = Origine + FVector(P.Position.X, P.Position.Y, 0.0) * Metre + FVector(0.0, 0.0, 92.0);
		if (APalmeraPNJ* Habitant = GetWorld()->SpawnActor<APalmeraPNJ>(Place, FRotator(0.0, P.Cap, 0.0), Regles))
		{
			FFichePNJ Fiche;
			Fiche.Modele = P.Modele;
			Habitant->Configurer(Fiche);
			Habitants.Add(Habitant);
		}
	}

	const FVector Depart = Origine + FVector(0.0, -S.Taille.Y * 0.5 + 1.8, 0.0) * Metre + FVector(0.0, 0.0, 95.0);
	Heros->SetActorLocationAndRotation(Depart, FRotator(0.0, 90.0, 0.0), false, nullptr, ETeleportType::ResetPhysics);
	Heros->GetCharacterMovement()->StopMovementImmediately();
	J->SetControlRotation(FRotator(-6.0, 90.0, 0.0));
}

void APalmeraLieux::Sortir()
{
	if (Actuelle == Aucune)
	{
		return;
	}
	const FSalle& S = Salles()[Actuelle];
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	APalmeraHeros* Heros = J ? J->LireHeros() : nullptr;
	Vider();
	if (!Heros)
	{
		return;
	}
	const FVector2D Porte = S.Porte();
	const double Cap = S.Facade().Rotation().Yaw;
	Heros->SetActorLocationAndRotation(EnMetres(Porte.X, Porte.Y, NiveauTrottoir + 1.0), FRotator(0.0, Cap, 0.0), false, nullptr, ETeleportType::ResetPhysics);
	Heros->GetCharacterMovement()->StopMovementImmediately();
	J->SetControlRotation(FRotator(-6.0, Cap, 0.0));
}

void APalmeraLieux::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	Invite.Reset();
	TempsBulle = FMath::Max(TempsBulle - Dt, 0.0);
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	APalmeraHeros* Heros = J ? J->LireHeros() : nullptr;
	if (!Heros)
	{
		return;
	}

	// Essai : entrer directement dans un lieu au lancement
	if (!LieuEssai.IsEmpty())
	{
		TempsEssai -= Dt;
		if (TempsEssai <= 0.0)
		{
			Entrer(Salles().IndexOfByPredicate([this](const FSalle& S) { return S.Id == LieuEssai; }));
			LieuEssai.Reset();
		}
	}

	// Essai : passe chaque porte dans les deux sens et note où le héros se tient ensuite
	if (bEssaiPortes && Prochaine == Aucune)
	{
		TempsEssai -= Dt;
		if (TempsEssai <= 0.0 && PorteEssai < Salles().Num())
		{
			const FVector M = Heros->GetActorLocation() / Metre;
			if (bEssaiDedans)
			{
				J->Noter(FString::Printf(TEXT("dedans %s : (%.1f, %.1f, %.1f), decor %s, habitants %d"), *Salles()[PorteEssai].Id, M.X, M.Y, M.Z, Decor && Decor->GetStaticMesh() ? TEXT("ok") : TEXT("ABSENT"), Habitants.Num()));
				Prochaine = VersDehors;
				bEssaiDedans = false;
				PorteEssai++;
			}
			else
			{
				if (PorteEssai > 0)
				{
					J->Noter(FString::Printf(TEXT("  ressorti : (%.1f, %.1f, %.1f)"), M.X, M.Y, M.Z));
				}
				if (PorteEssai < Salles().Num())
				{
					Prochaine = PorteEssai;
					bEssaiDedans = true;
				}
			}
			TempsEssai = 1.6;
		}
	}

	if (Prochaine != Aucune)
	{
		Noir = FMath::Min(Noir + Dt / 0.3, 1.0);
		if (Noir >= 1.0)
		{
			if (Prochaine == VersDehors)
			{
				Sortir();
			}
			else
			{
				Entrer(Prochaine);
			}
			Prochaine = Aucune;
		}
		return;
	}
	Noir = FMath::Max(Noir - Dt / 0.4, 0.0);

	APalmeraHistoire* Recit = APalmeraHistoire::Lire(this);
	if (J->GetPawn() != Heros || (Recit && Recit->SceneActive()))
	{
		return;
	}
	const FVector Ici = Heros->GetActorLocation();
	const bool bAction = J->DemandeAction();
	APalmeraPartie* Partie = APalmeraPartie::Lire(this);

	if (Actuelle == Aucune)
	{
		const TArray<FSalle>& Liste = Salles();
		for (int32 I = 0; I < Liste.Num(); I++)
		{
			const FVector2D Porte = Liste[I].Porte();
			if (FVector2D::Distance(FVector2D(Ici.X, Ici.Y) / Metre, Porte) < 2.8)
			{
				Invite = FString::Printf(TEXT("E : entrer - %s"), *Liste[I].Nom);
				if (bAction)
				{
					if (Partie && Partie->Recherche > 0)
					{
						Partie->Annoncer(TEXT("Impossible d'entrer : la police te cherche"), 3.0);
					}
					else
					{
						Prochaine = I;
					}
				}
				break;
			}
		}
		return;
	}

	const FSalle& S = Salles()[Actuelle];
	const FVector2D Local = FVector2D(Ici.X - Ancre().X, Ici.Y - Ancre().Y) / Metre;
	// Le héros ne devrait quitter la salle que par la porte : si l'histoire le ramène en ville, on range tout
	if (FMath::Abs(Local.X) > 200.0 || FMath::Abs(Local.Y) > 200.0 || Ici.Z < Ancre().Z - 2000.0)
	{
		Vider();
		return;
	}
	if (FVector2D::Distance(Local, FVector2D(0.0, -S.Taille.Y * 0.5)) < 3.0)
	{
		Invite = FString::Printf(TEXT("E : sortir de %s"), *S.Nom);
		if (bAction)
		{
			Prochaine = VersDehors;
		}
		return;
	}
	for (int32 I = 0; I < S.Personnes.Num(); I++)
	{
		if (Habitants.IsValidIndex(I) && Habitants[I] && FVector::Dist2D(Habitants[I]->GetActorLocation(), Ici) < 220.0)
		{
			Invite = TEXT("E : parler");
			if (bAction)
			{
				Habitants[I]->Regarder(Ici);
				Dire(S.Personnes[I].Phrase);
			}
			return;
		}
	}
	for (const FSalle::FAction& A : S.Actions)
	{
		if (FVector2D::Distance(Local, A.Position) < A.Rayon)
		{
			Invite = A.Invite;
			if (bAction && Partie)
			{
				if (Partie->Argent < A.Prix)
				{
					Dire(TEXT("Tu n'as pas assez d'argent."));
				}
				else
				{
					Partie->Argent += A.Gain - A.Prix;
					Dire(A.Message);
				}
			}
			return;
		}
	}
}
