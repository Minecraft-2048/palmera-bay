// Palmera Racing : lance le jeu de course, qui partage le moteur et la ville de Palmera Bay.
using System.Diagnostics;
using System.IO;
using System.Reflection;

class Lanceur
{
    static void Main()
    {
        string dossier = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
        ProcessStartInfo jeu = new ProcessStartInfo(Path.Combine(dossier, "PalmeraBay.exe"), "-course -sansaide");
        jeu.WorkingDirectory = dossier;
        Process.Start(jeu);
    }
}
