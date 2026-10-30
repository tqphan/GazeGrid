[CmdletBinding()]
param([string]$BuildDirectory = 'build/windows')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$project = Get-Content "$root/CMakeLists.txt" -Raw
if ($project -notmatch 'project\(gaze-grid\s+VERSION\s+(\d+\.\d+\.\d+)') {
    throw 'Cannot determine the gaze-grid release version.'
}
$version = $Matches[1]
$tag = "v$version"
if ($env:GITHUB_REF_TYPE -eq 'tag' -and $env:GITHUB_REF_NAME -ne $tag) {
    throw "Tag $env:GITHUB_REF_NAME does not match CMake version $tag."
}
$cache = Get-Content "$BuildDirectory/CMakeCache.txt" -Raw
if ($cache -notmatch '(?m)^GAZE_GRID_WITH_TOBII:BOOL=(ON|OFF)\r?$') { throw 'Cannot determine whether Tobii support was built.' }
$trackerNote = if ($Matches[1] -eq 'ON') { 'This build includes Tobii eye tracking and mouse dwell. Install the Tobii Experience software and drivers separately; no Tobii DLL is included.' }
else { 'This build supports mouse dwell; tracker support is not compiled in.' }
$msi = @(Get-ChildItem "$BuildDirectory/*.msi")
if ($msi.Count -ne 1) { throw 'Expected exactly one signed MSI.' }
$destination = Join-Path $BuildDirectory 'release'
New-Item -ItemType Directory -Force $destination | Out-Null
Copy-Item $msi[0].FullName "$destination/gaze-grid-$version-Windows-x64.msi"
Copy-Item "$BuildDirectory/gaze-grid.cer" "$destination/gaze-grid.cer"
$assets = @(Get-ChildItem "$destination/*.msi") + @(Get-ChildItem "$destination/*.cer")
$checksums = foreach ($asset in $assets) {
    $hash = (Get-FileHash -LiteralPath $asset.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($asset.Name)"
}
$checksums | Set-Content "$destination/SHA256SUMS.txt" -Encoding ascii
$thumbprint = (Get-PfxCertificate "$destination/gaze-grid.cer").Thumbprint
$changesPath = Join-Path $root "docs/releases/$version.md"
$changes = if (Test-Path $changesPath) { Get-Content $changesPath -Raw } else { "gaze-grid $version for Windows x64." }
@"
$changes

- The MSI is signed with the self-signed gaze-grid certificate.
- Setup selects Accept by default to trust the gaze-grid certificate for all users. Verify its thumbprint before continuing. Accept installs the certificate and signed UIAccess app under Program Files.
- Decline installs an unsigned app with UIAccess disabled: it cannot send input to higher-privilege windows or stay on top of all windows. Silent installations also default to Accept; pass GAZE_GRID_TRUST_CERTIFICATE=0 to decline.
- Certificate trust remains after uninstall. Administrators can revoke it using certlm.msc in Trusted Root Certification Authorities / Certificates.
- The completion screen offers Launch gaze-grid after setup, selected by default. Uncheck it to finish without opening the app. Launch uses the installed app's UIAccess-compatible Windows shell path after certificate trust is installed. Quiet installations and maintenance do not launch the app.
- Windows sign-in startup is enabled by default for trusted installations and uses the existing keyboard with prediction disabled. Uncheck it in setup or pass GAZE_GRID_SIGN_IN=0 for a silent installation to disable it. Unsigned installations keep it off, and repairs preserve your choice. Use Settings > Keyboard > Windows sign-in to copy your current keyboard and gaze preferences; administrator approval is required. Tobii availability before sign-in depends on its driver/service installation.
- Certificate thumbprint: $thumbprint
- SHA-256 checksums are in SHA256SUMS.txt.
- $trackerNote
- Tracker-enabled builds require the Tobii Experience software and drivers from https://gaming.tobii.com/getstarted/. The vendor installation supplies the Stream Engine DLL; gaze-grid does not bundle it.
- Virtual Xbox 360 gamepad output requires the separately installed ViGEmBus driver.
- Native hardware acceptance remains pending.

Source commit: $env:GITHUB_SHA
"@ | Set-Content "$destination/RELEASE-NOTES.md" -Encoding utf8
if ($env:GITHUB_OUTPUT) {
    "version=$version" | Out-File $env:GITHUB_OUTPUT -Append
    "tag=$tag" | Out-File $env:GITHUB_OUTPUT -Append
}
