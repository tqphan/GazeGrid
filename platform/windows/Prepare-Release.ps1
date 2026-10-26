[CmdletBinding()]
param([string]$BuildDirectory = 'build/windows')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$project = Get-Content "$root/CMakeLists.txt" -Raw
if ($project -notmatch 'project\(GazeGrid\s+VERSION\s+(\d+\.\d+\.\d+)') {
    throw 'Cannot determine the GazeGrid release version.'
}
$version = $Matches[1]
$tag = "v$version"
if ($env:GITHUB_REF_TYPE -eq 'tag' -and $env:GITHUB_REF_NAME -ne $tag) {
    throw "Tag $env:GITHUB_REF_NAME does not match CMake version $tag."
}
$msi = @(Get-ChildItem "$BuildDirectory/*.msi")
if ($msi.Count -ne 1) { throw 'Expected exactly one signed MSI.' }
$destination = Join-Path $BuildDirectory 'release'
New-Item -ItemType Directory -Force $destination | Out-Null
Copy-Item $msi[0].FullName "$destination/GazeGrid-$version-Windows-x64.msi"
Copy-Item "$BuildDirectory/GazeGrid.cer" "$destination/GazeGrid.cer"
$assets = @(Get-ChildItem "$destination/*.msi") + @(Get-ChildItem "$destination/*.cer")
$checksums = foreach ($asset in $assets) {
    $hash = (Get-FileHash -LiteralPath $asset.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($asset.Name)"
}
$checksums | Set-Content "$destination/SHA256SUMS.txt" -Encoding ascii
$thumbprint = (Get-PfxCertificate "$destination/GazeGrid.cer").Thumbprint
@"
GazeGrid $version for Windows x64.

- The MSI is signed with the self-signed GazeGrid certificate.
- Setup asks for consent to trust the GazeGrid certificate for all users. Verify its thumbprint before accepting. Accept installs the certificate and signed UIAccess app under Program Files.
- Decline (the default) installs an unsigned app with UIAccess disabled: it cannot send input to higher-privilege windows or stay on top of all windows.
- Certificate trust remains after uninstall. Administrators can revoke it using certlm.msc in Trusted Root Certification Authorities / Certificates.
- Certificate thumbprint: $thumbprint
- SHA-256 checksums are in SHA256SUMS.txt.
- This build supports mouse dwell; tracker support is not compiled in.
- Tracker-enabled builds require the Tobii Gaming software and drivers from https://gaming.tobii.com/getstarted/. The vendor installation supplies the Stream Engine DLL; GazeGrid does not bundle it.
- Virtual Xbox 360 gamepad output requires the separately installed ViGEmBus driver.
- Native hardware acceptance remains pending.

Source commit: $env:GITHUB_SHA
"@ | Set-Content "$destination/RELEASE-NOTES.md" -Encoding utf8
if ($env:GITHUB_OUTPUT) {
    "version=$version" | Out-File $env:GITHUB_OUTPUT -Append
    "tag=$tag" | Out-File $env:GITHUB_OUTPUT -Append
}
