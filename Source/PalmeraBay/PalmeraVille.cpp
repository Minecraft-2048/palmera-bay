#include "PalmeraVille.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "PalmeraConfig.h"
#include "PalmeraMonde.h"
#include "PalmeraPartie.h"

using namespace Palmera;

namespace
{
	FLinearColor Couleur(uint8 R, uint8 V, uint8 B)
	{
		return FLinearColor::FromSRGBColor(FColor(R, V, B));
	}

	// Couleurs d'enduit de la ville, du blanc cassé aux pastels de bord de mer.
	const TArray<FLinearColor>& Pastels()
	{
		static const TArray<FLinearColor> Liste = {
			Couleur(244, 240, 230), Couleur(250, 232, 205), Couleur(250, 214, 190), Couleur(247, 190, 175),
			Couleur(180, 224, 208), Couleur(176, 214, 232), Couleur(214, 200, 232), Couleur(238, 220, 170),
			Couleur(232, 232, 226), Couleur(205, 200, 190)
		};
		return Liste;
	}

	const TArray<FLinearColor>& Sobres()
	{
		static const TArray<FLinearColor> Liste = {
			Couleur(214, 210, 200), Couleur(190, 188, 182), Couleur(170, 168, 165), Couleur(226, 216, 196), Couleur(150, 155, 160)
		};
		return Liste;
	}

	const TArray<FLinearColor>& Accents()
	{
		static const TArray<FLinearColor> Liste = {
			Couleur(232, 62, 88), Couleur(38, 190, 196), Couleur(246, 176, 50), Couleur(108, 86, 214),
			Couleur(60, 170, 104), Couleur(238, 110, 56), Couleur(240, 70, 170)
		};
		return Liste;
	}

	const TCHAR* Prendre(FRandomStream& H, std::initializer_list<const TCHAR*> Liste)
	{
		return *(Liste.begin() + H.RandRange(0, int32(Liste.size()) - 1));
	}
}

APalmeraVille::APalmeraVille()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Racine")));
	RootComponent->SetMobility(EComponentMobility::Static);
}

EQuartier APalmeraVille::Quartier(int32 I, int32 J)
{
	if (I == RueMin)
	{
		return EQuartier::Plage;
	}
	if ((I == -5 || I == -4) && (J == -1 || J == 0))
	{
		return EQuartier::Parc;
	}
	if (J <= -7 || (I >= 4 && J <= -5))
	{
		return EQuartier::Port;
	}
	if (I >= -2 && I <= 3 && J >= -4 && J <= 1)
	{
		return EQuartier::Centre;
	}
	return J >= 3 ? EQuartier::Residentiel : EQuartier::Mixte;
}

UStaticMesh* APalmeraVille::Maillage(const FString& Dossier, const FString& Nom) const
{
	const FString Chemin = FString::Printf(TEXT("/Game/Modeles/%s/%s/StaticMeshes/%s.%s"), *Dossier, *Nom, *Nom, *Nom);
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Chemin);
	UE_CLOG(!M, LogTemp, Warning, TEXT("PALMERA modele absent : %s"), *Chemin);
	return M;
}

UInstancedStaticMeshComponent* APalmeraVille::Groupe(const FString& Dossier, const FString& Nom)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Existant = Groupes.Find(Nom))
	{
		return *Existant;
	}
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
	I->SetMobility(EComponentMobility::Static);
	I->SetupAttachment(RootComponent);
	I->SetStaticMesh(Maillage(Dossier, Nom));
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	I->SetCanEverAffectNavigation(false);
	I->NumCustomDataFloats = 7;
	I->RegisterComponent();
	Groupes.Add(Nom, I);
	return I;
}

int32 APalmeraVille::Poser(const FString& Dossier, const FString& Nom, const FVector& Position, double Lacet, const TArrayView<const float>& Donnees)
{
	UInstancedStaticMeshComponent* I = Groupe(Dossier, Nom);
	const int32 Index = I->AddInstance(FTransform(FRotator(0.0, Lacet, 0.0), Position), true);
	if (Donnees.Num() > 0)
	{
		I->SetCustomData(Index, Donnees, false);
	}
	return Index;
}

void APalmeraVille::PoserBatiment(const FString& Nom, double X, double Y, double Lacet, FRandomStream& H)
{
	const bool bPastel = Nom.StartsWith(TEXT("SM_deco")) || Nom.StartsWith(TEXT("SM_maison")) || Nom.StartsWith(TEXT("SM_boutiques"))
		|| Nom.StartsWith(TEXT("SM_residence"));
	const TArray<FLinearColor>& Liste = bPastel ? Pastels() : Sobres();
	const FLinearColor T = Liste[H.RandRange(0, Liste.Num() - 1)];
	const FLinearColor A = Accents()[H.RandRange(0, Accents().Num() - 1)];
	const float Donnees[7] = { T.R, T.G, T.B, A.R, A.G, A.B, H.FRand() };
	const FVector Position = EnMetres(X, Y, NiveauTrottoir);
	Poser(TEXT("Immeubles"), Nom, Position, Lacet, Donnees);
	NbBatiments++;

	if (const UStaticMesh* M = Groupe(TEXT("Immeubles"), Nom)->GetStaticMesh())
	{
		const FBox B = M->GetBoundingBox();
		const FVector Taille = B.GetSize();
		// Le boîtier ignore antennes et grues de toit : on garde 95 % de la hauteur
		const FVector Decalage = FRotator(0.0, Lacet, 0.0).RotateVector(FVector(B.GetCenter().X, B.GetCenter().Y, 0.0));
		const FTransform T2(FRotator(0.0, Lacet, 0.0), Position + Decalage + FVector(0.0, 0.0, Taille.Z * 0.475),
			FVector(FMath::Min(Taille.X, 6400.0), FMath::Min(Taille.Y, 8000.0), Taille.Z * 0.95) / 100.0);
		CollisionsBoites->AddInstance(T2, true);
	}
}

void APalmeraVille::PoserPalmier(double X, double Y, FRandomStream& H)
{
	const FString Nom = Prendre(H, { TEXT("SM_Palmier_a"), TEXT("SM_Palmier_b"), TEXT("SM_Palmier_c"), TEXT("SM_Palmier_d") });
	Poser(TEXT("Mobilier"), Nom, EnMetres(X, Y, NiveauTrottoir), H.FRandRange(0.0f, 360.0f));
	PoserObstacle(X, Y, 0.34, 6.0);
}

void APalmeraVille::PoserVoitureGaree(double X, double Y, double Lacet, FRandomStream& H)
{
	static const FLinearColor Peintures[] = {
		FLinearColor(0.72f, 0.72f, 0.74f), FLinearColor(0.02f, 0.02f, 0.025f), FLinearColor(0.85f, 0.85f, 0.83f), FLinearColor(0.08f, 0.1f, 0.28f),
		FLinearColor(0.42f, 0.03f, 0.03f), FLinearColor(0.2f, 0.22f, 0.24f), FLinearColor(0.05f, 0.25f, 0.2f), FLinearColor(0.75f, 0.68f, 0.5f)
	};
	const FLinearColor C = Peintures[H.RandRange(0, UE_ARRAY_COUNT(Peintures) - 1)];
	const float Donnees[7] = { C.R, C.G, C.B, 0.f, 0.f, 0.f, H.FRand() };
	const FString Nom = Prendre(H, { TEXT("SM_Garee_a"), TEXT("SM_Garee_b"), TEXT("SM_Garee_a"), TEXT("SM_Garee_c"), TEXT("SM_Garee_d") });
	Poser(TEXT("Mobilier"), Nom, EnMetres(X, Y, NiveauTrottoir), Lacet, Donnees);
	CollisionsBoites->AddInstance(FTransform(FRotator(0.0, Lacet, 0.0), EnMetres(X, Y, NiveauTrottoir + 0.8), FVector(4.6, 1.85, 1.5)), true);
}

void APalmeraVille::PoserObstacle(double X, double Y, double Rayon, double Hauteur){
	CollisionsPoteaux->AddInstance(FTransform(FRotator::ZeroRotator, EnMetres(X, Y, Hauteur * 0.5),
		FVector(Rayon * 2.0, Rayon * 2.0, Hauteur)), true);
}

UStaticMeshComponent* APalmeraVille::Piece(const FString& Dossier, const FString& Nom, const FVector& Position, double Lacet, bool bOmbre)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Static);
	C->SetupAttachment(RootComponent);
	C->SetWorldLocationAndRotation(Position, FRotator(0.0, Lacet, 0.0));
	C->SetStaticMesh(Maillage(Dossier, Nom));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(bOmbre);
	C->SetCanEverAffectNavigation(false);
	C->RegisterComponent();
	return C;
}

void APalmeraVille::BoiteCollision(const FVector& Centre, const FVector& DemiTaille, const FRotator& Rotation)
{
	UBoxComponent* B = NewObject<UBoxComponent>(this);
	B->SetMobility(EComponentMobility::Static);
	B->SetupAttachment(RootComponent);
	B->SetWorldLocationAndRotation(Centre, Rotation);
	B->SetBoxExtent(DemiTaille, false);
	B->SetCollisionProfileName(TEXT("BlockAll"));
	B->SetCanEverAffectNavigation(false);
	B->RegisterComponent();
}

void APalmeraVille::ConstruireSol()
{
	// Chaussée : un seul grand plan, les marquages sont calculés par son matériau
	UStaticMeshComponent* Route = NewObject<UStaticMeshComponent>(this);
	Route->SetMobility(EComponentMobility::Static);
	Route->SetupAttachment(RootComponent);
	Route->SetWorldScale3D(FVector(1942.0, 1942.0, 1.0));
	Route->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Route->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Route.M_Route")));
	Route->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Route->SetCastShadow(false);
	Route->RegisterComponent();
	BoiteCollision(EnMetres(0, 0, -5.0), EnMetres(971, 971, 5.0));

	// Trottoirs extérieurs, promenade, quais au nord, au sud et à l'est
	for (int32 K = RueMin; K < RueMax + 1; K++)
	{
		const double C = (K + 0.5 - (K == RueMax ? 1.0 : 0.0)) * Pas;
		Poser(TEXT("Sol"), TEXT("SM_Bord"), EnMetres(C, BordNord - 9.0), 0.0);
		Poser(TEXT("Sol"), TEXT("SM_Bord"), EnMetres(C, BordSud + 9.0), 180.0);
		Poser(TEXT("Sol"), TEXT("SM_Bord"), EnMetres(BordOuest - 9.0, C), 270.0);
		Poser(TEXT("Sol"), TEXT("SM_Bord"), EnMetres(BordEst + 9.0, C), 90.0);
		Poser(TEXT("Sol"), TEXT("SM_Quai"), EnMetres(C, BordNord - 19.0), 180.0);
		Poser(TEXT("Sol"), TEXT("SM_Quai"), EnMetres(C, BordSud + 19.0), 0.0);
		Poser(TEXT("Sol"), TEXT("SM_Quai"), EnMetres(BordEst + 19.0, C), 270.0);
	}
	BoiteCollision(EnMetres(BordOuest - 9.0, 0, -5.0), EnMetres(2.0, 1000, 5.15));
	BoiteCollision(EnMetres(BordEst + 9.0, 0, -5.0), EnMetres(2.0, 1000, 5.15));
	BoiteCollision(EnMetres(0, BordNord - 9.0, -5.0), EnMetres(1000, 2.0, 5.15));
	BoiteCollision(EnMetres(0, BordSud + 9.0, -5.0), EnMetres(1000, 2.0, 5.15));
	BoiteCollision(EnMetres(0, BordNord - 19.0, -5.0), EnMetres(1000, 8.2, 5.15));
	BoiteCollision(EnMetres(0, BordSud + 19.0, -5.0), EnMetres(1000, 8.2, 5.15));
	BoiteCollision(EnMetres(BordEst + 19.0, 0, -5.0), EnMetres(8.2, 1000, 5.15));

	Piece(TEXT("Sol"), TEXT("SM_Promenade"), EnMetres(BordOuest - 13.5, 0.0), 0.0);
	BoiteCollision(EnMetres(BordOuest - 13.5, 0, -5.0), EnMetres(2.6, 1000, 5.2));
	Piece(TEXT("Sol"), TEXT("SM_Plage"), EnMetres(0, 0));
	// Sable : une grande pente qui passe sous la mer
	const double Pente = FMath::RadiansToDegrees(FMath::Atan(0.0215));
	BoiteCollision(EnMetres(-1125.0, 0.0, -3.1 - 2.5), EnMetres(155, 1040, 2.5), FRotator(Pente, 0.0, 0.0));

	// La paillote de Lou, les parasols, et le yacht de Valmont à quai
	Piece(TEXT("Mobilier"), TEXT("SM_Paillote"), EnMetres(-994.0, -120.0, -0.2), 90.0);
	BoiteCollision(EnMetres(-994.0, -120.0, 1.2), EnMetres(3.6, 4.4, 1.5));
	Piece(TEXT("Mobilier"), TEXT("SM_Yacht"), EnMetres(300.0, -995.0, NiveauMer), 0.0);
	FRandomStream Plage(811);
	for (int32 I = 0; I < 56; I++)
	{
		const double D = Plage.FRandRange(9.0f, 30.0f);
		const double Y = Plage.FRandRange(-900.0f, 900.0f);
		const FLinearColor A = Accents()[Plage.RandRange(0, Accents().Num() - 1)];
		const float Donnees[7] = { 1.f, 1.f, 1.f, A.R, A.G, A.B, Plage.FRand() };
		Poser(TEXT("Mobilier"), TEXT("SM_Parasol"), EnMetres(-975.0 - D, Y, 0.1 - 0.0205 * D), Plage.FRandRange(0.0f, 360.0f), Donnees);
	}

	UStaticMeshComponent* Mer = Piece(TEXT("Sol"), TEXT("SM_Ocean"), EnMetres(0, 0, NiveauMer), 0.0, false);
	Mer->bAffectDynamicIndirectLighting = false;
	Mer->bVisibleInRayTracing = false;
	Mer->SetAffectDistanceFieldLighting(false);

	Piece(TEXT("Sol"), TEXT("SM_Collines"), EnMetres(0, 0));
	BoiteCollision(EnMetres(BordEst + 28.0, 0, 50.0), EnMetres(1.0, 1000, 60.0));
}

void APalmeraVille::ConstruireBloc(int32 I, int32 J)
{
	FRandomStream H((I + 37) * 1009 + (J + 53) * 131);
	const double Cx = (I + 0.5) * Pas;
	const double Cy = (J + 0.5) * Pas;
	const EQuartier Q = Quartier(I, J);

	Poser(TEXT("Sol"), TEXT("SM_Bloc"), EnMetres(Cx, Cy));
	CollisionsBoites->AddInstance(FTransform(FRotator::ZeroRotator, EnMetres(Cx, Cy, NiveauTrottoir * 0.5),
		FVector(106.0, 106.0, NiveauTrottoir)), true);

	const double Lot98 = 98.0;
	if (Q == EQuartier::Plage)
	{
		// Hôtels art déco en première ligne, face à l'océan
		for (int32 K = 0; K < 3; K++)
		{
			const double Y = Cy + (K - 1) * 32.6;
			const TCHAR* Nom = Prendre(H, { TEXT("SM_deco_a"), TEXT("SM_deco_b"), TEXT("SM_deco_c"), TEXT("SM_deco_d") });
			PoserBatiment(Nom, Cx - 25.0, Y, 0.0, H);
			const TCHAR* Arriere = Prendre(H, { TEXT("SM_boutiques_a"), TEXT("SM_boutiques_b"), TEXT("SM_residence_c"), TEXT("SM_boutiques_d") });
			PoserBatiment(Arriere, Cx + 26.0, Y, H.RandRange(0, 3) * 90.0, H);
		}
	}
	else if (Q == EQuartier::Centre)
	{
		for (int32 A = 0; A < 2; A++)
		{
			for (int32 B = 0; B < 2; B++)
			{
				const TCHAR* Nom = Prendre(H, { TEXT("SM_tour_a"), TEXT("SM_tour_b"), TEXT("SM_tour_c"), TEXT("SM_tour_d"), TEXT("SM_beton_a"),
					TEXT("SM_beton_b"), TEXT("SM_bureaux_b"), TEXT("SM_bureaux_c") });
				PoserBatiment(Nom, Cx + (A - 0.5) * 49.0, Cy + (B - 0.5) * 49.0, H.RandRange(0, 3) * 90.0, H);
			}
		}
	}
	else if (Q == EQuartier::Mixte)
	{
		const double Case = Lot98 / 3.0;
		for (int32 A = 0; A < 3; A++)
		{
			for (int32 B = 0; B < 3; B++)
			{
				const double X = Cx + (A - 1) * Case;
				const double Y = Cy + (B - 1) * Case;
				if (A == 1 && B == 1)
				{
					if (H.FRand() < 0.45f)
					{
						for (int32 Rang = -1; Rang <= 1; Rang += 2)
						{
							for (int32 Place = -2; Place <= 2; Place++)
							{
								if (H.FRand() < 0.72f)
								{
									PoserVoitureGaree(X + Rang * 6.5, Y + Place * 3.1, Rang > 0 ? 180.0 : 0.0, H);
								}
							}
						}
					}
					else
					{
						Poser(TEXT("Sol"), TEXT("SM_Pelouse33"), EnMetres(X, Y));
						for (int32 P = 0; P < 4; P++)
						{
							PoserPalmier(X + H.FRandRange(-12.0f, 12.0f), Y + H.FRandRange(-12.0f, 12.0f), H);
						}
					}
					continue;
				}
				const TCHAR* Nom = Prendre(H, { TEXT("SM_bureaux_a"), TEXT("SM_brique_a"), TEXT("SM_brique_b"), TEXT("SM_brique_c"), TEXT("SM_residence_a"),
					TEXT("SM_residence_c"), TEXT("SM_boutiques_a"), TEXT("SM_boutiques_b"), TEXT("SM_boutiques_c"), TEXT("SM_boutiques_d") });
				PoserBatiment(Nom, X, Y, H.RandRange(0, 3) * 90.0, H);
			}
		}
	}
	else if (Q == EQuartier::Residentiel)
	{
		const double Case = Lot98 / 4.0;
		for (int32 A = 0; A < 4; A++)
		{
			for (int32 B = 0; B < 4; B++)
			{
				const double X = Cx + (A - 1.5) * Case;
				const double Y = Cy + (B - 1.5) * Case;
				Poser(TEXT("Sol"), TEXT("SM_Pelouse24"), EnMetres(X, Y));
				if ((A == 1 || A == 2) && (B == 1 || B == 2))
				{
					PoserPalmier(X + H.FRandRange(-8.0f, 8.0f), Y + H.FRandRange(-8.0f, 8.0f), H);
					continue;
				}
				// La façade du jardin regarde vers l'intérieur du bloc
				const double Lacet = (B == 0) ? 0.0 : (B == 3 ? 180.0 : (A == 0 ? 270.0 : 90.0));
				const TCHAR* Nom = Prendre(H, { TEXT("SM_maison_a"), TEXT("SM_maison_b"), TEXT("SM_maison_c"), TEXT("SM_maison_d") });
				PoserBatiment(Nom, X - 2.0, Y, Lacet, H);
				if (H.FRand() < 0.45f)
				{
					Poser(TEXT("Mobilier"), TEXT("SM_Piscine"), EnMetres(X + (A < 2 ? 6.0 : -6.0), Y + (B < 2 ? 8.0 : -8.0), NiveauTrottoir), 0.0);
				}
				PoserPalmier(X + H.FRandRange(-10.0f, 10.0f), Y + (B < 2 ? -9.5 : 9.5), H);
				Poser(TEXT("Mobilier"), TEXT("SM_Buisson"), EnMetres(X + H.FRandRange(-10.0f, 10.0f), Y + (B < 2 ? -10.5 : 10.5), NiveauTrottoir + 0.1), H.FRandRange(0.0f, 360.0f));
			}
		}
	}
	else if (Q == EQuartier::Parc)
	{
		for (int32 A = 0; A < 2; A++)
		{
			for (int32 B = 0; B < 2; B++)
			{
				const double X = Cx + (A - 0.5) * 49.0;
				const double Y = Cy + (B - 0.5) * 49.0;
				Poser(TEXT("Sol"), TEXT("SM_Pelouse49"), EnMetres(X, Y));
				for (int32 P = 0; P < 9; P++)
				{
					PoserPalmier(X + H.FRandRange(-21.0f, 21.0f), Y + H.FRandRange(-21.0f, 21.0f), H);
				}
				Poser(TEXT("Mobilier"), TEXT("SM_Banc"), EnMetres(X + (A == 0 ? 24.8 : -24.8), Y, NiveauTrottoir), A == 0 ? 180.0 : 0.0);
			}
		}
	}
	else // Port
	{
		const TCHAR* Nom = Prendre(H, { TEXT("SM_entrepot_a"), TEXT("SM_entrepot_b") });
		PoserBatiment(Nom, Cx - 23.0, Cy, 0.0, H);
		for (int32 R = 0; R < 5; R++)
		{
			for (int32 C = 0; C < 3; C++)
			{
				const int32 Pile = H.RandRange(1, 4);
				for (int32 N = 0; N < Pile; N++)
				{
					const FLinearColor A = Accents()[H.RandRange(0, Accents().Num() - 1)];
					const float D[7] = { A.R, A.G, A.B, 0.f, 0.f, 0.f, H.FRand() };
					Poser(TEXT("Mobilier"), TEXT("SM_Conteneur"), EnMetres(Cx + 30.0 + C * 3.0, Cy - 32.0 + R * 16.0, NiveauTrottoir + N * 2.6), 90.0, D);
				}
				CollisionsBoites->AddInstance(FTransform(FRotator(0.0, 90.0, 0.0), EnMetres(Cx + 30.0 + C * 3.0, Cy - 32.0 + R * 16.0, Pile * 1.3 + NiveauTrottoir),
					FVector(12.0, 2.44, Pile * 2.6)), true);
			}
		}
	}
}

void APalmeraVille::PoserMobilier(double X, double Y, double Cote, bool bNordSud, FRandomStream& H)
{
	// Lacet qui tourne le dos aux immeubles, vers la chaussée
	const double VersRoute = bNordSud ? (Cote < 0.0 ? 0.0 : 180.0) : (Cote < 0.0 ? 90.0 : 270.0);
	const double Tirage = H.FRand();
	const double Z = NiveauTrottoir;
	if (Tirage < 0.2)
	{
		Poser(TEXT("Mobilier"), TEXT("SM_Banc"), EnMetres(X, Y, Z), VersRoute + 180.0);
	}
	else if (Tirage < 0.42)
	{
		Poser(TEXT("Mobilier"), TEXT("SM_Poubelle"), EnMetres(X, Y, Z));
		PoserObstacle(X, Y, 0.3, 1.0);
	}
	else if (Tirage < 0.52)
	{
		const FLinearColor Rouge(0.7f, 0.04f, 0.03f);
		const float D[7] = { 1.f, 1.f, 1.f, Rouge.R, Rouge.G, Rouge.B, 0.f };
		Poser(TEXT("Mobilier"), TEXT("SM_Borne"), EnMetres(X, Y, Z), 0.0, D);
		PoserObstacle(X, Y, 0.14, 0.8);
	}
	else if (Tirage < 0.6)
	{
		const FLinearColor A = Accents()[H.RandRange(0, Accents().Num() - 1)];
		const float D[7] = { 1.f, 1.f, 1.f, A.R, A.G, A.B, H.FRand() };
		Poser(TEXT("Mobilier"), TEXT("SM_Abribus"), EnMetres(X, Y, Z), VersRoute + 90.0, D);
		PoserObstacle(X, Y, 0.5, 2.4);
	}
	else if (Tirage < 0.74)
	{
		Poser(TEXT("Mobilier"), TEXT("SM_Jardiniere"), EnMetres(X, Y, Z), VersRoute);
		PoserObstacle(X, Y, 0.7, 1.0);
	}
}

void APalmeraVille::ConstruireRues(){
	FRandomStream H(4242);
	// Le long de chaque tronçon de rue : lampadaires d'un côté, palmiers en vis-à-vis
	for (int32 Rue = RueMin; Rue <= RueMax; Rue++)
	{
		for (int32 T = RueMin; T < RueMax; T++)
		{
			const bool bPortNS = Quartier(FMath::Min(Rue, RueMax - 1), T) == EQuartier::Port;
			const bool bPortEO = Quartier(T, FMath::Min(Rue, RueMax - 1)) == EQuartier::Port;
			for (int32 K = 0; K < 3; K++)
			{
				const double L = (T + 0.5) * Pas + (K - 1) * 34.0;

				// Rue nord-sud (X constant)
				const double X = Rue * Pas;
				const double Cote = (K + Rue + T) % 2 == 0 ? -1.0 : 1.0;
				const double XL = X + Cote * 7.75;
				Poser(TEXT("Mobilier"), TEXT("SM_Lampadaire"), EnMetres(XL, L, NiveauTrottoir), Cote < 0.0 ? 0.0 : 180.0);
				Lampadaires.Add(EnMetres(XL - Cote * 3.0, L, 8.4));
				PoserObstacle(XL, L, 0.14, 7.0);
				if (!bPortNS && Rue > RueMin)
				{
					PoserPalmier(X - Cote * 8.1, L + 17.0, H);
				}
				if (Rue > RueMin && Rue < RueMax)
				{
					PoserMobilier(X + Cote * 10.2, L + 9.0, Cote, true, H);
				}

				// Rue est-ouest (Y constant)
				const double Y = Rue * Pas;
				const double YL = Y + Cote * 7.75;
				Poser(TEXT("Mobilier"), TEXT("SM_Lampadaire"), EnMetres(L, YL, NiveauTrottoir), Cote < 0.0 ? 90.0 : 270.0);
				Lampadaires.Add(EnMetres(L, YL - Cote * 3.0, 8.4));
				PoserObstacle(L, YL, 0.14, 7.0);
				if (!bPortEO)
				{
					PoserPalmier(L + 17.0, Y - Cote * 8.1, H);
				}
				if (Rue > RueMin && Rue < RueMax)
				{
					PoserMobilier(L + 9.0, Y + Cote * 10.2, Cote, false, H);
				}
			}
		}
	}

	// Feux aux carrefours : un mât à chaque angle, tourné vers la circulation qui arrive
	for (int32 A = RueMin; A <= RueMax; A++)
	{
		for (int32 B = RueMin; B <= RueMax; B++)
		{
			const double X = A * Pas;
			const double Y = B * Pas;
			const struct { double Dx, Dy, Lacet; float Axe; } Mats[4] = {
				{ -8.0, -8.0, 0.0, 1.f }, { 8.0, -8.0, 90.0, 0.f }, { 8.0, 8.0, 180.0, 1.f }, { -8.0, 8.0, 270.0, 0.f }
			};
			for (const auto& M : Mats)
			{
				if (FMath::Abs(X + M.Dx) > 966.0 || FMath::Abs(Y + M.Dy) > 966.0)
				{
					continue;
				}
				const float D[7] = { M.Axe, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
				Poser(TEXT("Mobilier"), TEXT("SM_Feu"), EnMetres(X + M.Dx, Y + M.Dy, NiveauTrottoir), M.Lacet, D);
				PoserObstacle(X + M.Dx, Y + M.Dy, 0.14, 6.0);
			}
		}
	}

	// Tremplins : sur la plage et sur les quais
	const struct { double X, Y, Lacet; } Tremplins[] = { { -1045.0, -280.0, 90.0 }, { -1045.0, 320.0, 270.0 }, { -300.0, BordNord - 19.0, 0.0 }, { 480.0, BordSud + 19.0, 180.0 }, { -1060.0, 40.0, 180.0 } };
	for (const auto& T : Tremplins)
	{
		const double Sol = T.X < BordOuest ? 0.12 - 0.0205 * (-975.0 - T.X) : 0.15;
		Poser(TEXT("Mobilier"), TEXT("SM_Tremplin"), EnMetres(T.X, T.Y, Sol), T.Lacet);
		const FRotator Pente(FMath::RadiansToDegrees(FMath::Atan2(1.7, 12.0)), T.Lacet, 0.0);
		BoiteCollision(EnMetres(T.X, T.Y, Sol + 0.8) - FRotator(0.0, T.Lacet, 0.0).Vector() * 0.0, EnMetres(6.05, 3.0, 0.05), Pente);
	}

	// Le nom des lieux importants, en grandes lettres au-dessus du trottoir
	for (const FLieu& Lieu : APalmeraPartie::Lieux())
	{
		UTextRenderComponent* Enseigne = NewObject<UTextRenderComponent>(this);
		Enseigne->SetMobility(EComponentMobility::Static);
		Enseigne->SetupAttachment(RootComponent);
		Enseigne->SetWorldLocationAndRotation(EnMetres(Lieu.Position.X, Lieu.Position.Y + 11.2, 9.0), FRotator(0.0, -90.0, 0.0));
		Enseigne->SetText(FText::FromString(Lieu.Nom.ToUpper()));
		Enseigne->SetTextRenderColor(Lieu.Couleur.ToFColorSRGB());
		Enseigne->SetHorizontalAlignment(EHTA_Center);
		Enseigne->SetVerticalAlignment(EVRTA_TextCenter);
		Enseigne->SetWorldSize(190.0f);
		Enseigne->SetCastShadow(true);
		Enseigne->RegisterComponent();
	}

	// Grues sur les quais du port
	for (int32 I = -2; I <= 6; I += 3)
	{
		const UStaticMeshComponent* G = Piece(TEXT("Mobilier"), TEXT("SM_Grue"), EnMetres(I * Pas, BordNord - 18.0, 0.15), 0.0);
		(void)G;
	}
}

void APalmeraVille::BeginPlay()
{
	Super::BeginPlay();

	const double Debut = FPlatformTime::Seconds();

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylindre = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	const auto Invisible = [this](UStaticMesh* Forme)
	{
		UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
		I->SetMobility(EComponentMobility::Static);
		I->SetupAttachment(RootComponent);
		I->SetStaticMesh(Forme);
		I->SetHiddenInGame(true);
		I->SetCastShadow(false);
		I->SetCollisionProfileName(TEXT("BlockAll"));
		I->SetCanEverAffectNavigation(false);
		I->RegisterComponent();
		return I;
	};
	CollisionsBoites = Invisible(Cube);
	CollisionsPoteaux = Invisible(Cylindre);

	ConstruireSol();
	for (int32 I = RueMin; I < RueMax; I++)
	{
		for (int32 J = RueMin; J < RueMax; J++)
		{
			ConstruireBloc(I, J);
		}
	}
	ConstruireRues();

	// Les vraies lumières sont réservées aux lampadaires proches ; les autres ne brillent que par leur ampoule
	for (int32 I = 0; I < 56; I++)
	{
		USpotLightComponent* L = NewObject<USpotLightComponent>(this);
		L->SetMobility(EComponentMobility::Movable);
		L->SetupAttachment(RootComponent);
		L->SetWorldRotation(FRotator(-90.0, 0.0, 0.0));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(5200.0f);
		L->SetAttenuationRadius(3200.0f);
		L->SetInnerConeAngle(38.0f);
		L->SetOuterConeAngle(68.0f);
		L->SetLightColor(FLinearColor(1.0f, 0.74f, 0.46f));
		L->SetCastShadows(true);
		L->SetVisibility(false);
		L->RegisterComponent();
		Lumieres.Add(L);
	}

	UE_LOG(LogTemp, Display, TEXT("PALMERA ville : %d batiments, %d lampadaires, %d groupes, %.1f s"),
		NbBatiments, Lampadaires.Num(), Groupes.Num(), FPlatformTime::Seconds() - Debut);
}

void APalmeraVille::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ProchaineMaj -= DeltaSeconds;
	if (ProchaineMaj <= 0.0)
	{
		ProchaineMaj = 0.5;
		ActualiserLumieres();
	}
}

void APalmeraVille::ActualiserLumieres()
{
	const APalmeraMonde* Monde = nullptr;
	for (TActorIterator<APalmeraMonde> It(GetWorld()); It; ++It)
	{
		Monde = *It;
	}
	const bool bAllumer = Monde && Monde->Nuit > 0.35;
	if (!bAllumer)
	{
		if (bLumieresAllumees)
		{
			bLumieresAllumees = false;
			for (USpotLightComponent* L : Lumieres)
			{
				L->SetVisibility(false);
			}
		}
		return;
	}
	bLumieresAllumees = true;

	const APlayerController* Joueur = GetWorld()->GetFirstPlayerController();
	if (!Joueur || !Joueur->PlayerCameraManager)
	{
		return;
	}
	const FVector Camera = Joueur->PlayerCameraManager->GetCameraLocation();
	TArray<TPair<double, int32>> Proches;
	Proches.Reserve(Lampadaires.Num());
	for (int32 I = 0; I < Lampadaires.Num(); I++)
	{
		const double D = FVector::DistSquared2D(Lampadaires[I], Camera);
		if (D < FMath::Square(24000.0))
		{
			Proches.Add({ D, I });
		}
	}
	Proches.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
	for (int32 I = 0; I < Lumieres.Num(); I++)
	{
		USpotLightComponent* L = Lumieres[I];
		if (Proches.IsValidIndex(I))
		{
			L->SetWorldLocation(Lampadaires[Proches[I].Value]);
			L->SetVisibility(true);
		}
		else
		{
			L->SetVisibility(false);
		}
	}
}
