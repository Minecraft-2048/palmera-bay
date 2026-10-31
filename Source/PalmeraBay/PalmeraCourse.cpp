#include "PalmeraCourse.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "PalmeraConfig.h"
#include "PalmeraJoueur.h"
#include "PalmeraMonde.h"
#include "PalmeraVoiture.h"

using namespace Palmera;

namespace
{
	constexpr double RayonVirage = 15.0;
	constexpr double DemiLargeur = 8.0;
	constexpr int32 NombreRivaux = 5;

	const struct { const TCHAR* Nom; FLinearColor Couleur; double Talent; double Decalage; } Rivaux[NombreRivaux] = {
		{ TEXT("Léa Marin"), FLinearColor(0.02f, 0.25f, 0.7f), 0.99, -2.2 },
		{ TEXT("Max Turbo"), FLinearColor(0.85f, 0.5f, 0.02f), 0.96, 2.2 },
		{ TEXT("Zoé Rallye"), FLinearColor(0.75f, 0.75f, 0.78f), 0.93, -1.0 },
		{ TEXT("Noé Piste"), FLinearColor(0.05f, 0.45f, 0.2f), 0.9, 1.0 },
		{ TEXT("Ana Drift"), FLinearColor(0.45f, 0.05f, 0.55f), 0.87, 0.0 }
	};
	const TCHAR* NomsVoitures[] = { TEXT("Coupé sport"), TEXT("Tout-terrain"), TEXT("Berline"), TEXT("Citadine"), TEXT("SUV") };

	FVector2D Noeud(const FIntPoint& N)
	{
		return FVector2D(N.X * Pas, N.Y * Pas);
	}

	FFicheVoiture FicheDe(int32 Modele)
	{
		switch (Modele)
		{
		case 0: return APalmeraVoiture::FicheSport();
		case 1: return APalmeraVoiture::FicheToutTerrain();
		case 2: return APalmeraVoiture::FicheCivile(0);
		case 3: return APalmeraVoiture::FicheCivile(1);
		default: return APalmeraVoiture::FicheCivile(2);
		}
	}
}

APalmeraCourse* APalmeraCourse::Lire(const UObject* Contexte)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (!Monde)
	{
		return nullptr;
	}
	TActorIterator<APalmeraCourse> It(Monde);
	return It ? *It : nullptr;
}

APalmeraCourse::APalmeraCourse()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));

	FCircuit A;
	A.Nom = TEXT("Front de mer");
	A.Heure = 17.6;
	A.Noeuds = { FIntPoint(-8, -4), FIntPoint(-8, 4), FIntPoint(-6, 4), FIntPoint(-6, -4) };
	FCircuit B;
	B.Nom = TEXT("Centre-ville de nuit");
	B.Heure = 21.5;
	B.Noeuds = { FIntPoint(-2, -4), FIntPoint(3, -4), FIntPoint(3, -2), FIntPoint(1, -2), FIntPoint(1, 2), FIntPoint(-2, 2) };
	FCircuit C;
	C.Nom = TEXT("Tour des quartiers");
	C.Heure = 11.0;
	C.Tours = 2;
	C.Noeuds = { FIntPoint(-7, -5), FIntPoint(-2, -5), FIntPoint(-2, 2), FIntPoint(4, 2), FIntPoint(4, 6), FIntPoint(-7, 6) };
	FCircuit D;
	D.Nom = TEXT("Orage sur le port");
	D.Heure = 15.0;
	D.Pluie = 1.0;
	D.Noeuds = { FIntPoint(-3, -7), FIntPoint(3, -7), FIntPoint(3, -5), FIntPoint(-3, -5) };
	ListeCircuits = { A, B, C, D };
}

void APalmeraCourse::BeginPlay()
{
	Super::BeginPlay();
	bEssai = FParse::Param(FCommandLine::Get(), TEXT("essaicourse"));
	if (bEssai)
	{
		FParse::Value(FCommandLine::Get(), TEXT("-circuit="), ChoixCircuit);
		ToursChoisis = 1;
	}
	Preparer();
}

FString APalmeraCourse::NomVoiture() const
{
	return NomsVoitures[FMath::Clamp(ChoixVoiture, 0, 4)];
}

double APalmeraCourse::Record() const
{
	double Valeur = -1.0;
	GConfig->GetDouble(TEXT("PalmeraRacing"), *FString::Printf(TEXT("Record%d"), ChoixCircuit), Valeur, GGameUserSettingsIni);
	return Valeur;
}

double FConcurrent::Progression() const
{
	return Tours * 1000000.0 + S;
}

TArray<const FConcurrent*> APalmeraCourse::Classement() const
{
	TArray<const FConcurrent*> Liste;
	for (const FConcurrent& C : Concurrents)
	{
		Liste.Add(&C);
	}
	Liste.Sort([](const FConcurrent& A, const FConcurrent& B)
	{
		if ((A.Arrivee >= 0.0) != (B.Arrivee >= 0.0))
		{
			return A.Arrivee >= 0.0;
		}
		return A.Arrivee >= 0.0 ? A.Arrivee < B.Arrivee : A.Progression() > B.Progression();
	});
	return Liste;
}

int32 APalmeraCourse::Rang() const
{
	const TArray<const FConcurrent*> Liste = Classement();
	return Concurrents.Num() > 0 ? Liste.IndexOfByKey(&Concurrents[0]) + 1 : 1;
}

FVector2D APalmeraCourse::Point(int32 Index, double Decalage) const
{
	const int32 N = Points.Num();
	const FVector2D P = Points[(Index % N + N) % N];
	const FVector2D Suite = Points[((Index + 2) % N + N) % N];
	const FVector2D Sens = (Suite - P).GetSafeNormal();
	return P + FVector2D(-Sens.Y, Sens.X) * Decalage;
}

double APalmeraCourse::Cap(int32 Index) const
{
	const FVector2D D = Point(Index + 2, 0.0) - Point(Index, 0.0);
	return FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
}

void APalmeraCourse::ConstruireTrace(const FCircuit& C)
{
	Points.Reset();
	const int32 N = C.Noeuds.Num();
	for (int32 I = 0; I < N; I++)
	{
		const FVector2D A = Noeud(C.Noeuds[(I + N - 1) % N]);
		const FVector2D B = Noeud(C.Noeuds[I]);
		const FVector2D D = Noeud(C.Noeuds[(I + 1) % N]);
		const FVector2D U = (B - A).GetSafeNormal();
		const FVector2D V = (D - B).GetSafeNormal();
		// Ligne droite jusqu'au virage, puis arc arrondi
		const FVector2D Debut = A + U * RayonVirage;
		const FVector2D Fin = B - U * RayonVirage;
		const int32 Pas2 = FMath::CeilToInt(FVector2D::Distance(Debut, Fin) / 2.0);
		for (int32 K = 0; K < Pas2; K++)
		{
			Points.Add(FMath::Lerp(Debut, Fin, double(K) / Pas2));
		}
		const FVector2D Sortie = B + V * RayonVirage;
		for (int32 K = 0; K < 12; K++)
		{
			const double T = K / 12.0;
			Points.Add(Fin * ((1.0 - T) * (1.0 - T)) + B * (2.0 * T * (1.0 - T)) + Sortie * (T * T));
		}
	}
	// Le tour commence au milieu de la première ligne droite
	const FVector2D Milieu = (Noeud(C.Noeuds[0]) + Noeud(C.Noeuds[1])) * 0.5;
	int32 Debut = 0;
	double Meilleure = 1e12;
	for (int32 I = 0; I < Points.Num(); I++)
	{
		const double D = FVector2D::DistSquared(Points[I], Milieu);
		if (D < Meilleure)
		{
			Meilleure = D;
			Debut = I;
		}
	}
	TArray<FVector2D> Tournes;
	const int32 Total = Points.Num();
	for (int32 I = 0; I < Total; I++)
	{
		Tournes.Add(Points[(I + Debut) % Total]);
	}
	Points = MoveTemp(Tournes);

	Cumul.SetNum(Total);
	VMax.SetNum(Total);
	Longueur = 0.0;
	for (int32 I = 0; I < Total; I++)
	{
		Cumul[I] = Longueur;
		const FVector2D A = Points[(I + Total - 1) % Total];
		const FVector2D B = Points[I];
		const FVector2D D = Points[(I + 1) % Total];
		const double Ds = FMath::Max(FVector2D::Distance(B, D), 0.1);
		Longueur += Ds;
		const double Angle = FMath::Abs(FMath::Acos(FMath::Clamp(FVector2D::DotProduct((B - A).GetSafeNormal(), (D - B).GetSafeNormal()), -1.0, 1.0)));
		const double Courbure = Angle / Ds;
		const double Adherence = (C.Pluie > 0.0 ? 8.5 : 12.5);
		VMax[I] = Courbure > 1e-4 ? FMath::Min(FMath::Sqrt(Adherence / Courbure), 90.0) : 90.0;
	}
	// Freiner assez tôt : on remonte le tour en limitant la vitesse à ce que les freins peuvent absorber
	for (int32 Passe = 0; Passe < 2; Passe++)
	{
		for (int32 I = Total - 1; I >= 0; I--)
		{
			const int32 Suite = (I + 1) % Total;
			const double Ds = FVector2D::Distance(Points[I], Points[Suite]);
			VMax[I] = FMath::Min(VMax[I], FMath::Sqrt(VMax[Suite] * VMax[Suite] + 2.0 * 7.5 * Ds));
		}
	}
}

void APalmeraCourse::PoserPiece(const TCHAR* Nom, const FVector& Position, double Lacet, const FVector& DemiBoite)
{
	const FString Chemin = FString::Printf(TEXT("/Game/Modeles/Mobilier/%s/StaticMeshes/%s.%s"), Nom, Nom, Nom);
	UStaticMeshComponent* Morceau = NewObject<UStaticMeshComponent>(this);
	Morceau->SetupAttachment(RootComponent);
	Morceau->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Chemin));
	Morceau->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Morceau->RegisterComponent();
	Morceau->SetWorldLocationAndRotation(Position, FRotator(0.0, Lacet, 0.0));
	Pieces.Add(Morceau);
	if (!DemiBoite.IsZero())
	{
		UBoxComponent* Boite = NewObject<UBoxComponent>(this);
		Boite->SetupAttachment(RootComponent);
		Boite->SetBoxExtent(DemiBoite * Metre);
		Boite->SetCollisionProfileName(TEXT("BlockAll"));
		Boite->RegisterComponent();
		Boite->SetWorldLocationAndRotation(Position + FVector(0.0, 0.0, DemiBoite.Z * Metre), FRotator(0.0, Lacet, 0.0));
		Pieces.Add(Boite);
	}
}

void APalmeraCourse::PoserBarrieres(const FCircuit& C)
{
	// Toutes les rues qui croisent le tracé sont fermées : on note les directions empruntées à chaque carrefour
	TMap<FIntPoint, TArray<FIntPoint>> Utilises;
	const int32 N = C.Noeuds.Num();
	for (int32 I = 0; I < N; I++)
	{
		FIntPoint A = C.Noeuds[I];
		const FIntPoint B = C.Noeuds[(I + 1) % N];
		const FIntPoint Sens(FMath::Sign(B.X - A.X), FMath::Sign(B.Y - A.Y));
		while (A != B)
		{
			Utilises.FindOrAdd(A).AddUnique(Sens);
			A += Sens;
			Utilises.FindOrAdd(A).AddUnique(FIntPoint(-Sens.X, -Sens.Y));
		}
	}
	static const FIntPoint Directions[] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	for (const TPair<FIntPoint, TArray<FIntPoint>>& Paire : Utilises)
	{
		for (const FIntPoint& D : Directions)
		{
			const FIntPoint Voisin = Paire.Key + D;
			if (Paire.Value.Contains(D) || FMath::Abs(Voisin.X) > 8 || FMath::Abs(Voisin.Y) > 8)
			{
				continue;
			}
			const FVector2D Place = Noeud(Paire.Key) + FVector2D(D.X, D.Y) * (DemiLargeur + 3.0);
			PoserPiece(TEXT("SM_Barriere"), EnMetres(Place.X, Place.Y, 0.0), D.X != 0 ? 90.0 : 0.0, FVector(9.0, 0.45, 0.6));
		}
	}
	// Portique d'arrivée
	const double Lacet = Cap(0);
	const FVector2D Ligne = Point(0, 0.0);
	PoserPiece(TEXT("SM_Arche"), EnMetres(Ligne.X, Ligne.Y, 0.0), Lacet + 90.0, FVector::ZeroVector);
}

APalmeraVoiture* APalmeraCourse::CreerVoiture(int32 Modele, const FLinearColor& Peinture, const FVector2D& Position, double Lacet)
{
	FFicheVoiture Fiche = FicheDe(Modele);
	Fiche.Peinture = Peinture;
	const FTransform Place(FRotator(0.0, Lacet, 0.0), EnMetres(Position.X, Position.Y, 0.0) + FVector(0.0, 0.0, Fiche.CentreBoite.Z));
	APalmeraVoiture* V = GetWorld()->SpawnActorDeferred<APalmeraVoiture>(APalmeraVoiture::StaticClass(), Place);
	V->Configurer(Fiche);
	V->FinishSpawning(Place);
	V->FacteurDegats = 0.0;
	V->ActiverPhysique(true);
	Voitures.Add(V);
	return V;
}

void APalmeraCourse::Vider()
{
	for (UActorComponent* Morceau : Pieces)
	{
		if (Morceau)
		{
			Morceau->DestroyComponent();
		}
	}
	Pieces.Reset();
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	if (J)
	{
		J->LacherVoiture();
	}
	for (APalmeraVoiture* V : Voitures)
	{
		if (V)
		{
			V->Destroy();
		}
	}
	Voitures.Reset();
	Concurrents.Reset();
}

void APalmeraCourse::Preparer()
{
	Vider();
	const FCircuit& C = ListeCircuits[ChoixCircuit];
	ToursChoisis = FMath::Clamp(ToursChoisis, 1, 9);
	ConstruireTrace(C);
	PoserBarrieres(C);
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		It->Heure = C.Heure;
		It->Pluie = It->PluieCible = C.Pluie;
		It->bMeteoAuto = false;
	}

	// La grille de départ : deux colonnes derrière la ligne, le joueur en dernier
	const int32 Total = Points.Num();
	const auto IndexGrille = [&](int32 Rang)
	{
		const double Recul = 10.0 + Rang * 8.0;
		int32 I = Total - 1;
		while (I > 0 && Longueur - Cumul[I] < Recul)
		{
			I--;
		}
		return I;
	};
	const int32 Modele = ChoixVoiture <= 1 ? 0 : ChoixVoiture;
	for (int32 R = 0; R < NombreRivaux; R++)
	{
		FConcurrent Rival;
		const int32 Index = IndexGrille(R);
		const double Cote = (R % 2 == 0) ? -3.4 : 3.4;
		Rival.Voiture = CreerVoiture(Modele, Rivaux[R].Couleur, Point(Index, Cote), Cap(Index));
		Rival.Voiture->bEnnemie = true;
		Rival.Nom = Rivaux[R].Nom;
		Rival.Couleur = Rivaux[R].Couleur;
		Rival.Talent = Rivaux[R].Talent;
		Rival.Decalage = Rivaux[R].Decalage;
		Rival.Index = Index;
		Rival.S = Cumul[Index] - Longueur;
		Concurrents.Add(Rival);
	}
	FConcurrent Moi;
	const int32 Index = IndexGrille(NombreRivaux);
	Moi.Voiture = CreerVoiture(ChoixVoiture, FLinearColor(0.72f, 0.03f, 0.05f), Point(Index, 3.4), Cap(Index));
	Moi.Nom = TEXT("Toi");
	Moi.Couleur = FLinearColor(1.f, 0.3f, 0.25f);
	Moi.Index = Index;
	Moi.S = Cumul[Index] - Longueur;
	Concurrents.Insert(Moi, 0);

	if (APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()))
	{
		J->PrendreVoiture(Moi.Voiture);
		J->bCommandesGelees = true;
	}
	Etat = EEtatCourse::Menu;
	Chrono = TempsTour = 0.0;
	MeilleurTour = -1.0;
	bNouveauRecord = false;
	if (bEssai)
	{
		Etat = EEtatCourse::Depart;
		Compte = 3.0;
	}
}

void APalmeraCourse::CommandesMenu()
{
	APlayerController* J = GetWorld()->GetFirstPlayerController();
	const bool bHaut = J->WasInputKeyJustPressed(EKeys::Up) || J->WasInputKeyJustPressed(EKeys::Z) || J->WasInputKeyJustPressed(EKeys::W);
	const bool bBas = J->WasInputKeyJustPressed(EKeys::Down) || J->WasInputKeyJustPressed(EKeys::S);
	const bool bGauche = J->WasInputKeyJustPressed(EKeys::Left) || J->WasInputKeyJustPressed(EKeys::Q) || J->WasInputKeyJustPressed(EKeys::A);
	const bool bDroite = J->WasInputKeyJustPressed(EKeys::Right) || J->WasInputKeyJustPressed(EKeys::D);
	if (bHaut || bBas)
	{
		ChoixLigne = (ChoixLigne + (bBas ? 1 : 2)) % 3;
	}
	if (bGauche || bDroite)
	{
		const int32 Sens = bDroite ? 1 : -1;
		if (ChoixLigne == 0)
		{
			ChoixCircuit = (ChoixCircuit + Sens + ListeCircuits.Num()) % ListeCircuits.Num();
			ToursChoisis = ListeCircuits[ChoixCircuit].Tours;
			Preparer();
		}
		else if (ChoixLigne == 1)
		{
			ChoixVoiture = (ChoixVoiture + Sens + 5) % 5;
			Preparer();
		}
		else
		{
			ToursChoisis = FMath::Clamp(ToursChoisis + Sens, 1, 9);
		}
	}
	if (J->WasInputKeyJustPressed(EKeys::Enter) || J->WasInputKeyJustPressed(EKeys::SpaceBar) || J->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		Etat = EEtatCourse::Depart;
		Compte = 4.0;
	}
}

void APalmeraCourse::Avancer(FConcurrent& C)
{
	const int32 Total = Points.Num();
	const FVector Ici = C.Voiture->GetActorLocation() / Metre;
	int32 Meilleur = C.Index;
	double Plus = 1e12;
	for (int32 I = -6; I < 70; I++)
	{
		const int32 K = (C.Index + I + Total) % Total;
		const double D = FVector2D::DistSquared(Points[K], FVector2D(Ici.X, Ici.Y));
		if (D < Plus)
		{
			Plus = D;
			Meilleur = K;
		}
	}
	const int32 Avant = C.Index;
	C.Index = Meilleur;
	C.S = Cumul[Meilleur];
	if (Meilleur > Total / 2 && Meilleur < Total * 3 / 4)
	{
		C.bMoitie = true;
	}
	// La ligne est franchie quand on passe de la fin du tracé à son début
	if (Avant > Total * 3 / 4 && Meilleur < Total / 4 && C.bMoitie)
	{
		C.Tours++;
		C.bMoitie = false;
		if (&C == &Concurrents[0] && C.Tours > 0)
		{
			MeilleurTour = MeilleurTour < 0.0 ? TempsTour : FMath::Min(MeilleurTour, TempsTour);
			TempsTour = 0.0;
		}
		if (C.Tours >= ToursChoisis && C.Arrivee < 0.0)
		{
			C.Arrivee = Chrono;
		}
	}
}

void APalmeraCourse::Piloter(FConcurrent& C, double Dt)
{
	APalmeraVoiture* V = C.Voiture;
	const double Vitesse = V->GetVelocity().Size() / Metre;
	if (C.Arrivee >= 0.0)
	{
		V->Commander(0.0, 0.0, true);
		return;
	}
	const FVector Ici = V->GetActorLocation() / Metre;

	// Coincé : le concurrent repart du tracé, comme remis en piste par un commissaire
	C.Bloque = Vitesse < 1.5 ? C.Bloque + Dt : 0.0;
	if (C.Bloque > 2.5)
	{
		C.Bloque = 0.0;
		const FVector2D Reprise = Point(C.Index + 2, C.Decalage);
		V->SetActorLocationAndRotation(EnMetres(Reprise.X, Reprise.Y, 0.0) + FVector(0.0, 0.0, V->LireFiche().CentreBoite.Z + 15.0), FRotator(0.0, Cap(C.Index + 2), 0.0), false, nullptr, ETeleportType::ResetPhysics);
		return;
	}

	const int32 Vise = C.Index + FMath::Max(FMath::RoundToInt((7.0 + Vitesse * 0.5) / 2.0), 3);
	const FVector2D But = Point(Vise, C.Decalage);
	const double CapBut = FMath::RadiansToDegrees(FMath::Atan2(But.Y - Ici.Y, But.X - Ici.X));
	const double Ecart = FMath::FindDeltaAngleDegrees(V->GetActorRotation().Yaw, CapBut);

	// Vitesse permise un peu plus loin, modulée par le talent et l'écart avec le joueur
	const int32 Total = Points.Num();
	double Permise = VMax[(C.Index + 2) % Total] * C.Talent;
	const double Retard = Concurrents[0].Progression() - C.Progression();
	if (C.Tours == Concurrents[0].Tours)
	{
		Permise *= FMath::Clamp(1.0 + Retard / 1500.0, 0.93, 1.06);
	}
	const double Gaz = Vitesse < Permise ? 1.0 : (Vitesse > Permise + 1.2 ? -1.0 : 0.15);
	V->Commander(Gaz, FMath::Clamp(Ecart * 1.2 / FMath::Max(V->BraquageMax(), 1.0), -1.0, 1.0), false);
}

void APalmeraCourse::Finir()
{
	Etat = EEtatCourse::Resultats;
	const double Temps = Concurrents[0].Arrivee;
	if (bEssai)
	{
		if (APalmeraJoueur* Pilote = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()))
		{
			Pilote->Noter(FString::Printf(TEXT("course %s, %.0f m, tour en %.1f s"), *ListeCircuits[ChoixCircuit].Nom, Longueur, Temps));
			for (const FConcurrent* C : Classement())
			{
				Pilote->Noter(FString::Printf(TEXT("  %s : arrivee %.1f s, tours %d, position %.0f m"), *C->Nom, C->Arrivee, C->Tours, C->S));
			}
		}
	}
	const double Ancien = Record();
	if (Temps > 0.0 && (Ancien < 0.0 || Temps < Ancien) && ToursChoisis == ListeCircuits[ChoixCircuit].Tours)
	{
		GConfig->SetDouble(TEXT("PalmeraRacing"), *FString::Printf(TEXT("Record%d"), ChoixCircuit), Temps, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		bNouveauRecord = true;
	}
	if (APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController()))
	{
		J->bCommandesGelees = true;
	}
}

void APalmeraCourse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	APalmeraJoueur* J = Cast<APalmeraJoueur>(GetWorld()->GetFirstPlayerController());
	if (!J || Concurrents.Num() == 0)
	{
		return;
	}

	switch (Etat)
	{
	case EEtatCourse::Menu:
		CommandesMenu();
		for (int32 I = 1; I < Concurrents.Num(); I++)
		{
			Concurrents[I].Voiture->Commander(0.0, 0.0, true);
		}
		break;

	case EEtatCourse::Depart:
		Compte -= Dt;
		for (int32 I = 1; I < Concurrents.Num(); I++)
		{
			Concurrents[I].Voiture->Commander(0.0, 0.0, true);
		}
		if (Compte <= 1.0)
		{
			Etat = EEtatCourse::Course;
			J->bCommandesGelees = false;
		}
		break;

	case EEtatCourse::Course:
		Chrono += Dt;
		TempsTour += Dt;
		for (FConcurrent& C : Concurrents)
		{
			Avancer(C);
		}
		for (int32 I = bEssai ? 0 : 1; I < Concurrents.Num(); I++)
		{
			Piloter(Concurrents[I], Dt);
		}
		if (J->WasInputKeyJustPressed(EKeys::R))
		{
			// Remise en piste du joueur, en cas de sortie de route
			FConcurrent& Moi = Concurrents[0];
			const FVector2D Reprise = Point(Moi.Index, 0.0);
			Moi.Voiture->SetActorLocationAndRotation(EnMetres(Reprise.X, Reprise.Y, 0.0) + FVector(0.0, 0.0, Moi.Voiture->LireFiche().CentreBoite.Z + 15.0), FRotator(0.0, Cap(Moi.Index), 0.0), false, nullptr, ETeleportType::ResetPhysics);
		}
		if (Concurrents[0].Arrivee >= 0.0)
		{
			ApresArrivee += Dt;
			if (ApresArrivee > 1.0)
			{
				ApresArrivee = 0.0;
				Finir();
			}
		}
		break;

	case EEtatCourse::Resultats:
		for (FConcurrent& C : Concurrents)
		{
			C.Voiture->Commander(0.0, 0.0, true);
		}
		if (J->WasInputKeyJustPressed(EKeys::Enter) || J->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
		{
			Preparer();
		}
		break;
	}
}
