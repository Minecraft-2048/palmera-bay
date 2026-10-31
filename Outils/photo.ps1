# Lance le jeu, prend une photo à l'endroit demandé et se ferme.
param(
	[string]$Nom = 'essai',
	[string]$Cam = '-950,0,6,180,0',
	[double]$Heure = 17.6,
	[int]$L = 1920,
	[int]$H = 1080,
	[int]$Attente = 12,
	[string]$Extra = ''
)
. "$PSScriptRoot\env.ps1"
$chemin = "E:\Minecraft\Claude Unreal Engine\PalmeraBay\Saved\Photos\$Nom.png"
$heureTexte = $Heure.ToString([Globalization.CultureInfo]::InvariantCulture)
$arguments = @($Projet, '/Game/Cartes/Palmera', '-game', '-windowed', "-ResX=$L", "-ResY=$H", '-nosound', '-nosplash', '-unattended',
	"-photo=$chemin", "-cam=$Cam", "-heure=$heureTexte", '-tempsfige', "-attente=$Attente", '-log=Photo.log', '-stdout', '-FullStdOutLogOutput', '-sansaide')
if ($Extra) { $arguments += $Extra.Split(' ') }
$guillemets = $arguments | ForEach-Object { if ($_ -match ' ') { '"' + $_ + '"' } else { $_ } }
$p = Start-Process -FilePath $Editeur -ArgumentList $guillemets -PassThru -Wait
exit $p.ExitCode
