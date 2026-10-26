# Run only on an isolated Windows test machine: temporarily removes this test root.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Msi, [Parameter(Mandatory)][string]$CertificateThumbprint)
$ErrorActionPreference = 'Stop'
$path = (Resolve-Path $Msi).Path
$installed = "$env:ProgramFiles/GazeGrid/gazegrid.exe"
function Run-Msi([string]$Operation, [string[]]$Properties = @()) {
    $process = Start-Process msiexec.exe -ArgumentList (@($Operation, "`"$path`"", '/qn', '/norestart', '/l*v', "`"$env:RUNNER_TEMP/gazegrid-msi-$mode-$($Operation.TrimStart('/')).log`"") + $Properties) -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) { throw "MSI failed: $($process.ExitCode)" }
}
Remove-Item "Cert:\LocalMachine\Root\$CertificateThumbprint" -Force
foreach ($mode in @('default', 'accept', 'decline-trusted')) {
$expectedUIAccess = $mode -eq 'accept'
$properties = if ($expectedUIAccess) { @('GAZEGRID_TRUST_CERTIFICATE=1') } else { @() }
Run-Msi '/i' $properties
try {
    $rootTrusted = Test-Path "Cert:\LocalMachine\Root\$CertificateThumbprint"
    if ($mode -eq 'default' -and $rootTrusted) { throw 'Default installation unexpectedly trusted the certificate.' }
    if ($mode -ne 'default' -and !$rootTrusted) { throw 'Accepted certificate trust is missing.' }
    # A repair with no consent property must preserve the original mode.
    Run-Msi '/fomus'

    $signature = Get-AuthenticodeSignature $installed
    if ($expectedUIAccess -and ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $CertificateThumbprint)) {
        throw 'Installed application signature did not match GazeGrid.'
    }
    if (!$expectedUIAccess -and $signature.Status -ne 'NotSigned') { throw 'Declining must install the unsigned executable.' }
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
    if (!$diagnostics.portal.tokenQuerySucceeded -or [bool]$diagnostics.portal.uiAccess -ne $expectedUIAccess) { throw "Unexpected UIAccess token in $mode installation." }
    $process = Start-Process $installed -ArgumentList '--smoke' -WorkingDirectory $env:TEMP -PassThru
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw 'Installed application timed out.' }
    if ($process.ExitCode -ne 0) { throw "Installed application failed: $($process.ExitCode)" }
} finally { Run-Msi '/x' }

}
