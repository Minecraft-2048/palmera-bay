#pragma once

#include "CoreMinimal.h"

// Dimensions de la ville, en mètres. X va vers l'est, Y vers le sud, Z vers le haut.
// L'océan est à l'ouest : le soleil s'y couche.
namespace Palmera
{
	inline constexpr double Metre = 100.0;

	inline constexpr double Pas = 120.0;		// distance entre deux axes de rue
	inline constexpr int32 RueMin = -8;
	inline constexpr int32 RueMax = 8;
	inline constexpr double DemiRoute = 7.0;	// du milieu de la rue à la bordure
	inline constexpr double FinTrottoir = 11.0;	// du milieu de la rue au fond du trottoir
	inline constexpr double NiveauTrottoir = 0.15;
	inline constexpr double NiveauMer = -0.6;

	inline constexpr double BordOuest = RueMin * Pas;
	inline constexpr double BordEst = RueMax * Pas;
	inline constexpr double BordNord = RueMin * Pas;
	inline constexpr double BordSud = RueMax * Pas;

	inline FVector EnMetres(double X, double Y, double Z = 0.0)
	{
		return FVector(X, Y, Z) * Metre;
	}
}
