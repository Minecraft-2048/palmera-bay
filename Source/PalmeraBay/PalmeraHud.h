#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class APalmeraJoueur;

// Toute l'interface est dessinée ici : mini-carte, compteur, argent, recherche, objectifs, carte et menu.
class SPalmeraHud : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPalmeraHud) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APalmeraJoueur>, Joueur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1920, 1080); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometrie, const FSlateRect& Rect, FSlateWindowElementList& Liste,
		int32 Couche, const FWidgetStyle& Style, bool bActif) const override;

	bool bCarteOuverte = false;
	bool bMenuOuvert = false;
	bool bAideOuverte = true;
	int32 ChoixMenu = 0;
	int32 Qualite = 2;

	static int32 NombreChoix() { return 6; }
	bool bJournalOuvert = false;

private:
	void Texte(FSlateWindowElementList& L, int32 C, const FGeometry& G, const FString& T, FVector2D P, int32 Taille, const FLinearColor& Couleur, double Ancrage = 0.0, bool bGras = true) const;
	void Boite(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D P, FVector2D Taille, const FLinearColor& Couleur, float Rayon = 10.f) const;
	void Plan(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Haut, FVector2D Taille, FVector2D Centre, double Echelle, double Cap, bool bComplet) const;
	void Etoile(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D P, double Rayon, const FLinearColor& Couleur) const;
	void Menu(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const;
	void Aide(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const;
	void Recit(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const;
	void Journal(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const;

	TWeakObjectPtr<APalmeraJoueur> Joueur;
};
