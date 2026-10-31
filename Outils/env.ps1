# Environnement commun : tout ce qui est temporaire ou en cache va sur E: (C: est presque plein).
$env:TEMP = 'E:\UnrealCache\Temp'
$env:TMP = 'E:\UnrealCache\Temp'
${env:UE-LocalDataCachePath} = 'E:\UnrealCache\DDC'
${env:UE-ZenDataPath} = 'E:\UnrealCache\Zen'
$script:Moteur = 'E:\Epic Games\UE_5.8'
$script:Projet = 'E:\Minecraft\Claude Unreal Engine\PalmeraBay\PalmeraBay.uproject'
$script:EditeurCmd = "$script:Moteur\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$script:Editeur = "$script:Moteur\Engine\Binaries\Win64\UnrealEditor.exe"
