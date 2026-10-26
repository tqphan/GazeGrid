# Run only on an isolated Windows test machine with the signing certificate trusted.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Msi, [Parameter(Mandatory)][string]$CertificateThumbprint)
$ErrorActionPreference = 'Stop'
$path = (Resolve-Path $Msi).Path
$installed = "$env:ProgramFiles/GazeGrid/gazegrid.exe"
function Run-Msi([string]$Operation) {
    $process = Start-Process msiexec.exe -ArgumentList @($Operation, "`"$path`"", '/qn', '/norestart') -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) { throw "MSI failed: $($process.ExitCode)" }
}
Run-Msi '/i'
try {
    $signature = Get-AuthenticodeSignature $installed
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $CertificateThumbprint) {
        throw 'Installed application signature did not match GazeGrid.'
    }
    # Avoid inheriting Qt paths from the build environment: verify deployed plugins.
    Remove-Item Env:QT_PLUGIN_PATH -ErrorAction SilentlyContinue
    Remove-Item Env:QML2_IMPORT_PATH -ErrorAction SilentlyContinue
    Remove-Item Env:QML_IMPORT_PATH -ErrorAction SilentlyContinue
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:GAZEGRID_CONFIG_DIR = "$env:RUNNER_TEMP/gazegrid-installed-test"
    if (!(Test-Path "$env:ProgramFiles/GazeGrid/keyboards/main-14x1-bottom.xml")) { throw 'Installed keyboard pack is missing.' }
    $report = "$env:TEMP/gazegrid-installed-diagnose.json"
    $diagnose = Start-Process $installed -ArgumentList @('--diagnose', '--validate-keyboards') -WorkingDirectory $env:TEMP -PassThru -RedirectStandardOutput $report
    if (!$diagnose.WaitForExit(30000)) { $diagnose.Kill(); throw 'Installed diagnostics timed out.' }
    if ($diagnose.ExitCode -ne 0) { throw 'Installed diagnostics failed.' }
    $diagnostics = Get-Content $report -Raw | ConvertFrom-Json
    if (!$diagnostics.portal.uiAccess) { throw 'Windows did not grant UIAccess to the installed application.' }
    $process = Start-Process $installed -ArgumentList '--smoke' -WorkingDirectory $env:TEMP -PassThru
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw 'Installed application timed out.' }
    if ($process.ExitCode -ne 0) { throw "Installed application failed: $($process.ExitCode)" }
} finally { Run-Msi '/x' }
