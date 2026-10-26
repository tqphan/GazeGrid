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

- The MSI and application executable are signed with the self-signed GazeGrid certificate.
- Verify and trust the public GazeGrid.cer certificate in Local Computer / Trusted Root Certification Authorities, then install the MSI under Program Files for UIAccess.
- Certificate thumbprint: $thumbprint
- SHA-256 checksums are in SHA256SUMS.txt.
- This build supports mouse dwell; the proprietary Tobii SDK is not included.
- Virtual Xbox 360 gamepad output requires the separately installed ViGEmBus driver.
- Native hardware acceptance remains pending.

Source commit: $env:GITHUB_SHA
"@ | Set-Content "$destination/RELEASE-NOTES.md" -Encoding utf8
if ($env:GITHUB_OUTPUT) {
    "version=$version" | Out-File $env:GITHUB_OUTPUT -Append
    "tag=$tag" | Out-File $env:GITHUB_OUTPUT -Append
}
