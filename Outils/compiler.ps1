# Compile la cible éditeur (par défaut) ou le jeu. Les fichiers temporaires restent sur E:.
param([string]$Cible = 'PalmeraBayEditor', [string]$Config = 'Development')
$env:TEMP = 'E:\UnrealCache\Temp'
$env:TMP = 'E:\UnrealCache\Temp'
$projet = 'E:\Minecraft\Claude Unreal Engine\PalmeraBay\PalmeraBay.uproject'
& 'E:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' $Cible Win64 $Config "-Project=$projet" -WaitMutex -NoHotReload -NoUBA
exit $LASTEXITCODE
