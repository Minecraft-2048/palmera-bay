# Essai de vrai clavier et de vraie souris dans le jeu final. Chaque frappe est refusée si le jeu n'est pas au premier plan.
param([string]$Sortie = 'E:\Minecraft\Claude Unreal Engine\PalmeraBay\Saved\Photos')
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class Clavier {
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint data, flags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Explicit)] public struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; [FieldOffset(8)] public MOUSEINPUT mi; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] p, int taille);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mode);
    public static uint ProcessusDevant() { uint pid; GetWindowThreadProcessId(GetForegroundWindow(), out pid); return pid; }
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
$exe = 'E:\Minecraft\Claude Unreal Engine\Palmera Bay\Windows\PalmeraBay\Binaries\Win64\PalmeraBay-Win64-Shipping.exe'
Remove-Item "$Sortie\parcours.txt" -ErrorAction SilentlyContinue
$jeu = Start-Process -FilePath $exe -ArgumentList @('-windowed', '-ResX=1920', '-ResY=1080', '-silence', "`"-photo=$Sortie\clavier.png`"", '-cam=joueur', '-heure=17.6', '-tempsfige', '-attente=62', '-avecinterface', '-sansaide', '-releve') -PassThru
$script:Refus = 0
function Devant {
    if ([Clavier]::ProcessusDevant() -eq $jeu.Id) { return $true }
    [void][Clavier]::SetForegroundWindow($jeu.MainWindowHandle)
    Start-Sleep -Milliseconds 300
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
# Attend que la fenêtre existe et que la ville soit chargée
$limite = (Get-Date).AddSeconds(40)
while ($jeu.MainWindowHandle -eq 0 -and (Get-Date) -lt $limite) { Start-Sleep -Milliseconds 300; $jeu.Refresh() }
Start-Sleep -Seconds 14
$VK_Z = 0x5A; $VK_Q = 0x51; $VK_S = 0x53; $VK_D = 0x44; $VK_F = 0x46; $VK_MAJ = 0xA0; $VK_ESPACE = 0x20; $VK_ECHAP = 0x1B; $VK_ENTREE = 0x0D; $VK_BAS = 0x28; $VK_M = 0x4D; $VK_L = 0x4C; $VK_C = 0x43
Maintenir @($VK_Z) 900 'marche avec Z'
Maintenir @($VK_Q) 500 'pas de cote avec Q'
if (Devant) { 'souris vers la droite'; for ($i = 0; $i -lt 40; $i++) { [Clavier]::Souris(12, 0); Start-Sleep -Milliseconds 15 } ; for ($i = 0; $i -lt 40; $i++) { [Clavier]::Souris(-12, 0); Start-Sleep -Milliseconds 15 } }
Maintenir @($VK_Z, $VK_MAJ) 500 'course avec Maj'
Maintenir @($VK_ESPACE) 150 'saut'
Start-Sleep -Milliseconds 900
Maintenir @($VK_F) 120 'monter avec F'
Start-Sleep -Milliseconds 700
Maintenir @($VK_L) 120 'phares'
Maintenir @($VK_C) 120 'radio'
Maintenir @($VK_Z) 3200 'accelerer avec Z'
Maintenir @($VK_D, $VK_Z) 900 'tourner avec D vers la plage'
Maintenir @($VK_Z) 3500 'rouler sur le sable'
Maintenir @($VK_ESPACE) 900 'frein a main'
Maintenir @($VK_S) 1800 'freiner et reculer avec S'
Maintenir @($VK_ECHAP) 120 'ouvrir le menu'
Start-Sleep -Milliseconds 700
Maintenir @($VK_BAS) 120 'descendre dans le menu' $true
Maintenir @($VK_ENTREE) 120 'changer les graphismes'
Start-Sleep -Milliseconds 900
Maintenir @($VK_ENTREE) 120 'changer encore'
Maintenir @($VK_ENTREE) 120 'et encore'
Maintenir @($VK_ENTREE) 120 'et encore'
Maintenir @($VK_ENTREE) 120 'retour au reglage de depart'
Maintenir @($VK_ECHAP) 120 'fermer le menu'
Maintenir @($VK_M) 120 'ouvrir la carte'
Start-Sleep -Seconds 1
Maintenir @($VK_M) 120 'fermer la carte'
Maintenir @($VK_F) 120 'descendre avec F'
Start-Sleep -Seconds 1
"Frappes refusees : $script:Refus"
if (-not $jeu.WaitForExit(120000)) { 'jeu toujours ouvert, fermeture'; Stop-Process -Id $jeu.Id -Force } else { "Sortie du jeu : $($jeu.ExitCode)" }
'--- RELEVE'
if (Test-Path "$Sortie\parcours.txt") { Get-Content "$Sortie\parcours.txt" -Encoding utf8 } else { 'pas de releve' }
