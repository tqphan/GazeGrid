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
$changesPath = Join-Path $root "docs/releases/$version.md"
$changes = if (Test-Path $changesPath) { Get-Content $changesPath -Raw } else { "gaze-grid $version for Windows x64." }
@"
$changes

Source commit: $env:GITHUB_SHA
"@ | Set-Content "$destination/RELEASE-NOTES.md" -Encoding utf8
if ($env:GITHUB_OUTPUT) {
    "version=$version" | Out-File $env:GITHUB_OUTPUT -Append
    "tag=$tag" | Out-File $env:GITHUB_OUTPUT -Append
}
