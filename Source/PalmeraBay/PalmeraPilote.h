#pragma once

#include "CoreMinimal.h"

class AActor;
class APalmeraVoiture;

// Mémoire d'un conducteur automatique : blocages, marche arrière, ce qu'il voit de sa cible.
struct FPiloteAuto
{
	double Bloquee = 0.0;
	double MarcheArriere = 0.0;
	bool bVoitCible = false;
	double Distance = 1e9;
};

namespace Palmera
{
	// Conduit une voiture vers une cible : par les rues tant qu'elle ne la voit pas, tout droit ensuite.
	// DistanceArret : à quelle distance de la cible elle lève le pied (0 pour lui foncer dessus).
	void PiloterVers(APalmeraVoiture* Voiture, const FVector& Cible, const AActor* ActeurCible, FPiloteAuto& Pilote, double Dt, double DistanceArret);
}
