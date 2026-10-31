# Exécute un script Python dans l'éditeur, sans fenêtre.
# -Gpu garde la carte graphique active (nécessaire pour compiler les shaders ou faire des captures).
param([Parameter(Mandatory = $true)][string]$Script, [switch]$Gpu)
. "$PSScriptRoot\env.ps1"
$Script = $Script -replace '\\', '/'
$arguments = @($Projet, '-run=pythonscript', "-script=$Script", '-unattended', '-nosplash', '-nopause', '-stdout', '-FullStdOutLogOutput', '-log=Python.log')
if (-not $Gpu) { $arguments += '-nullrhi' }
& $EditeurCmd @arguments
exit $LASTEXITCODE
