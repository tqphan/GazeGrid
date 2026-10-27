# Run only on an isolated Windows test machine: temporarily removes this test root.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Msi, [Parameter(Mandatory)][string]$CertificateThumbprint, [switch]$ExpectTobii)
$ErrorActionPreference = 'Stop'
$path = (Resolve-Path $Msi).Path
& "$PSScriptRoot/Assert-NoTobiiRuntime.ps1" -Msi $path
$installed = "$env:ProgramFiles/GazeGrid/gazegrid.exe"
function Run-Msi([string]$Operation, [string[]]$Properties = @()) {
    $process = Start-Process msiexec.exe -ArgumentList (@($Operation, "`"$path`"", '/qn', '/norestart', '/l*v', "`"$env:RUNNER_TEMP/gazegrid-msi-$mode-$($Operation.TrimStart('/')).log`"") + $Properties) -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) { throw "MSI failed: $($process.ExitCode)" }
}
Remove-Item "Cert:\LocalMachine\Root\$CertificateThumbprint" -Force
$accessibility = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Accessibility'
$atName = 'GazeGrid_GazeGrid_v1'
$originalStartup = @(((Get-ItemProperty $accessibility -Name Configuration -ErrorAction SilentlyContinue).Configuration -split ',') | Where-Object { $_ -and $_.Trim() -ne $atName })
foreach ($mode in @('decline', 'default', 'accept', 'sign-in', 'decline-trusted')) {
Write-Host "Checking installation mode: $mode"
$expectedUIAccess = $mode -in @('default', 'accept', 'sign-in')
[string[]]$properties = if ($mode -eq 'default') { @() } elseif ($expectedUIAccess) { @('GAZEGRID_TRUST_CERTIFICATE=1') } else { @('GAZEGRID_TRUST_CERTIFICATE=0') }
if ($mode -eq 'sign-in') { $properties += 'GAZEGRID_SIGN_IN=1' }
Run-Msi '/i' $properties
try {
    $rootTrusted = Test-Path "Cert:\LocalMachine\Root\$CertificateThumbprint"
    if ($mode -eq 'decline' -and $rootTrusted) { throw 'Declined installation unexpectedly trusted the certificate.' }
    if ($mode -ne 'decline' -and !$rootTrusted) { throw 'Accepted certificate trust is missing.' }
    # A repair with no consent property must preserve the original mode.
    Run-Msi '/fomus'
    $startup = @(((Get-ItemProperty $accessibility -Name Configuration -ErrorAction SilentlyContinue).Configuration -split ',') | Where-Object { $_ })
    if (($atName -in $startup) -ne ($mode -eq 'sign-in')) { throw 'Unexpected sign-in startup selection after repair.' }
    if (@(Compare-Object $originalStartup @($startup | Where-Object { $_.Trim() -ne $atName })).Count) { throw 'Other accessibility startup entries changed.' }
    $registration = Get-ItemProperty "$accessibility\ATs\$atName" -ErrorAction SilentlyContinue
    if ($expectedUIAccess -and (!$registration -or $registration.StartParams -ne '--sign-in' -or [IO.Path]::GetFullPath($registration.StartExe) -ne [IO.Path]::GetFullPath($installed))) { throw 'Trusted sign-in registration is missing or incorrect.' }
    if (!$expectedUIAccess -and $registration) { throw 'Unsigned installation must not register secure-desktop startup.' }

    $signature = Get-AuthenticodeSignature $installed
    if ($expectedUIAccess -and ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $CertificateThumbprint)) {
        throw 'Installed application signature did not match GazeGrid.'
    }
    if (!$expectedUIAccess -and $signature.Status -ne 'NotSigned') { throw 'Declining must install the unsigned executable.' }
    if ($mode -eq 'sign-in') {
        # The isolated CI runner is already elevated. Exercise the same helper
        # that Settings launches with an administrator prompt on a user's PC.
        $preferences = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes('{"startLayout":"main-14x1-top","keyboardScale":120,"gazeEnabled":false}'))
        $helper = Start-Process $installed -Verb Open -ArgumentList @('--configure-sign-in', 'enable', $preferences) -PassThru
        $null = $helper.Handle
        if (!$helper.WaitForExit(30000)) { $helper.Kill(); throw 'Sign-in configuration helper timed out.' }
        if ($helper.ExitCode -ne 0) { throw 'Could not copy the selected keyboard to sign-in preferences.' }
        $shared = Get-ItemProperty 'HKLM:\SOFTWARE\GazeGrid\SignIn' -Name Profile
        $profile = [Text.Encoding]::UTF8.GetString([byte[]]$shared.Profile) | ConvertFrom-Json
        if ($profile.startLayout -ne 'main-14x1-top' -or $profile.keyboardScale -ne 120 -or $profile.predictionMode -ne 'off' -or $profile.learning) { throw 'Unexpected sign-in keyboard preferences.' }
    }
    # Avoid inheriting Qt paths from the build environment: verify deployed plugins.
    Remove-Item Env:QT_PLUGIN_PATH -ErrorAction SilentlyContinue
    Remove-Item Env:QML2_IMPORT_PATH -ErrorAction SilentlyContinue
    Remove-Item Env:QML_IMPORT_PATH -ErrorAction SilentlyContinue
    $env:QT_LOGGING_TO_CONSOLE = '1'
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:GAZEGRID_CONFIG_DIR = "$env:RUNNER_TEMP/gazegrid-installed-test"
    if (!(Test-Path "$env:ProgramFiles/GazeGrid/keyboards/main-14x1-bottom.xml")) { throw 'Installed keyboard pack is missing.' }
    $report = "$env:RUNNER_TEMP/gazegrid-app-$mode-diagnose.json"
    # UIAccess must launch through ShellExecute, as it would from the Start menu.
    # Redirecting stdout switches Start-Process to CreateProcess (error 740).
    Remove-Item $report -ErrorAction SilentlyContinue
    $diagnose = Start-Process $installed -Verb Open -ArgumentList @('--diagnose', '--validate-keyboards', '--diagnostic-output', "`"$report`"") -WorkingDirectory $env:TEMP -PassThru
    $null = $diagnose.Handle
    if (!$diagnose.WaitForExit(30000)) { $diagnose.Kill(); throw 'Installed diagnostics timed out.' }
    if ($diagnose.ExitCode -ne 0) { throw 'Installed diagnostics failed.' }
    $diagnostics = Get-Content $report -Raw | ConvertFrom-Json
    if ($ExpectTobii -and !$diagnostics.tobii.compiled) { throw 'Installed application was built without Tobii support.' }
    if (@(Get-ChildItem "$env:ProgramFiles/GazeGrid" -Recurse -File -Filter '*tobii*.dll').Count) { throw 'Installed package includes a forbidden Tobii runtime DLL.' }
    if (!$diagnostics.portal.tokenQuerySucceeded -or [bool]$diagnostics.portal.uiAccess -ne $expectedUIAccess) { throw "Unexpected UIAccess token in $mode installation." }
    $process = Start-Process $installed -Verb Open -ArgumentList '--smoke' -WorkingDirectory $env:TEMP -PassThru
    $null = $process.Handle
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw 'Installed application timed out.' }
    if ($process.ExitCode -ne 0) { throw "Installed application failed: $($process.ExitCode)" }
} finally { Run-Msi '/x' }
$remainingStartup = @(((Get-ItemProperty $accessibility -Name Configuration -ErrorAction SilentlyContinue).Configuration -split ',') | Where-Object { $_ })
if ($atName -in $remainingStartup) { throw 'Sign-in startup remained after uninstall.' }
if (@(Compare-Object $originalStartup $remainingStartup).Count) { throw 'Uninstall changed another accessibility startup entry.' }
if (Test-Path "$accessibility\ATs\$atName") { throw 'Sign-in registration remained after uninstall.' }

}
