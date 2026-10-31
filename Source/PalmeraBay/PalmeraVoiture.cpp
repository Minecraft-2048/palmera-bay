#include "PalmeraVoiture.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PalmeraConfig.h"
#include "PalmeraPartie.h"

namespace
{
	// Les forces d'Unreal sont en kg·cm/s² : 1 newton vaut 100 unités.
	constexpr double UnitesParNewton = 100.0;
	const double Rapports[] = { 3.6, 2.3, 1.65, 1.25, 1.0, 0.82 };
}

APalmeraVoiture::APalmeraVoiture()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Boite = CreateDefaultSubobject<UBoxComponent>(TEXT("Boite"));
	Boite->SetCollisionProfileName(TEXT("PhysicsActor"));
	Boite->SetNotifyRigidBodyCollision(true);
	SetRootComponent(Boite);

	Carrosserie = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Carrosserie"));
	Carrosserie->SetupAttachment(Boite);
	Carrosserie->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Vitres = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Vitres"));
	Vitres->SetupAttachment(Carrosserie);
	Vitres->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Rampe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rampe"));
	Rampe->SetupAttachment(Carrosserie);
	Rampe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rampe->SetCastShadow(false);

	for (int32 I = 0; I < 4; I++)
	{
		UStaticMeshComponent* R = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Roue%d"), I));
		R->SetupAttachment(Carrosserie);
		R->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MaillagesRoues.Add(R);
	}

	Bras = CreateDefaultSubobject<USpringArmComponent>(TEXT("Bras"));
	Bras->SetupAttachment(Boite);
	Bras->TargetArmLength = 720.0f;
	Bras->SocketOffset = FVector(0, 0, 210);
	Bras->bDoCollisionTest = true;
	Bras->ProbeSize = 18.0f;
	Bras->ProbeChannel = ECC_Visibility;
	Bras->bEnableCameraLag = true;
	Bras->CameraLagSpeed = 12.0f;
	Bras->bInheritPitch = false;
	Bras->bInheritRoll = false;
	Bras->bInheritYaw = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Bras);
	Camera->SetFieldOfView(78.0f);
	Camera->SetRelativeRotation(FRotator(-7.0, 0.0, 0.0));
	Camera->bConstrainAspectRatio = false;
	Camera->SetAutoActivate(false);

	const auto CreerPhare = [this](const TCHAR* Nom)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(Nom);
		L->SetupAttachment(Boite);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(22000.0f);
		L->SetAttenuationRadius(5500.0f);
		L->SetInnerConeAngle(13.0f);
		L->SetOuterConeAngle(30.0f);
		L->SetLightColor(FLinearColor(1.0f, 0.93f, 0.8f));
		L->SetVisibility(false);
		L->SetCastShadows(false);
		return L;
	};
	PhareGauche = CreerPhare(TEXT("PhareGauche"));
	PhareDroit = CreerPhare(TEXT("PhareDroit"));

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

FFicheVoiture APalmeraVoiture::FicheSport()
{
	FFicheVoiture F;
	F.Nom = TEXT("Coupé sport");
	F.Caisse = TEXT("/Game/Vehicles/SportsCar/SM_SportsCar.SM_SportsCar");
	F.Vitres = TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Glass.SM_SportsCar_Glass");
	F.Roue = TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Wheel.SM_SportsCar_Wheel");
	F.DemiTaille = FVector(235, 98, 51);
	F.CentreBoite = FVector(12, 0, 66);
	F.Masse = 1400.0;
	F.Puissance = 260000.0;
	F.ForceMax = 13000.0;
	F.Trainee = 0.5;
	F.Adherence = 1.6;
	F.Raideur = 42000.0;
	F.Amortissement = 5200.0;
	F.Rayon = 39.0;
	F.Empattement = 132.1;
	F.DecalageAvant = 2.9;
	F.Voie = 90.0;
	F.HauteurRoue = 37.0;
	F.IndexPeinture = 1;
	return F;
}

FFicheVoiture APalmeraVoiture::FicheCivile(int32 Modele)
{
	static const struct
	{
		const TCHAR* Maillage; const TCHAR* Roue; const TCHAR* Titre;
		double Long, Large, Haut, Bas, Avant, Arriere, Rayon, Masse, Puissance, Trainee;
	} Table[] = {
		{ TEXT("SM_Berline"), TEXT("SM_Roue"), TEXT("Berline"), 462, 182, 146, 17, 142, -138, 33, 1450, 125000, 0.62 },
		{ TEXT("SM_Citadine"), TEXT("SM_Roue"), TEXT("Citadine"), 382, 172, 153, 16, 120, -120, 31, 1100, 80000, 0.7 },
		{ TEXT("SM_Suv"), TEXT("SM_RoueGrande"), TEXT("SUV"), 475, 194, 180, 27, 145, -142, 37, 1950, 165000, 0.85 },
		{ TEXT("SM_Fourgon"), TEXT("SM_RoueGrande"), TEXT("Fourgon"), 530, 202, 234, 20, 172, -158, 36, 2400, 110000, 1.1 },
		{ TEXT("SM_Pickup"), TEXT("SM_RoueGrande"), TEXT("Pick-up"), 530, 192, 188, 30, 172, -162, 38, 2100, 170000, 0.95 },
		{ TEXT("SM_Taxi"), TEXT("SM_Roue"), TEXT("Taxi"), 462, 182, 146, 17, 142, -138, 33, 1450, 120000, 0.62 }
	};
	const auto& T = Table[FMath::Clamp(Modele, 0, 5)];
	const auto Chemin = [](const TCHAR* Nom) { return FString::Printf(TEXT("/Game/Modeles/Vehicules/%s/StaticMeshes/%s.%s"), Nom, Nom, Nom); };
	FFicheVoiture F;
	F.Nom = T.Titre;
	F.Caisse = Chemin(T.Maillage);
	F.Roue = Chemin(T.Roue);
	F.DemiTaille = FVector(T.Long * 0.5, T.Large * 0.5 - 3.0, (T.Haut - T.Bas) * 0.5);
	F.CentreBoite = FVector(0.0, 0.0, (T.Haut + T.Bas) * 0.5);
	F.Masse = T.Masse;
	F.Puissance = T.Puissance;
	F.ForceMax = T.Masse * 4.2;
	F.Trainee = T.Trainee;
	F.Adherence = 1.3;
	F.Raideur = T.Masse * 9.81 / (4.0 * 0.08);
	F.Amortissement = T.Masse * 3.6;
	F.Rayon = T.Rayon;
	F.Empattement = (T.Avant - T.Arriere) * 0.5;
	F.DecalageAvant = (T.Avant + T.Arriere) * 0.5;
	F.Voie = T.Large * 0.5 - 13.0;
	F.HauteurRoue = T.Rayon;
	F.bTraction = Modele <= 1 || Modele == 5;
	return F;
}

FFicheVoiture APalmeraVoiture::FicheToutTerrain(){
	FFicheVoiture F;
	F.Nom = TEXT("Tout-terrain");
	F.Caisse = TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Body.SM_Offroad_Body");
	F.Roue = TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Tire.SM_Offroad_Tire");
	F.DemiTaille = FVector(185, 90, 58);
	F.CentreBoite = FVector(11, 0, 100);
	F.Masse = 1900.0;
	F.Puissance = 210000.0;
	F.ForceMax = 15000.0;
	F.Trainee = 0.8;
	F.Adherence = 1.35;
	F.Raideur = 57000.0;
	F.Amortissement = 7000.0;
	F.Rayon = 51.0;
	F.Empattement = 151.7;
	F.DecalageAvant = 16.5;
	F.Voie = 124.1;
	F.VoieArriere = 139.8;
	F.HauteurRoue = 51.0;
	F.bQuatreRoues = true;
	F.IndexPeinture = 0;
	return F;
}

void APalmeraVoiture::Configurer(const FFicheVoiture& NouvelleFiche)
{
	Fiche = NouvelleFiche;

	Boite->SetBoxExtent(Fiche.DemiTaille, false);
	Carrosserie->SetRelativeLocation(FVector(0, 0, -Fiche.CentreBoite.Z) + FVector(-Fiche.CentreBoite.X, 0, 0));
	Carrosserie->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Fiche.Caisse));
	if (!Fiche.Vitres.IsEmpty())
	{
		Vitres->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Fiche.Vitres));
	}

	// Les voitures de tous les jours nomment leur peinture ; celles du moteur ont un numéro
	const int32 ParNom = Carrosserie->GetMaterialIndex(TEXT("peinture"));
	const int32 SlotPeinture = ParNom != INDEX_NONE ? ParNom : Fiche.IndexPeinture;
	if (UMaterialInterface* Mat = Carrosserie->GetMaterial(SlotPeinture))
	{
		MatCaisse = Carrosserie->CreateDynamicMaterialInstance(SlotPeinture, Mat);
		if (MatCaisse)
		{
			MatCaisse->SetVectorParameterValue(TEXT("Paint Tint"), Fiche.Peinture);
		}
	}

	if (Fiche.bPolice)
	{
		Rampe->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Rampe->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiaux/M_Gyrophare.M_Gyrophare")));
		Rampe->SetRelativeLocation(FVector(-10.0, 0.0, Fiche.CentreBoite.Z + Fiche.DemiTaille.Z + 6.0));
		Rampe->SetRelativeScale3D(FVector(0.26, 1.1, 0.1));
	}

	UStaticMesh* MaillageRoue = LoadObject<UStaticMesh>(nullptr, *Fiche.Roue);
	Roues.Reset();
	const double X = Fiche.Empattement;
	const double Y = Fiche.Voie;
	const double Y2 = Fiche.VoieArriere > 0.0 ? Fiche.VoieArriere : Y;
	const FVector Positions[4] = { FVector(X + Fiche.DecalageAvant, -Y, 0), FVector(X + Fiche.DecalageAvant, Y, 0), FVector(-X + Fiche.DecalageAvant, -Y2, 0), FVector(-X + Fiche.DecalageAvant, Y2, 0) };
	for (int32 I = 0; I < 4; I++)
	{
		FRoue R;
		R.Position = Positions[I];
		R.bAvant = I < 2;
		R.bDroite = I % 2 == 1;
		R.Maillage = MaillagesRoues[I];
		R.Maillage->SetStaticMesh(MaillageRoue);
		R.Hauteur = Fiche.HauteurRoue;
		Roues.Add(R);
	}

	PhareGauche->SetRelativeLocation(FVector(Fiche.DemiTaille.X - 5.0, -55, -10));
	PhareDroit->SetRelativeLocation(FVector(Fiche.DemiTaille.X - 5.0, 55, -10));
	PhareGauche->SetRelativeRotation(FRotator(-3.0, 0.0, 0.0));
	PhareDroit->SetRelativeRotation(FRotator(-3.0, 0.0, 0.0));
}

void APalmeraVoiture::BeginPlay()
{
	Super::BeginPlay();

	Boite->SetMassOverrideInKg(NAME_None, float(Fiche.Masse), true);
	Boite->SetLinearDamping(0.01f);
	Boite->SetAngularDamping(0.55f);
	Boite->SetCenterOfMass(FVector(0, 0, -28), NAME_None);
	Boite->BodyInstance.bUseCCD = true;
	ActiverPhysique(bPhysique);
	LacetCamera = GetActorRotation().Yaw;
}

void APalmeraVoiture::ActiverPhysique(bool bActif)
{
	bPhysique = bActif;
	Boite->SetSimulatePhysics(bActif);
	if (!bActif)
	{
		Boite->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
	else
	{
		Boite->SetCollisionProfileName(TEXT("PhysicsActor"));
	}
}

void APalmeraVoiture::Commander(double Gaz, double Commande, bool bFrein)
{
	ConsigneGaz = FMath::Clamp(Gaz, -1.0, 1.0);
	ConsigneVolant = FMath::Clamp(Commande, -1.0, 1.0);
	bConsigneFrein = bFrein;
}

void APalmeraVoiture::AnimerRoues(double Vitesse, double Dt)
{
	for (FRoue& R : Roues)
	{
		R.Rotation += Vitesse / (Fiche.Rayon / 100.0) * Dt;
		R.Hauteur = Fiche.HauteurRoue - Fiche.CentreBoite.Z;
	}
}

double APalmeraVoiture::VitesseKmh() const
{
	return FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036;
}

bool APalmeraVoiture::EstRetournee() const
{
	return GetActorUpVector().Z < 0.25;
}

void APalmeraVoiture::Redresser()
{
	const FRotator R = GetActorRotation();
	Boite->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Boite->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorLocationAndRotation(GetActorLocation() + FVector(0, 0, 120), FRotator(0.0, R.Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
}

void APalmeraVoiture::AllumerPhares(bool bAllume)
{
	bPhares = bAllume;
	PhareGauche->SetVisibility(bAllume);
	PhareDroit->SetVisibility(bAllume);
	bLueurs = !bAllume;
	AllumerLueurs(bAllume);
}

void APalmeraVoiture::AllumerLueurs(bool bAllume)
{
	if (bLueurs != bAllume)
	{
		bLueurs = bAllume;
		Carrosserie->SetScalarParameterValueOnMaterials(TEXT("EmissivePower"), bAllume ? 60.0f : 0.0f);
	}
}

void APalmeraVoiture::AllumerStop(bool bAllume)
{
	if (bStop != bAllume)
	{
		bStop = bAllume;
		Carrosserie->SetScalarParameterValueOnMaterials(TEXT("Frein"), bAllume ? 1.0f : 0.0f);
	}
}

void APalmeraVoiture::ActiverCamera(bool bActif)
{
	Camera->SetActive(bActif);
	LacetCamera = GetActorRotation().Yaw;
	OrbiteLacet = 0.0;
	OrbiteTangage = 0.0;
}

void APalmeraVoiture::TournerCamera(double Lacet, double Tangage)
{
	if (FMath::Abs(Lacet) + FMath::Abs(Tangage) > 0.01)
	{
		bRecentrer = false;
		OrbiteLacet = FRotator::NormalizeAxis(OrbiteLacet + Lacet);
		OrbiteTangage = FMath::Clamp(OrbiteTangage + Tangage, -45.0, 40.0);
	}
}

void APalmeraVoiture::Abimer(double Points)
{
	Vie = FMath::Max(Vie - Points * FacteurDegats, 0.0);
}

void APalmeraVoiture::Reparer()
{
	Vie = VieMax;
}
void APalmeraVoiture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Dt = FMath::Min(double(DeltaSeconds), 0.05);
	Choc = FMath::Max(Choc - Dt * 12.0, 0.0);
	if (bPhysique)
	{
		SimulerRoues(Dt);
	}
	else
	{
		for (FRoue& R : Roues)
		{
			R.bContact = true;
			R.Compression = 0.075;
		}
	}
	ActualiserVisuel(Dt);
	if (bCibleJoueur)
	{
		ActualiserCamera(Dt);
	}
}

void APalmeraVoiture::NotifyHit(UPrimitiveComponent* MaPiece, AActor* Autre, UPrimitiveComponent* AutrePiece, bool bMoiBouge, FVector Lieu, FVector Normale, FVector Impulsion, const FHitResult& Touche)
{
	Super::NotifyHit(MaPiece, Autre, AutrePiece, bMoiBouge, Lieu, Normale, Impulsion, Touche);

	const double DeltaV = Impulsion.Size() / FMath::Max(Fiche.Masse, 1.0) / 100.0;
	Choc = FMath::Max(Choc, DeltaV);
	if (bPhysique && DeltaV > 2.5)
	{
		Abimer(DeltaV * 2.2);
	}
	if (bCibleJoueur && DeltaV > 3.0)
	{
		if (const APalmeraVoiture* Heurtee = Cast<APalmeraVoiture>(Autre))
		{
			if (APalmeraPartie* Partie = APalmeraPartie::Lire(this))
			{
				if (!Heurtee->bEnnemie)
				{
					Partie->Faute(Heurtee->LireFiche().bPolice ? 22.0 : 5.0);
				}
			}
		}
	}
}

double APalmeraVoiture::BraquageMax() const
{
	const double V = GetVelocity().Size() / 100.0;
	const double LongueurAxes = Fiche.Empattement * 2.0 / 100.0;
	const double AngleTenu = FMath::RadiansToDegrees(FMath::Atan(LongueurAxes * Fiche.Adherence * 9.81 * 0.95 / FMath::Max(V * V, 1.0))) + 1.2;
	return FMath::Min(36.0, AngleTenu);
}

void APalmeraVoiture::SimulerRoues(double Dt)
{
	const FTransform T = Boite->GetComponentTransform();
	const FVector Avant = T.GetUnitAxis(EAxis::X);
	const FVector Droite = T.GetUnitAxis(EAxis::Y);
	const FVector Haut = T.GetUnitAxis(EAxis::Z);
	const FVector Vitesse = Boite->GetPhysicsLinearVelocity() / 100.0;
	const double VLong = FVector::DotProduct(Vitesse, Avant);
	const double VAbs = Vitesse.Size();
	const double Masse = Fiche.Masse;

	// Le volant ne braque pas plus que ce que les pneus peuvent tenir à cette vitesse : seul le frein à main fait décrocher
	Volant = FMath::FInterpTo(Volant, ConsigneVolant, Dt, ConsigneVolant == 0.0 ? 9.0 : 6.0);
	const double AngleMax = bConsigneFrein ? FMath::Lerp(36.0, 6.5, FMath::Clamp(VAbs / 62.0, 0.0, 1.0)) : BraquageMax();
	const double Angle = FMath::DegreesToRadians(Volant * AngleMax);

	// Moteur : force plafonnée à bas régime, puissance constante ensuite
	const double VPourForce = FMath::Max(FMath::Abs(VLong), 2.0);
	const double ForceMoteur = FMath::Min(Fiche.ForceMax, Fiche.Puissance / VPourForce);
	double Traction = 0.0;
	double Freinage = 0.0;
	if (ConsigneGaz > 0.02)
	{
		if (VLong < -1.0)
		{
			Freinage = ConsigneGaz;
		}
		else
		{
			Traction = ConsigneGaz * ForceMoteur;
		}
	}
	else if (ConsigneGaz < -0.02)
	{
		if (VLong > 1.0)
		{
			Freinage = -ConsigneGaz;
		}
		else
		{
			Traction = ConsigneGaz * ForceMoteur * 0.45;
			if (VLong < -17.0)
			{
				Traction = 0.0;
			}
		}
	}

	if ((!bCibleJoueur && !Fiche.bPolice && !bEnnemie) || EstHorsService())
	{
		Traction = 0.0;
		Freinage = FMath::Max(Freinage, EstHorsService() ? 0.35 : 1.0);
	}

	AllumerStop(Freinage > 0.05 || bConsigneFrein);

	const int32 RouesMotrices = Fiche.bQuatreRoues ? 4 : 2;
	int32 AuSol = 0;
	GlisseMax = 0.0;

	for (FRoue& R : Roues)
	{
		const double Repos = 32.0;
		const FVector Attache = T.TransformPosition(FVector(R.Position.X - Fiche.CentreBoite.X, R.Position.Y, Fiche.HauteurRoue - Fiche.CentreBoite.Z + Repos - 8.0));
		const FVector Fin = Attache - Haut * (Repos + Fiche.Rayon);

		FHitResult Touche;
		FCollisionQueryParams P(SCENE_QUERY_STAT(RoueVoiture), false, this);
		R.bContact = GetWorld()->LineTraceSingleByChannel(Touche, Attache, Fin, ECC_Visibility, P);
		R.CompressionAvant = R.Compression;

		if (!R.bContact)
		{
			R.Compression = 0.0;
			R.Charge = 0.0;
			R.Hauteur = Fiche.HauteurRoue - Fiche.CentreBoite.Z - 8.0;
			continue;
		}
		AuSol++;
		R.Compression = (Repos + Fiche.Rayon - Touche.Distance) / 100.0;
		R.Sol = Touche.ImpactNormal;
		R.Hauteur = Fiche.HauteurRoue - Fiche.CentreBoite.Z + Repos - 8.0 - (Touche.Distance - Fiche.Rayon);

		const double VitesseCompression = (R.Compression - R.CompressionAvant) / FMath::Max(Dt, 0.001);
		const double FRessort = Fiche.Raideur * R.Compression + Fiche.Amortissement * FMath::Clamp(VitesseCompression, -3.0, 3.0);
		R.Charge = FMath::Clamp(FRessort, 0.0, Masse * 9.81 * 2.2);
		Boite->AddForceAtLocation(Haut * R.Charge * UnitesParNewton, Attache);

		// Repère de la roue, posé sur le sol
		const double AngleRoue = R.bAvant ? Angle : 0.0;
		const FVector AvantRoue0 = Avant * FMath::Cos(AngleRoue) + Droite * FMath::Sin(AngleRoue);
		const FVector AvantRoue = (AvantRoue0 - R.Sol * FVector::DotProduct(AvantRoue0, R.Sol)).GetSafeNormal();
		const FVector DroiteRoue = FVector::CrossProduct(R.Sol, AvantRoue).GetSafeNormal();

		const FVector Contact = Touche.ImpactPoint;
		const FVector VPoint = Boite->GetPhysicsLinearVelocityAtPoint(Contact) / 100.0;
		const double VRoue = FVector::DotProduct(VPoint, AvantRoue);
		const double VGlisse = FVector::DotProduct(VPoint, DroiteRoue);
		GlisseMax = FMath::Max(GlisseMax, FMath::Abs(VGlisse));

		const bool bSable = Contact.X < -97500.0;
		double Mu = Fiche.Adherence * (bSable ? 0.55 : 1.0);
		if (bConsigneFrein && !R.bAvant)
		{
			Mu *= 0.42;
		}
		const double FMax = Mu * R.Charge;

		// Force longitudinale : moteur et freins
		const bool bMotrice = Fiche.bQuatreRoues || (Fiche.bTraction == R.bAvant);
		double FLong = bMotrice ? Traction / RouesMotrices : 0.0;
		const double ForceFrein = (Freinage * 15000.0 + (bConsigneFrein && !R.bAvant ? 7000.0 : 0.0)) * (R.bAvant ? 0.3 : 0.2);
		FLong -= FMath::Clamp(VRoue * 4000.0, -ForceFrein, ForceFrein);
		if (FMath::Abs(ConsigneGaz) < 0.02)
		{
			FLong -= FMath::Clamp(VRoue * 300.0, -320.0, 320.0);	// frein moteur
		}

		// Force latérale : ramène la vitesse de glisse vers zéro, limitée par l'adhérence
		const double FLat = -FMath::Clamp(VGlisse * Masse * 0.25 / 0.055, -FMax, FMax);

		// Tenir la route passe avant : le moteur et les freins ne prennent que l'adhérence qui reste
		const double Reste = FMath::Sqrt(FMath::Max(FMax * FMax - FLat * FLat, FMath::Square(FMax * 0.3)));
		FLong = FMath::Clamp(FLong, -Reste, Reste);

		// Le point d'application est relevé pour limiter le roulis
		const FVector Point = Contact + Haut * 34.0;
		Boite->AddForceAtLocation((AvantRoue * FLong + DroiteRoue * FLat) * UnitesParNewton, Point);

		R.Rotation += VRoue / (Fiche.Rayon / 100.0) * Dt;
	}

	// Barres antiroulis
	for (int32 Essieu = 0; Essieu < 2; Essieu++)
	{
		const FRoue& G = Roues[Essieu * 2];
		const FRoue& D = Roues[Essieu * 2 + 1];
		if (G.bContact && D.bContact)
		{
			const double Ecart = (G.Compression - D.Compression) * 22000.0;
			const FVector Decalage(Fiche.CentreBoite.X, 0.0, 0.0);
			Boite->AddForceAtLocation(Haut * Ecart * UnitesParNewton, T.TransformPosition(G.Position - Decalage));
			Boite->AddForceAtLocation(-Haut * Ecart * UnitesParNewton, T.TransformPosition(D.Position - Decalage));
		}
	}

	// Traînée et appui aérodynamique
	Boite->AddForce(-Vitesse * VAbs * Fiche.Trainee * UnitesParNewton);
	if (AuSol > 0)
	{
		Boite->AddForce(-Haut * VAbs * VAbs * 1.5 * UnitesParNewton);
		TempsAuSol = 0.0;
	}
	else
	{
		// En l'air, la voiture se remet doucement à plat
		TempsAuSol += Dt;
		const FVector Couple = FVector::CrossProduct(Haut, FVector::UpVector) * 900000.0;
		Boite->AddTorqueInRadians(Couple * UnitesParNewton * 0.01);
	}

	// Compte-tours et boîte automatique, pour le son et l'interface
	const double RapportCourant = Rapports[RapportActuel - 1];
	const double Cible = 900.0 + FMath::Abs(VLong) / (Fiche.Rayon / 100.0) * RapportCourant * 3.42 * 9.55;
	Tours = FMath::FInterpTo(Tours, FMath::Clamp(Cible, 900.0, 7400.0), Dt, 10.0);
	if (Tours > 6600.0 && RapportActuel < 6)
	{
		RapportActuel++;
	}
	else if (Tours < 2600.0 && RapportActuel > 1)
	{
		RapportActuel--;
	}
}

void APalmeraVoiture::ActualiserVisuel(double Dt)
{
	for (FRoue& R : Roues)
	{
		if (!R.Maillage)
		{
			continue;
		}
		const double Yaw = R.bAvant ? Volant * 30.0 : 0.0;
		const double Roll = R.bDroite ? 0.0 : 180.0;
		const FVector Local(R.Position.X, R.Position.Y, R.bContact ? R.Hauteur + Fiche.CentreBoite.Z : Fiche.HauteurRoue - 8.0);
		R.Maillage->SetRelativeLocationAndRotation(Local,
			FRotator(R.bDroite ? -FMath::RadiansToDegrees(R.Rotation) : FMath::RadiansToDegrees(R.Rotation), Yaw + (R.bDroite ? 0.0 : 180.0), 0.0));
		(void)Roll;
	}
}

void APalmeraVoiture::ActualiserCamera(double Dt)
{
	const double Cap = GetVelocity().Size() > 300.0 && FVector::DotProduct(GetVelocity(), GetActorForwardVector()) < -100.0
		? GetActorRotation().Yaw + 180.0 : GetActorRotation().Yaw;
	const double Delta = FMath::FindDeltaAngleDegrees(LacetCamera, Cap);
	LacetCamera += Delta * FMath::Min(Dt * 4.0, 1.0);

	// Regard libre à la souris sur 360° : la caméra reste où le joueur l'a mise, sauf s'il demande de la replacer
	if (bRecentrer)
	{
		OrbiteLacet = FMath::FInterpTo(OrbiteLacet, 0.0, Dt, 7.0);
		OrbiteTangage = FMath::FInterpTo(OrbiteTangage, 0.0, Dt, 7.0);
		bRecentrer = FMath::Abs(OrbiteLacet) + FMath::Abs(OrbiteTangage) > 0.5;
	}
	Bras->SetWorldRotation(FRotator(-7.0 + OrbiteTangage, LacetCamera + OrbiteLacet, 0.0));
	Camera->SetFieldOfView(float(78.0 + FMath::Clamp(GetVelocity().Size() / 100.0 / 60.0, 0.0, 1.0) * 18.0));
}
