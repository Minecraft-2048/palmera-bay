# Lance une copie isolée de Blender (réglages et add-on sur E:, rien dans le Blender de Steam) sur un script Python.
param([Parameter(Mandatory)][string]$Script, [string]$Arguments = '')
$r = 'E:\UnrealTools\MakeHuman\blender'
$env:BLENDER_USER_CONFIG = "$r\config"
$env:BLENDER_USER_SCRIPTS = "$r\scripts"
$env:BLENDER_USER_DATAFILES = "$r\datafiles"
$env:BLENDER_USER_EXTENSIONS = "$r\extensions"
$env:TEMP = 'E:\UnrealCache\Temp'; $env:TMP = 'E:\UnrealCache\Temp'
$blender = 'C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe'
& $blender --background --factory-startup --python $Script -- $Arguments
exit $LASTEXITCODE
