#include "PalmeraHud.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "PalmeraConfig.h"
#include "PalmeraCourse.h"
#include "PalmeraHistoire.h"
#include "PalmeraJoueur.h"
#include "PalmeraLieux.h"
#include "PalmeraMonde.h"
#include "PalmeraPartie.h"
#include "PalmeraPolice.h"
#include "PalmeraVille.h"
#include "PalmeraVoiture.h"
#include "EngineUtils.h"
#include "Engine/World.h"

using namespace Palmera;

namespace
{
	const FLinearColor Blanc(1.f, 1.f, 1.f, 1.f);
	const FLinearColor Sombre(0.01f, 0.012f, 0.02f, 0.62f);
	const FLinearColor Corail(1.f, 0.36f, 0.3f, 1.f);
	const FLinearColor Or(1.f, 0.72f, 0.2f, 1.f);
	const FLinearColor Turquoise(0.2f, 0.85f, 0.8f, 1.f);

	FString Espacer(int32 N)
	{
		FString S = FString::FromInt(FMath::Abs(N));
		for (int32 I = S.Len() - 3; I > 0; I -= 3)
		{
			S.InsertAt(I, TEXT(" "));
		}
		return (N < 0 ? TEXT("-") : TEXT("")) + S;
	}
}

void SPalmeraHud::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;
	SetVisibility(EVisibility::HitTestInvisible);
}

void SPalmeraHud::Texte(FSlateWindowElementList& L, int32 C, const FGeometry& G, const FString& T, FVector2D P, int32 Taille, const FLinearColor& Couleur, double Ancrage, bool bGras) const
{
	const FSlateFontInfo Police = FCoreStyle::GetDefaultFontStyle(bGras ? "Bold" : "Regular", Taille);
	const FVector2D Mesure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T, Police);
	const FVector2D Origine(P.X - Mesure.X * Ancrage, P.Y);
	// Une ombre légère garde le texte lisible sur le ciel comme sur les façades
	FSlateDrawElement::MakeText(L, C, G.ToPaintGeometry(FVector2f(Mesure), FSlateLayoutTransform(FVector2f(Origine + FVector2D(1.5, 1.5)))), T, Police, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.7f));
	FSlateDrawElement::MakeText(L, C + 1, G.ToPaintGeometry(FVector2f(Mesure), FSlateLayoutTransform(FVector2f(Origine))), T, Police, ESlateDrawEffect::None, Couleur);
}

void SPalmeraHud::Boite(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D P, FVector2D Taille, const FLinearColor& Couleur, float Rayon) const
{
	const FSlateRoundedBoxBrush Pinceau(FLinearColor::White, Rayon);
	FSlateDrawElement::MakeBox(L, C, G.ToPaintGeometry(FVector2f(Taille), FSlateLayoutTransform(FVector2f(P))), &Pinceau, ESlateDrawEffect::None, Couleur);
}

void SPalmeraHud::Etoile(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D P, double Rayon, const FLinearColor& Couleur) const
{
	TArray<FVector2f> Points;
	for (int32 I = 0; I <= 10; I++)
	{
		const double A = I * UE_DOUBLE_PI / 5.0 - UE_DOUBLE_PI / 2.0;
		const double R = I % 2 == 0 ? Rayon : Rayon * 0.42;
		Points.Add(FVector2f(float(P.X + FMath::Cos(A) * R), float(P.Y + FMath::Sin(A) * R)));
	}
	FSlateDrawElement::MakeLines(L, C, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Couleur, true, 2.0f);
	for (int32 I = 0; I < 10; I += 2)
	{
		TArray<FVector2f> Trait = { FVector2f(P), Points[I] };
		FSlateDrawElement::MakeLines(L, C, G.ToPaintGeometry(), Trait, ESlateDrawEffect::None, Couleur, true, float(Rayon * 0.7));
	}
}

void SPalmeraHud::Plan(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Haut, FVector2D Taille, FVector2D Centre, double Echelle, double Cap, bool bComplet) const
{
	const double Co = FMath::Cos(FMath::DegreesToRadians(Cap));
	const double Si = FMath::Sin(FMath::DegreesToRadians(Cap));
	const FVector2D Milieu = Haut + Taille * 0.5;
	const auto Ecran = [&](double X, double Y)
	{
		const double Dx = X - Centre.X;
		const double Dy = Y - Centre.Y;
		const double Av = Dx * Co + Dy * Si;
		const double Dr = -Dx * Si + Dy * Co;
		return FVector2f(float(Milieu.X + Dr * Echelle), float(Milieu.Y - Av * Echelle));
	};
	const auto Rectangle = [&](double X0, double Y0, double X1, double Y1, const FLinearColor& Coul, int32 Niveau)
	{
		const double Ym = (Y0 + Y1) * 0.5;
		TArray<FVector2f> P = { Ecran(X0, Ym), Ecran(X1, Ym) };
		FSlateDrawElement::MakeLines(L, Niveau, G.ToPaintGeometry(), P, ESlateDrawEffect::None, Coul, false, float((Y1 - Y0) * Echelle));
	};
	const auto Pastille = [&](const FVector2D& Pos, double Rayon, const FLinearColor& Coul, int32 Niveau)
	{
		FVector2f E = Ecran(Pos.X, Pos.Y);
		FVector2D D(E);
		if (!bComplet)
		{
			// Hors de la mini-carte : la pastille reste collée au bord, dans la bonne direction
			D.X = FMath::Clamp(D.X, Haut.X + 8.0, Haut.X + Taille.X - 8.0);
			D.Y = FMath::Clamp(D.Y, Haut.Y + 8.0, Haut.Y + Taille.Y - 8.0);
		}
		Boite(L, Niveau, G, D - FVector2D(Rayon, Rayon), FVector2D(Rayon, Rayon) * 2.0, Coul, float(Rayon));
	};

	L.PushClip(FSlateClippingZone(G.ToPaintGeometry(FVector2f(Taille), FSlateLayoutTransform(FVector2f(Haut)))));

	Boite(L, C, G, Haut, Taille, FLinearColor(0.02f, 0.11f, 0.17f, bComplet ? 0.97f : 0.78f), 0.f);
	Rectangle(BordOuest - 13, BordNord - 28, BordEst + 28, BordSud + 28, FLinearColor(0.12f, 0.13f, 0.14f, 1.f), C + 1);
	Rectangle(BordOuest - 150, BordNord - 28, BordOuest - 13, BordSud + 28, FLinearColor(0.56f, 0.48f, 0.3f, 1.f), C + 1);
	Rectangle(BordEst + 30, BordNord - 400, BordEst + 1500, BordSud + 400, FLinearColor(0.09f, 0.14f, 0.07f, 1.f), C + 1);

	for (int32 I = RueMin; I < RueMax; I++)
	{
		for (int32 J = RueMin; J < RueMax; J++)
		{
			const double X = I * Pas + 7.0;
			const double Y = J * Pas + 7.0;
			FLinearColor Coul(0.2f, 0.21f, 0.23f, 1.f);
			switch (APalmeraVille::Quartier(I, J))
			{
			case EQuartier::Plage: Coul = FLinearColor(0.34f, 0.25f, 0.24f, 1.f); break;
			case EQuartier::Parc: Coul = FLinearColor(0.1f, 0.3f, 0.12f, 1.f); break;
			case EQuartier::Port: Coul = FLinearColor(0.2f, 0.2f, 0.26f, 1.f); break;
			case EQuartier::Centre: Coul = FLinearColor(0.3f, 0.32f, 0.38f, 1.f); break;
			case EQuartier::Residentiel: Coul = FLinearColor(0.15f, 0.22f, 0.13f, 1.f); break;
			default: break;
			}
			Rectangle(X, Y, X + Pas - 14.0, Y + Pas - 14.0, Coul, C + 2);
		}
	}

	APalmeraPartie* Partie = Joueur.IsValid() ? APalmeraPartie::Lire(Joueur.Get()) : nullptr;
	if (Partie)
	{
		for (const FLieu& Lieu : APalmeraPartie::Lieux())
		{
			Pastille(Lieu.Position, bComplet ? 7.0 : 4.0, Lieu.Couleur, C + 4);
			if (bComplet)
			{
				const FVector2f E = Ecran(Lieu.Position.X, Lieu.Position.Y);
				Texte(L, C + 5, G, Lieu.Nom, FVector2D(E.X + 12.0, E.Y - 9.0), 11, Blanc);
			}
		}
		for (int32 I = 0; I < Partie->DepartsMissions.Num(); I++)
		{
			if (!Partie->bEnMission)
			{
				Pastille(Partie->DepartsMissions[I], bComplet ? 9.0 : 5.0, Or, C + 5);
				if (bComplet)
				{
					const FVector2f E = Ecran(Partie->DepartsMissions[I].X, Partie->DepartsMissions[I].Y);
					Texte(L, C + 6, G, Partie->NomsMissions[I], FVector2D(E.X + 13.0, E.Y - 10.0), 12, Or);
				}
			}
		}
		if (bComplet)
		{
			for (const FVector2D& Cassette : Partie->Cassettes)
			{
				Pastille(Cassette, 3.5, Turquoise, C + 4);
			}
		}
		if (Partie->bCibleActive)
		{
			Pastille(Partie->Cible, bComplet ? 10.0 : 7.0, Or, C + 7);
		}
		for (TActorIterator<APalmeraVoiture> It(Joueur->GetWorld()); It; ++It)
		{
			if (It->bEnnemie && !It->EstHorsService())
			{
				const FVector P = It->GetActorLocation() / Metre;
				Pastille(FVector2D(P.X, P.Y), 5.0, FLinearColor(0.95f, 0.15f, 0.2f, 1), C + 7);
			}
		}
		for (TActorIterator<APalmeraPolice> It(Joueur->GetWorld()); It; ++It)
		{
			const bool bRouge = FMath::Fmod(FPlatformTime::Seconds() * 3.0, 1.0) < 0.5;
			for (const APalmeraVoiture* V : It->Patrouilles)
			{
				if (V && !V->IsHidden())
				{
					const FVector P = V->GetActorLocation() / Metre;
					Pastille(FVector2D(P.X, P.Y), 5.0, bRouge ? FLinearColor(1, 0.1f, 0.1f, 1) : FLinearColor(0.15f, 0.35f, 1, 1), C + 7);
				}
			}
		}
	}

	if (const APalmeraCourse* Course = Joueur.IsValid() ? APalmeraCourse::Lire(Joueur.Get()) : nullptr)
	{
		const TArray<FVector2D>& Trace = Course->LireTrace();
		TArray<FVector2f> Ligne;
		for (int32 I = 0; I < Trace.Num(); I += 6)
		{
			Ligne.Add(Ecran(Trace[I].X, Trace[I].Y));
		}
		if (Ligne.Num() > 1)
		{
			const FVector2f Premier = Ligne[0];
			Ligne.Add(Premier);
			FSlateDrawElement::MakeLines(L, C + 6, G.ToPaintGeometry(), Ligne, ESlateDrawEffect::None, Or, true, 3.0f);
		}
		for (const FConcurrent* Rival : Course->Classement())
		{
			if (Rival != Course->Joueur() && Rival->Voiture)
			{
				const FVector P = Rival->Voiture->GetActorLocation() / Metre;
				Pastille(FVector2D(P.X, P.Y), 5.0, FLinearColor(Rival->Couleur.R, Rival->Couleur.G, Rival->Couleur.B, 1.f), C + 7);
			}
		}
	}

	L.PopClip();

	// Flèche du joueur
	const APawn* Moi = Joueur.IsValid() ? Joueur->GetPawn() : nullptr;
	if (Moi)
	{
		const FVector P = Moi->GetActorLocation() / Metre;
		const FVector2f E = Ecran(P.X, P.Y);
		const double Dir = FMath::DegreesToRadians(Moi->GetActorRotation().Yaw - Cap);
		const auto Sommet = [&](double Angle, double R) { return FVector2f(E.X + float(FMath::Sin(Dir + Angle) * R), E.Y - float(FMath::Cos(Dir + Angle) * R)); };
		TArray<FVector2f> F = { Sommet(0.0, 11.0), Sommet(2.45, 9.0), Sommet(0.0, -3.5), Sommet(-2.45, 9.0), Sommet(0.0, 11.0) };
		FSlateDrawElement::MakeLines(L, C + 9, G.ToPaintGeometry(), F, ESlateDrawEffect::None, Blanc, true, 3.0f);
	}
}

void SPalmeraHud::Menu(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const
{
	Boite(L, C, G, FVector2D::ZeroVector, Ecran, FLinearColor(0.0f, 0.0f, 0.02f, 0.6f), 0.f);
	const FVector2D Taille(520, 490);
	const FVector2D P = (Ecran - Taille) * 0.5;
	Boite(L, C + 1, G, P, Taille, FLinearColor(0.02f, 0.03f, 0.05f, 0.94f), 16.f);
	const bool bCourse = Joueur.IsValid() && APalmeraCourse::Lire(Joueur.Get()) != nullptr;
	Texte(L, C + 2, G, bCourse ? TEXT("PALMERA RACING") : TEXT("PALMERA BAY"), FVector2D(Ecran.X * 0.5, P.Y + 26), 30, Corail, 0.5);
	static const TCHAR* Noms[] = { TEXT("Bas"), TEXT("Moyen"), TEXT("Élevé"), TEXT("Épique") };
	TArray<FString> Choix = { TEXT("Reprendre"), FString::Printf(TEXT("Graphismes : %s"), Noms[FMath::Clamp(Qualite, 0, 3)]) };
	if (!bCourse)
	{
		Choix.Append({ TEXT("Journal de l'histoire"), TEXT("Carte de la ville"), TEXT("Sauvegarder") });
	}
	Choix.Add(TEXT("Quitter le jeu"));
	for (int32 I = 0; I < Choix.Num(); I++)
	{
		const double Y = P.Y + 100 + I * 54;
		if (I == ChoixMenu)
		{
			Boite(L, C + 2, G, FVector2D(P.X + 40, Y - 6), FVector2D(Taille.X - 80, 52), Corail * FLinearColor(1, 1, 1, 0.92f), 10.f);
		}
		Texte(L, C + 4, G, Choix[I], FVector2D(Ecran.X * 0.5, Y + 4), 18, I == ChoixMenu ? FLinearColor(0.02f, 0.02f, 0.04f, 1.f) : Blanc, 0.5);
	}
	Texte(L, C + 3, G, TEXT("Flèches ou ZS : choisir  ·  Entrée : valider  ·  Échap : reprendre"), FVector2D(Ecran.X * 0.5, P.Y + Taille.Y - 34), 11, FLinearColor(0.7f, 0.72f, 0.76f, 1.f), 0.5, false);
}

namespace
{
	// Coupe un texte en lignes d'au plus Max caractères, sans casser les mots.
	TArray<FString> Couper(const FString& Texte, int32 Max)
	{
		TArray<FString> Mots, Sortie;
		Texte.ParseIntoArray(Mots, TEXT(" "));
		FString Ligne;
		for (const FString& M : Mots)
		{
			if (Ligne.Len() + M.Len() + 1 > Max && !Ligne.IsEmpty())
			{
				Sortie.Add(Ligne);
				Ligne.Empty();
			}
			Ligne += (Ligne.IsEmpty() ? TEXT("") : TEXT(" ")) + M;
		}
		if (!Ligne.IsEmpty())
		{
			Sortie.Add(Ligne);
		}
		return Sortie;
	}
}

void SPalmeraHud::Recit(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const
{
	APalmeraJoueur* J = Joueur.Get();
	APalmeraHistoire* H = J ? APalmeraHistoire::Lire(J) : nullptr;
	if (!H)
	{
		return;
	}
	if (H->SceneActive())
	{
		// Bandes noires de cinéma
		Boite(L, C, G, FVector2D::ZeroVector, FVector2D(Ecran.X, Ecran.Y * 0.09), FLinearColor(0, 0, 0, 1), 0.f);
		Boite(L, C, G, FVector2D(0.0, Ecran.Y * 0.91), FVector2D(Ecran.X, Ecran.Y * 0.09), FLinearColor(0, 0, 0, 1), 0.f);
	}
	const double Largeur = 980.0;
	const double X = (Ecran.X - Largeur) * 0.5;
	if (H->DialogueActif() && !H->ChoixOuvert())
	{
		const TArray<FString> Lignes = Couper(H->TexteAffiche(), 84);
		const double Hauteur = 62.0 + Lignes.Num() * 26.0;
		const double Y = Ecran.Y * 0.885 - Hauteur;
		Boite(L, C + 1, G, FVector2D(X, Y), FVector2D(Largeur, Hauteur), FLinearColor(0.01f, 0.015f, 0.03f, 0.8f), 12.f);
		Texte(L, C + 2, G, H->NomParleur(), FVector2D(X + 26.0, Y + 12.0), 15, H->CouleurParleur());
		for (int32 I = 0; I < Lignes.Num(); I++)
		{
			Texte(L, C + 2, G, Lignes[I], FVector2D(X + 26.0, Y + 40.0 + I * 26.0), 14, Blanc, 0.0, false);
		}
		if (H->SceneActive())
		{
			Texte(L, C + 2, G, TEXT("Entrée : continuer"), FVector2D(X + Largeur - 24.0, Y + 14.0), 10, FLinearColor(0.7f, 0.72f, 0.76f, 1.f), 1.0, false);
		}
	}
	if (H->ChoixOuvert())
	{
		const TArray<FString>& Choix = H->LignesDeChoix();
		const double Hauteur = 58.0 + Choix.Num() * 48.0;
		const double Y = Ecran.Y * 0.885 - Hauteur;
		Boite(L, C + 1, G, FVector2D(X, Y), FVector2D(Largeur, Hauteur), FLinearColor(0.01f, 0.015f, 0.03f, 0.88f), 12.f);
		Texte(L, C + 2, G, TEXT("Que réponds-tu ?"), FVector2D(X + 26.0, Y + 14.0), 14, Or);
		for (int32 I = 0; I < Choix.Num(); I++)
		{
			const double Yc = Y + 46.0 + I * 48.0;
			const bool bActif = I == H->ChoixSelectionne();
			if (bActif)
			{
				Boite(L, C + 2, G, FVector2D(X + 18.0, Yc - 4.0), FVector2D(Largeur - 36.0, 42.0), Corail * FLinearColor(1, 1, 1, 0.9f), 8.f);
			}
			Texte(L, C + 4, G, Choix[I], FVector2D(X + 34.0, Yc + 6.0), 14, bActif ? FLinearColor(0.02f, 0.02f, 0.04f, 1.f) : Blanc);
		}
	}
	if (!H->Invite.IsEmpty())
	{
		Texte(L, C + 2, G, H->Invite, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.74), 17, Blanc, 0.5);
	}
	else if (const APalmeraPartie* Partie = APalmeraPartie::Lire(J))
	{
		if (!Partie->Aide.IsEmpty())
		{
			Texte(L, C + 2, G, Partie->Aide, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.74), 17, Blanc, 0.5);
		}
	}
	if (J->GetWorld()->GetTimeSeconds() < H->FinTitre)
	{
		Boite(L, C + 2, G, FVector2D(0.0, Ecran.Y * 0.38), FVector2D(Ecran.X, 128.0), FLinearColor(0, 0, 0, 0.55f), 0.f);
		Texte(L, C + 3, G, H->TitreEcran, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.38 + 18.0), 34, Corail, 0.5);
		Texte(L, C + 3, G, H->SousTitreEcran, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.38 + 76.0), 18, Blanc, 0.5);
	}
	const auto Jauge = [&](const FString& Nom, double Part, const FLinearColor& Coul, double Y)
	{
		const double W = 300.0;
		const double Gx = (Ecran.X - W) * 0.5;
		Texte(L, C + 2, G, Nom, FVector2D(Ecran.X * 0.5, Y), 12, Blanc, 0.5);
		Boite(L, C + 2, G, FVector2D(Gx, Y + 22.0), FVector2D(W, 10.0), FLinearColor(1, 1, 1, 0.16f), 5.f);
		Boite(L, C + 3, G, FVector2D(Gx, Y + 22.0), FVector2D(FMath::Max(W * Part, 10.0), 10.0), Coul, 5.f);
	};
	if (H->Detection >= 0.0)
	{
		Jauge(TEXT("Soupçon des gardes"), H->Detection, FLinearColor::LerpUsingHSV(Or, Corail, float(H->Detection)), 112.0);
	}
	if (H->VieAlliee >= 0.0)
	{
		Jauge(TEXT("Voiture d'Inès"), H->VieAlliee, Turquoise, 112.0);
	}
	if (H->VieCible >= 0.0)
	{
		Jauge(H->NomCible, H->VieCible, Corail, 112.0);
	}
}

void SPalmeraHud::Journal(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const
{
	APalmeraJoueur* J = Joueur.Get();
	APalmeraHistoire* H = J ? APalmeraHistoire::Lire(J) : nullptr;
	const APalmeraPartie* Partie = J ? APalmeraPartie::Lire(J) : nullptr;
	if (!H || !Partie)
	{
		return;
	}
	const FVector2D Taille(760, 430);
	const FVector2D P = (Ecran - Taille) * 0.5;
	Boite(L, C, G, P, Taille, FLinearColor(0.02f, 0.03f, 0.05f, 0.97f), 16.f);
	Texte(L, C + 1, G, TEXT("JOURNAL"), FVector2D(Ecran.X * 0.5, P.Y + 22.0), 24, Corail, 0.5);
	Texte(L, C + 1, G, H->NomChapitre(), FVector2D(P.X + 40.0, P.Y + 84.0), 17, Or);
	double Y = P.Y + 122.0;
	for (const FString& Ligne : Couper(H->Resume().IsEmpty() ? FString(TEXT("L'histoire est terminée. La ville est à toi.")) : H->Resume(), 70))
	{
		Texte(L, C + 1, G, Ligne, FVector2D(P.X + 40.0, Y), 14, Blanc, 0.0, false);
		Y += 26.0;
	}
	Y += 18.0;
	Texte(L, C + 1, G, FString::Printf(TEXT("Partie %d sur %d de l'histoire"), FMath::Min(H->ChapitreActuel() + 1, H->NombreChapitres()), H->NombreChapitres()), FVector2D(P.X + 40.0, Y), 14, Turquoise, 0.0, false);
	Y += 30.0;
	Texte(L, C + 1, G, FString::Printf(TEXT("Cassettes de Marius : %d sur 20"), Partie->CassettesTrouvees), FVector2D(P.X + 40.0, Y), 14, Turquoise, 0.0, false);
	Y += 44.0;
	if (H->Drapeaux.Contains(TEXT("theo_couvert")))
	{
		Texte(L, C + 1, G, TEXT("Tu as couvert Théo. Il te doit une faveur."), FVector2D(P.X + 40.0, Y), 13, Blanc, 0.0, false);
		Y += 26.0;
	}
	if (H->Drapeaux.Contains(TEXT("theo_denonce")))
	{
		Texte(L, C + 1, G, TEXT("Tu as poussé Théo à dire la vérité à la police."), FVector2D(P.X + 40.0, Y), 13, Blanc, 0.0, false);
		Y += 26.0;
	}
	if (H->Drapeaux.Contains(TEXT("carla_arretee")))
	{
		Texte(L, C + 1, G, TEXT("Tu as livré la Murène à la lieutenante Benali."), FVector2D(P.X + 40.0, Y), 13, Blanc, 0.0, false);
	}
	else if (H->Drapeaux.Contains(TEXT("carla_libre")))
	{
		Texte(L, C + 1, G, TEXT("Tu as laissé partir la Murène."), FVector2D(P.X + 40.0, Y), 13, Blanc, 0.0, false);
	}
	Texte(L, C + 1, G, TEXT("Entrée : fermer"), FVector2D(Ecran.X * 0.5, P.Y + Taille.Y - 34.0), 11, FLinearColor(0.7f, 0.72f, 0.76f, 1.f), 0.5, false);
}

void SPalmeraHud::Aide(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran) const{
	const FVector2D Taille(760, 596);
	const FVector2D P = (Ecran - Taille) * 0.5;
	Boite(L, C, G, P, Taille, FLinearColor(0.02f, 0.03f, 0.05f, 0.92f), 16.f);
	Texte(L, C + 1, G, TEXT("BIENVENUE À PALMERA BAY"), FVector2D(Ecran.X * 0.5, P.Y + 24), 24, Corail, 0.5);
	static const TCHAR* Lignes[][2] = {
		{ TEXT("Z Q S D ou flèches"), TEXT("se déplacer, conduire") },
		{ TEXT("Souris"), TEXT("regarder autour de soi, à 360° en voiture") },
		{ TEXT("V"), TEXT("replacer la caméra derrière la voiture") },
		{ TEXT("Maj"), TEXT("courir") },
		{ TEXT("Espace"), TEXT("sauter, frein à main") },
		{ TEXT("F"), TEXT("monter dans une voiture ou en descendre") },
		{ TEXT("E"), TEXT("lancer une mission, sur un point doré") },
		{ TEXT("H"), TEXT("klaxon") },
		{ TEXT("C"), TEXT("changer de radio") },
		{ TEXT("L"), TEXT("phares") },
		{ TEXT("R"), TEXT("remettre la voiture sur ses roues") },
		{ TEXT("M ou Tab"), TEXT("carte de la ville") },
		{ TEXT("Échap ou P"), TEXT("menu, graphismes, sauvegarde") },
		{ TEXT("F1"), TEXT("revoir cette aide") }
	};
	double Y = P.Y + 80;
	for (const auto& Ligne : Lignes)
	{
		Texte(L, C + 1, G, Ligne[0], FVector2D(P.X + 300, Y), 13, Or, 1.0);
		Texte(L, C + 1, G, Ligne[1], FVector2D(P.X + 324, Y), 13, Blanc, 0.0, false);
		Y += 32;
	}
	Texte(L, C + 1, G, TEXT("Appuie sur Entrée pour jouer"), FVector2D(Ecran.X * 0.5, P.Y + Taille.Y - 40), 15, Turquoise, 0.5);
}

void SPalmeraHud::AfficherCourse(FSlateWindowElementList& L, int32 C, const FGeometry& G, FVector2D Ecran, const APalmeraCourse* Course) const
{
	const auto Temps = [](double T)
	{
		if (T < 0.0)
		{
			return FString(TEXT("--"));
		}
		const int32 M = FMath::FloorToInt(T / 60.0);
		return FString::Printf(TEXT("%d:%05.2f"), M, T - M * 60.0);
	};
	const FLinearColor Gris(0.7f, 0.72f, 0.76f, 1.f);
	const FCircuit& Circuit = Course->Circuits()[Course->ChoixCircuit];

	if (Course->Etat == EEtatCourse::Menu)
	{
		const FVector2D Taille(640, 430);
		const FVector2D P = (Ecran - Taille) * 0.5;
		Boite(L, C, G, P, Taille, FLinearColor(0.02f, 0.03f, 0.05f, 0.9f), 16.f);
		Texte(L, C + 2, G, TEXT("PALMERA RACING"), FVector2D(Ecran.X * 0.5, P.Y + 26), 32, Corail, 0.5);
		const FString Lignes[] = {
			FString::Printf(TEXT("Circuit :  < %s >"), *Circuit.Nom),
			FString::Printf(TEXT("Voiture :  < %s >"), *Course->NomVoiture()),
			FString::Printf(TEXT("Tours :  < %d >"), Course->ToursChoisis)
		};
		for (int32 I = 0; I < 3; I++)
		{
			const double Y = P.Y + 106 + I * 60;
			if (I == Course->ChoixLigne)
			{
				Boite(L, C + 1, G, FVector2D(P.X + 40, Y - 8), FVector2D(Taille.X - 80, 52), Corail * FLinearColor(1, 1, 1, 0.92f), 10.f);
			}
			Texte(L, C + 3, G, Lignes[I], FVector2D(Ecran.X * 0.5, Y + 4), 18, I == Course->ChoixLigne ? FLinearColor(0.02f, 0.02f, 0.04f, 1.f) : Blanc, 0.5);
		}
		Texte(L, C + 2, G, FString::Printf(TEXT("Record du circuit : %s"), *Temps(Course->Record())), FVector2D(Ecran.X * 0.5, P.Y + 300), 14, Or, 0.5);
		Texte(L, C + 2, G, TEXT("Haut, bas : choisir  ·  Gauche, droite : changer  ·  Entrée : lancer la course"), FVector2D(Ecran.X * 0.5, P.Y + Taille.Y - 40), 11, Gris, 0.5, false);
		return;
	}

	const FConcurrent* Moi = Course->Joueur();
	if (!Moi)
	{
		return;
	}
	if (Course->Etat == EEtatCourse::Depart)
	{
		Texte(L, C + 2, G, FString::FromInt(FMath::CeilToInt(Course->Compte - 1.0)), FVector2D(Ecran.X * 0.5, Ecran.Y * 0.26), 90, Or, 0.5);
	}
	else if (Course->Etat == EEtatCourse::Course && Course->Chrono < 1.2)
	{
		Texte(L, C + 2, G, TEXT("PARTEZ !"), FVector2D(Ecran.X * 0.5, Ecran.Y * 0.26), 70, Or, 0.5);
	}

	const double LargeurUtile = FMath::Min(Ecran.X, Ecran.Y * 2.1);
	const double Gauche = (Ecran.X - LargeurUtile) * 0.5 + 34.0;
	const TArray<const FConcurrent*> Liste = Course->Classement();
	Boite(L, C, G, FVector2D(Gauche, 24.0), FVector2D(270.0, 214.0), FLinearColor(0.01f, 0.015f, 0.03f, 0.72f), 12.f);
	Texte(L, C + 1, G, FString::Printf(TEXT("%d / %d"), Course->Rang(), Liste.Num()), FVector2D(Gauche + 20.0, 34.0), 34, Or);
	Texte(L, C + 1, G, FString::Printf(TEXT("Tour %d / %d"), FMath::Min(Moi->Tours + 1, Course->ToursChoisis), Course->ToursChoisis), FVector2D(Gauche + 20.0, 94.0), 16, Blanc);
	Texte(L, C + 1, G, Temps(Course->Chrono), FVector2D(Gauche + 20.0, 126.0), 16, Blanc);
	Texte(L, C + 1, G, FString::Printf(TEXT("Meilleur tour : %s"), *Temps(Course->MeilleurTour)), FVector2D(Gauche + 20.0, 160.0), 11, Gris, 0.0, false);
	Texte(L, C + 1, G, TEXT("R : revenir sur la piste"), FVector2D(Gauche + 20.0, 186.0), 11, Gris, 0.0, false);

	if (Course->Etat == EEtatCourse::Resultats)
	{
		const FVector2D Taille(600, 150.0 + Liste.Num() * 42.0);
		const FVector2D P = (Ecran - Taille) * 0.5;
		Boite(L, C + 4, G, P, Taille, FLinearColor(0.02f, 0.03f, 0.05f, 0.94f), 16.f);
		Texte(L, C + 5, G, Course->Rang() == 1 ? TEXT("VICTOIRE !") : TEXT("ARRIVÉE"), FVector2D(Ecran.X * 0.5, P.Y + 20), 28, Course->Rang() == 1 ? Or : Corail, 0.5);
		for (int32 I = 0; I < Liste.Num(); I++)
		{
			const double Y = P.Y + 82.0 + I * 42.0;
			const FLinearColor Teinte = Liste[I] == Moi ? Or : Blanc;
			Texte(L, C + 5, G, FString::Printf(TEXT("%d.  %s"), I + 1, *Liste[I]->Nom), FVector2D(P.X + 60.0, Y), 16, Teinte);
			Texte(L, C + 5, G, Liste[I]->Arrivee >= 0.0 ? Temps(Liste[I]->Arrivee) : FString(TEXT("en course")), FVector2D(P.X + Taille.X - 60.0, Y), 16, Teinte, 1.0);
		}
		Texte(L, C + 5, G, Course->bNouveauRecord ? TEXT("Nouveau record !  ·  Entrée : retour au menu") : TEXT("Entrée : retour au menu"), FVector2D(Ecran.X * 0.5, P.Y + Taille.Y - 40), 12, Gris, 0.5, false);
	}
}

int32 SPalmeraHud::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Rect, FSlateWindowElementList& L, int32 Couche, const FWidgetStyle& Style, bool bActif) const
{
	APalmeraJoueur* J = Joueur.Get();
	if (!J || !J->GetWorld())
	{
		return Couche;
	}
	APalmeraPartie* Partie = APalmeraPartie::Lire(J);
	const FVector2D Ecran(G.GetLocalSize());
	const int32 C = Couche + 1;

	// Sur un écran très large, l'interface reste dans une zone confortable au centre
	const double LargeurUtile = FMath::Min(Ecran.X, Ecran.Y * 2.1);
	const double Gauche = (Ecran.X - LargeurUtile) * 0.5 + 34.0;
	const double Droite = Ecran.X - Gauche;

	const APawn* Moi = J->GetPawn();
	const FVector Pos = Moi ? Moi->GetActorLocation() / Metre : FVector::ZeroVector;
	const double Cap = Moi ? J->GetControlRotation().Yaw : 0.0;
	const APalmeraVoiture* Voiture = J->VoitureActuelle();

	if (bCarteOuverte)
	{
		const double Cote = FMath::Min(Ecran.Y - 120.0, Ecran.X - 120.0);
		const double Echelle = Cote / 2300.0;
		const FVector2D Taille(FMath::Min(Ecran.X - 120.0, Cote * 1.5), Cote);
		const FVector2D Haut((Ecran - Taille) * 0.5);
		Boite(L, C, G, FVector2D::ZeroVector, Ecran, FLinearColor(0, 0, 0, 0.55f), 0.f);
		Plan(L, C + 1, G, Haut, Taille, FVector2D(-40.0, 0.0), Echelle, -90.0, true);
		Texte(L, C + 14, G, TEXT("PALMERA BAY"), FVector2D(Haut.X + 24, Haut.Y + 18), 24, Corail);
		Texte(L, C + 14, G, TEXT("M : fermer la carte"), FVector2D(Haut.X + 24, Haut.Y + Taille.Y - 34), 12, Blanc, 0.0, false);
		if (bMenuOuvert)
		{
			Menu(L, C + 30, G, Ecran);
		}
		return C + 40;
	}

	// Pendant une scène, seuls les bandes et le dialogue restent à l'écran
	const APalmeraHistoire* Histoire = APalmeraHistoire::Lire(J);
	const APalmeraLieux* Lieux = APalmeraLieux::Lire(J);
	const bool bScene = Histoire && Histoire->SceneActive();
	const bool bDedans = Lieux && Lieux->EstDedans();

	// Mini-carte, en bas à gauche
	if (!bScene && !bDedans)
	{
		const FVector2D TailleMini(300, 200);
		const FVector2D HautMini(Gauche, Ecran.Y - TailleMini.Y - 34.0);
		Plan(L, C, G, HautMini, TailleMini, FVector2D(Pos.X, Pos.Y), Voiture ? 0.42 : 0.7, Voiture ? Voiture->GetActorRotation().Yaw : Cap, false);
		Texte(L, C + 14, G, APalmeraPartie::NomQuartier(FVector2D(Pos.X, Pos.Y)), FVector2D(HautMini.X + 2, HautMini.Y - 28), 13, Blanc);
	}

	// Argent, heure et recherche, en haut à droite
	if (Partie && !bScene)
	{
		Texte(L, C + 2, G, FString::Printf(TEXT("%s $"), *Espacer(Partie->Argent)), FVector2D(Droite, 28), 22, FLinearColor(0.6f, 1.f, 0.65f, 1.f), 1.0);
		if (const APalmeraMonde* Monde = TActorIterator<APalmeraMonde>(J->GetWorld()) ? *TActorIterator<APalmeraMonde>(J->GetWorld()) : nullptr)
		{
			const int32 H = FMath::FloorToInt(Monde->Heure);
			const int32 M = FMath::FloorToInt((Monde->Heure - H) * 60.0);
			Texte(L, C + 2, G, FString::Printf(TEXT("%02d:%02d"), H, M), FVector2D(Droite, 66), 15, Blanc, 1.0);
		}
		Texte(L, C + 2, G, FString::Printf(TEXT("Cassettes %d / %d"), Partie->CassettesTrouvees, Partie->CassettesTrouvees + Partie->Cassettes.Num()), FVector2D(Droite, 92), 11, Turquoise, 1.0, false);
		for (int32 I = 0; I < 5; I++)
		{
			const bool bPleine = I < Partie->Recherche;
			const bool bClignote = bPleine && FMath::Fmod(FPlatformTime::Seconds() * 2.0, 1.0) < 0.5;
			if (Partie->Recherche > 0)
			{
				Etoile(L, C + 3, G, FVector2D(Droite - 13.0 - (4 - I) * 34.0, 134.0), 13.0,
					bPleine ? (bClignote ? Blanc : Or) : FLinearColor(0.25f, 0.25f, 0.28f, 0.75f));
			}
		}

		if (!Partie->Objectif.IsEmpty())
		{
			Boite(L, C + 2, G, FVector2D(Ecran.X * 0.5 - 340.0, 22.0), FVector2D(680.0, 46.0), Sombre, 12.f);
			Texte(L, C + 3, G, Partie->Objectif, FVector2D(Ecran.X * 0.5, 33.0), 15, Blanc, 0.5);
		}
		if (Partie->Chrono >= 0.0)
		{
			const int32 S = FMath::CeilToInt(Partie->Chrono);
			Texte(L, C + 3, G, FString::Printf(TEXT("%d:%02d"), S / 60, S % 60), FVector2D(Ecran.X * 0.5, 76.0), 24, S <= 10 ? Corail : Blanc, 0.5);
		}
		if (J->GetWorld()->GetTimeSeconds() < Partie->FinMessage)
		{
			Texte(L, C + 5, G, Partie->Message, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.2), 30, Or, 0.5);
		}
	}

	// Compteur, en bas à droite
	if (Voiture && !bScene)
	{
		const int32 Kmh = FMath::RoundToInt(FMath::Abs(Voiture->VitesseKmh()));
		Boite(L, C + 2, G, FVector2D(Droite - 246.0, Ecran.Y - 150.0), FVector2D(246.0, 116.0), Sombre, 14.f);
		Texte(L, C + 3, G, FString::FromInt(Kmh), FVector2D(Droite - 74.0, Ecran.Y - 142.0), 44, Blanc, 1.0);
		Texte(L, C + 3, G, TEXT("km/h"), FVector2D(Droite - 66.0, Ecran.Y - 102.0), 12, FLinearColor(0.8f, 0.82f, 0.86f, 1.f), 0.0, false);
		const double Part = FMath::Clamp((Voiture->Regime() - 900.0) / 6500.0, 0.0, 1.0);
		Boite(L, C + 3, G, FVector2D(Droite - 228.0, Ecran.Y - 56.0), FVector2D(210.0, 8.0), FLinearColor(1, 1, 1, 0.14f), 4.f);
		Boite(L, C + 4, G, FVector2D(Droite - 228.0, Ecran.Y - 56.0), FVector2D(FMath::Max(210.0 * Part, 8.0), 8.0), Part > 0.82 ? Corail : Turquoise, 4.f);
		Texte(L, C + 3, G, FString::Printf(TEXT("R%d"), Voiture->Rapport()), FVector2D(Droite - 20.0, Ecran.Y - 92.0), 14, Or, 1.0);
		const double Etat = FMath::Clamp(Voiture->LireVie() / FMath::Max(Voiture->LireVieMax(), 1.0), 0.0, 1.0);
		Boite(L, C + 3, G, FVector2D(Droite - 228.0, Ecran.Y - 42.0), FVector2D(210.0, 6.0), FLinearColor(1, 1, 1, 0.14f), 3.f);
		Boite(L, C + 4, G, FVector2D(Droite - 228.0, Ecran.Y - 42.0), FVector2D(FMath::Max(210.0 * Etat, 6.0), 6.0), Etat < 0.3 ? Corail : FLinearColor(0.5f, 0.9f, 0.5f, 1.f), 3.f);
	}

	Recit(L, C + 18, G, Ecran);
	if (const APalmeraCourse* Course = APalmeraCourse::Lire(J))
	{
		AfficherCourse(L, C + 18, G, Ecran, Course);
	}
	if (Lieux)
	{
		if (!Lieux->Invite.IsEmpty())
		{
			Texte(L, C + 20, G, Lieux->Invite, FVector2D(Ecran.X * 0.5, Ecran.Y * 0.74), 15, Or, 0.5);
		}
		if (Lieux->TempsBulle > 0.0)
		{
			const TArray<FString> Lignes = Couper(Lieux->Bulle, 84);
			const double Largeur = 980.0;
			const double Hauteur = 34.0 + Lignes.Num() * 26.0;
			const FVector2D Coin((Ecran.X - Largeur) * 0.5, Ecran.Y * 0.885 - Hauteur);
			Boite(L, C + 20, G, Coin, FVector2D(Largeur, Hauteur), FLinearColor(0.01f, 0.015f, 0.03f, 0.8f), 12.f);
			for (int32 I = 0; I < Lignes.Num(); I++)
			{
				Texte(L, C + 21, G, Lignes[I], FVector2D(Coin.X + 26.0, Coin.Y + 16.0 + I * 26.0), 14, Blanc, 0.0, false);
			}
		}
		if (Lieux->Noir > 0.0)
		{
			Boite(L, C + 26, G, FVector2D::ZeroVector, Ecran, FLinearColor(0.f, 0.f, 0.f, float(Lieux->Noir)), 0.f);
		}
	}
	if (bMenuOuvert)
	{
		if (bJournalOuvert)
		{
			Journal(L, C + 30, G, Ecran);
		}
		else
		{
			Menu(L, C + 30, G, Ecran);
		}
	}
	else if (bAideOuverte)
	{
		Aide(L, C + 30, G, Ecran);
	}
	return C + 50;
}
