# Fabrique le jeu final (exe autonome, sans l'éditeur). Par défaut dans « E:\Minecraft\Claude Unreal Engine\Palmera Bay ».
param([string]$Sortie = 'E:\Minecraft\Claude Unreal Engine\Palmera Bay')
. "$PSScriptRoot\env.ps1"
$uat = "$Moteur\Engine\Build\BatchFiles\RunUAT.bat"
& $uat BuildCookRun "-project=$Projet" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -compressed -archive "-archivedirectory=$Sortie" -unattended -utf8output -NoCodeSign "-ubtargs=-NoUBA"
exit $LASTEXITCODE
