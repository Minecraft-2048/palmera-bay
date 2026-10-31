#include "PalmeraPilote.h"

#include "Engine/World.h"
#include "PalmeraConfig.h"
#include "PalmeraVoiture.h"

void Palmera::PiloterVers(APalmeraVoiture* V, const FVector& Cible, const AActor* ActeurCible, FPiloteAuto& E, double Dt, double DistanceArret)
{
	const FVector Pos = V->GetActorLocation() / Metre;
	const FVector CibleM = Cible / Metre;

	FHitResult Touche;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(VuePilote), false, V);
	if (ActeurCible)
	{
		Q.AddIgnoredActor(ActeurCible);
	}
	E.bVoitCible = !V->GetWorld()->LineTraceSingleByChannel(Touche, V->GetActorLocation() + FVector(0, 0, 60), Cible + FVector(0, 0, 60), ECC_Visibility, Q);
	E.Distance = FVector::Dist2D(Pos, CibleM);

	FVector2D But(CibleM.X, CibleM.Y);
	if (!E.bVoitCible || E.Distance > 180.0)
	{
		// Suit la rue : avance vers le carrefour le plus utile, puis change d'axe
		const int32 Kx = FMath::RoundToInt(Pos.X / Pas);
		const int32 Ky = FMath::RoundToInt(Pos.Y / Pas);
		const bool bSurNS = FMath::Abs(Pos.X - Kx * Pas) < 8.0;
		const bool bSurEO = FMath::Abs(Pos.Y - Ky * Pas) < 8.0;
		const double Ex = CibleM.X - Pos.X;
		const double Ey = CibleM.Y - Pos.Y;
		if (bSurNS && (!bSurEO || FMath::Abs(Ey) >= FMath::Abs(Ex)))
		{
			But = FVector2D(Kx * Pas, Pos.Y + FMath::Sign(Ey) * 40.0);
		}
		else if (bSurEO)
		{
			But = FVector2D(Pos.X + FMath::Sign(Ex) * 40.0, Ky * Pas);
		}
		else
		{
			But = FVector2D(Kx * Pas, Ky * Pas);
		}
	}

	const double Cap = FMath::RadiansToDegrees(FMath::Atan2(But.Y - Pos.Y, But.X - Pos.X));
	const double Ecart = FMath::FindDeltaAngleDegrees(V->GetActorRotation().Yaw, Cap);
	const double Vitesse = V->GetVelocity().Size() / Metre;

	if (E.MarcheArriere > 0.0)
	{
		E.MarcheArriere -= Dt;
		V->Commander(-1.0, -FMath::Sign(Ecart), false);
		return;
	}
	E.Bloquee = Vitesse < 1.5 ? E.Bloquee + Dt : 0.0;
	if (E.Bloquee > 1.6)
	{
		E.Bloquee = 0.0;
		E.MarcheArriere = 1.4;
	}

	const double Virage = FMath::Abs(Ecart);
	const double Gaz = (Virage > 55.0 && Vitesse > 14.0) ? -1.0 : (E.Distance < DistanceArret ? 0.0 : 1.0);
	V->Commander(Gaz, FMath::Clamp(Ecart / 26.0, -1.0, 1.0), Virage > 75.0 && Vitesse > 9.0);
}
