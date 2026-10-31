# Essai de l'histoire au vrai clavier et à la vraie souris dans le jeu final : caméra 360° au volant, fumée, dialogues, choix, journal.
# Chaque frappe est refusée si le jeu n'est pas au premier plan. F9 n'existe qu'avec -testpas : elle saute à l'objectif de l'étape.
param([string]$Sortie = 'E:\Minecraft\Claude Unreal Engine\PalmeraBay\Saved\Photos', [ValidateSet('prologue', 'choix')][string]$Essai = 'prologue', [string]$Racine = 'E:\Minecraft\Claude Unreal Engine\Palmera Bay')
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class Clavier {
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint data, flags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Explicit)] public struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; [FieldOffset(8)] public MOUSEINPUT mi; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] p, int taille);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mode);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool lier);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int mode);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    public static uint ProcessusDevant() { uint pid; GetWindowThreadProcessId(GetForegroundWindow(), out pid); return pid; }
    public static void Amener(IntPtr h) {
        uint pid;
        uint autre = GetWindowThreadProcessId(GetForegroundWindow(), out pid);
        uint moi = GetCurrentThreadId();
        AttachThreadInput(moi, autre, true);
        BringWindowToTop(h);
        ShowWindow(h, 5);
        SetForegroundWindow(h);
        AttachThreadInput(moi, autre, false);
    }
    public static string Capturer(string chemin) {
        IntPtr h = GetForegroundWindow();
        RECT r; GetClientRect(h, out r);
        POINT p = new POINT(); ClientToScreen(h, ref p);
        using (var b = new System.Drawing.Bitmap(r.R, r.B)) using (var g = System.Drawing.Graphics.FromImage(b)) {
            g.CopyFromScreen(p.X, p.Y, 0, 0, b.Size);
            b.Save(chemin, System.Drawing.Imaging.ImageFormat.Png);
        }
        return r.R + "x" + r.B;
    }
    public static void Touche(ushort vk, bool enfoncee, bool etendue) {
        var e = new INPUT[1]; e[0].type = 1;
        e[0].ki.wScan = (ushort)MapVirtualKey(vk, 0);
        e[0].ki.dwFlags = 0x0008u | (enfoncee ? 0u : 0x0002u) | (etendue ? 0x0001u : 0u);
        SendInput(1, e, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void Souris(int dx, int dy) {
        var e = new INPUT[1]; e[0].type = 0; e[0].mi.dx = dx; e[0].mi.dy = dy; e[0].mi.flags = 0x0001u;
        SendInput(1, e, Marshal.SizeOf(typeof(INPUT)));
    }
}
'@
[void][Clavier]::SetProcessDPIAware()
$exe = "$Racine\Windows\PalmeraBay\Binaries\Win64\PalmeraBay-Win64-Shipping.exe"
Remove-Item "$Sortie\parcours.txt" -ErrorAction SilentlyContinue
# La sauvegarde du joueur est mise de côté pendant l'essai, puis remise : jamais effacée
if (Get-Process -Name 'PalmeraBay-Win64-Shipping' -ErrorAction SilentlyContinue) { 'Le jeu est deja ouvert : essai annule pour ne pas gener le joueur.'; exit 2 }
$sauvegarde = "$env:LOCALAPPDATA\PalmeraBay\Saved\SaveGames\Palmera.sav"
$cote = 'E:\UnrealCache\Temp\Palmera.joueur.sav'
if (Test-Path $sauvegarde) { Copy-Item $sauvegarde $cote -Force; Remove-Item $sauvegarde }
$arguments = @('-windowed', '-ResX=1920', '-ResY=1080', '-silence', "`"-photo=$Sortie\fin_essai.png`"", '-cam=joueur', '-heure=16.5', '-attente=420', '-avecinterface', '-sansaide', '-releve', '-testpas')
if ($Essai -eq 'choix') { $arguments += '-chapitre=4' }
$jeu = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru
$script:Refus = 0
$script:Reprises = 0
# Le jeu est amené devant six fois au plus : au-delà, quelqu'un se sert du PC et le test ne se bat pas avec lui.
function Devant {
    if ([Clavier]::ProcessusDevant() -eq $jeu.Id) { return $true }
    if ($script:Reprises -ge 6) { return $false }
    $script:Reprises++
    $jeu.Refresh()
    [Clavier]::Amener($jeu.MainWindowHandle)
    Start-Sleep -Milliseconds 400
    return [Clavier]::ProcessusDevant() -eq $jeu.Id
}
function Maintenir([int[]]$Touches, [int]$Duree, [string]$Nom, [bool]$Etendue = $false) {
    if (-not (Devant)) { "REFUS $Nom : le jeu n'est pas au premier plan"; $script:Refus++; return }
    "$((Get-Date).ToString('HH:mm:ss')) $Nom"
    foreach ($k in $Touches) { [Clavier]::Touche([uint16]$k, $true, $Etendue) }
    Start-Sleep -Milliseconds $Duree
    foreach ($k in $Touches) { [Clavier]::Touche([uint16]$k, $false, $Etendue) }
    Start-Sleep -Milliseconds 350
}
function Tourner([int]$Dx, [int]$Pas, [string]$Nom) {
    if (-not (Devant)) { "REFUS $Nom : le jeu n'est pas au premier plan"; $script:Refus++; return }
    "$((Get-Date).ToString('HH:mm:ss')) $Nom"
    for ($i = 0; $i -lt $Pas; $i++) { [Clavier]::Souris($Dx, 0); Start-Sleep -Milliseconds 12 }
    Start-Sleep -Milliseconds 500
}
function Photo([string]$Nom) {
    if (-not (Devant)) { "REFUS photo $Nom"; $script:Refus++; return }
    "$((Get-Date).ToString('HH:mm:ss')) photo $Nom ($([Clavier]::Capturer("$Sortie\$Nom.png")))"
}
function Passer([int]$Fois, [string]$Nom) {
    "$((Get-Date).ToString('HH:mm:ss')) $Nom : Entree x $Fois"
    for ($i = 0; $i -lt $Fois; $i++) { Maintenir @($VK_ENTREE) 90 'replique' | Out-Null; Start-Sleep -Milliseconds 250 }
}
function Etape {
    if (-not (Test-Path "$Sortie\parcours.txt")) { return '' }
    $l = Get-Content "$Sortie\parcours.txt" -Encoding utf8 | Where-Object { $_ -match 'chapitre \d+, etape' } | Select-Object -Last 1
    return "$l"
}
# Le premier appui affiche toute la réplique, le second passe à la suivante : on appuie jusqu'à ce que l'étape change.
function JusquA([string]$Motif, [int]$Max = 40, [int[]]$Touche = @(0x0D)) {
    for ($i = 0; $i -lt $Max; $i++) {
        if ((Etape) -match $Motif) { "$((Get-Date).ToString('HH:mm:ss')) arrive : $(Etape)"; return }
        Maintenir $Touche 90 'appui' | Out-Null
        Start-Sleep -Milliseconds 150
    }
    "PAS ARRIVE A $Motif : $(Etape)"
}
function Patienter([string]$Motif, [int]$Secondes) {
    $fin = (Get-Date).AddSeconds($Secondes)
    while ((Get-Date) -lt $fin) {
        if ((Etape) -match $Motif) { "$((Get-Date).ToString('HH:mm:ss')) arrive : $(Etape)"; return }
        Start-Sleep -Milliseconds 300
    }
    "PAS ARRIVE A $Motif : $(Etape)"
}
$VK_Z = 0x5A; $VK_D = 0x44; $VK_E = 0x45; $VK_F = 0x46; $VK_ESPACE = 0x20; $VK_ECHAP = 0x1B; $VK_ENTREE = 0x0D; $VK_BAS = 0x28; $VK_F9 = 0x78; $VK_M = 0x4D
$limite = (Get-Date).AddSeconds(50)
while ($jeu.MainWindowHandle -eq 0 -and (Get-Date) -lt $limite) { Start-Sleep -Milliseconds 300; $jeu.Refresh() }
Start-Sleep -Seconds 16

if ($Essai -eq 'prologue') {
    Photo 'h1_appel'
    Maintenir @($VK_F) 120 'monter avec F depuis le point de depart'
    Start-Sleep -Seconds 3
    Photo 'h2_derriere'
    foreach ($angle in 90, 180, 270, 360) {
        Tourner 13 41 "camera + 90 degres (cumul $angle)"
        Photo "h3_orbite$angle"
    }
    Tourner -13 41 'camera dans l''autre sens'
    Photo 'h4_autre_sens'
    # En roulant : le regard reste libre, puis revient derrière la voiture
    if (Devant) { 'accelerer avec Z'; [Clavier]::Touche([uint16]$VK_Z, $true, $false) }
    Start-Sleep -Milliseconds 2600
    Tourner 13 82 'regarder derriere en roulant'
    Photo 'h5_regard_arriere'
    Start-Sleep -Milliseconds 3200
    Photo 'h6_retour_derriere'
    Maintenir @($VK_D, $VK_ESPACE) 750 'frein a main en tournant'
    Photo 'h7_fumee'
    [Clavier]::Touche([uint16]$VK_Z, $false, $false)
    Start-Sleep -Seconds 1
    Patienter 'chapitre 0, etape 3' 25
    Maintenir @($VK_F9) 120 'F9 : aller au garage'
    Patienter 'chapitre 0, etape 4' 6
    Start-Sleep -Seconds 2
    Photo 'h8_scene_nina'
    Passer 2 'replique suivante'
    Start-Sleep -Milliseconds 900
    Photo 'h9_scene_sam'
    JusquA 'chapitre 0, etape 5'
    Start-Sleep -Milliseconds 1500
    Photo 'h10_scene_bruno'
    JusquA 'chapitre 0, etape 7'
    Start-Sleep -Seconds 4
    Photo 'h11_ennemi'
    Maintenir @($VK_F9) 120 'F9 : arreter les Murenes'
    Patienter 'chapitre 0, etape 9' 14
    Maintenir @($VK_F9) 120 'F9 : retour vers Nina'
    Start-Sleep -Seconds 1
    Photo 'h12_invite_parler'
    Maintenir @($VK_E) 120 'parler avec E'
    Patienter 'chapitre 0, etape 10' 4
    JusquA 'chapitre 1, etape'
    Start-Sleep -Seconds 2
    Photo 'h13_chapitre1'
    Maintenir @($VK_Z) 900 'marcher apres la scene'
    Maintenir @($VK_ECHAP) 120 'ouvrir le menu'
    Maintenir @($VK_BAS) 120 'descendre' $true
    Maintenir @($VK_BAS) 120 'descendre jusqu au journal' $true
    Maintenir @($VK_ENTREE) 120 'ouvrir le journal'
    Start-Sleep -Milliseconds 800
    Photo 'h14_journal'
    Maintenir @($VK_ECHAP) 120 'fermer'
    Start-Sleep -Milliseconds 500
    Maintenir @($VK_ECHAP) 120 'fermer le menu si besoin'
    Start-Sleep -Milliseconds 500
    Photo 'h15_apres_menu'
}
else {
    Photo 'c1_debut'
    Patienter 'chapitre 4, etape 2' 6
    Maintenir @($VK_F9) 120 'F9 : aller a Carla'
    Start-Sleep -Seconds 1
    Maintenir @($VK_E) 120 'parler avec E'
    Patienter 'chapitre 4, etape 3' 4
    Start-Sleep -Seconds 2
    Photo 'c2_carla'
    JusquA 'chapitre 4, etape 5'
    Start-Sleep -Seconds 4
    Photo 'c3_course'
    JusquA 'chapitre 4, etape 6' 24 @(0x78)
    Start-Sleep -Seconds 1
    Maintenir @($VK_F9) 120 'F9 : aller a Theo'
    Start-Sleep -Seconds 1
    Maintenir @($VK_E) 120 'parler avec E'
    Patienter 'chapitre 4, etape 7' 4
    Start-Sleep -Seconds 1
    Passer 4 'deux repliques de Theo'
    Start-Sleep -Milliseconds 800
    Photo 'c4_choix'
    Maintenir @($VK_BAS) 120 'choisir la deuxieme reponse' $true
    Start-Sleep -Milliseconds 400
    Photo 'c5_choix_bas'
    Passer 1 'valider'
    Start-Sleep -Milliseconds 1200
    Photo 'c6_reponse'
    JusquA 'chapitre 5, etape'
    Start-Sleep -Seconds 3
    Photo 'c7_chapitre5'
    # Quitter par le menu : le jeu doit se fermer proprement
    Maintenir @($VK_ECHAP) 120 'ouvrir le menu'
    for ($i = 0; $i -lt 5; $i++) { Maintenir @($VK_BAS) 100 'descendre' $true }
    Photo 'c8_quitter'
    Maintenir @($VK_ENTREE) 120 'quitter'
}
"Frappes refusees : $script:Refus"
function RemettreSauvegarde {
    if ($jeu -and -not $jeu.HasExited) { return }
    Remove-Item $sauvegarde -ErrorAction SilentlyContinue
    if (Test-Path $cote) { Move-Item $cote $sauvegarde -Force; 'sauvegarde du joueur remise' }
}
if ($Essai -eq 'choix') {
    if (-not $jeu.WaitForExit(15000)) { 'JEU TOUJOURS OUVERT apres Quitter'; Stop-Process -Id $jeu.Id -Force } else { "Sortie du jeu : $($jeu.ExitCode)" }
}
else { Stop-Process -Id $jeu.Id -Force; 'jeu ferme par le script' }
Start-Sleep -Seconds 2
RemettreSauvegarde
'--- ETAPES'
if (Test-Path "$Sortie\parcours.txt") { Get-Content "$Sortie\parcours.txt" -Encoding utf8 | Where-Object { $_ -notmatch '^t=[0-9.]+ pos=' -and $_ -notmatch ' pos=\(' } } else { 'pas de releve' }
'--- POSITIONS (une sur vingt)'
if (Test-Path "$Sortie\parcours.txt") { $n = 0; Get-Content "$Sortie\parcours.txt" -Encoding utf8 | Where-Object { $_ -match ' pos=\(' } | ForEach-Object { if ($n++ % 20 -eq 0) { $_ } } }
