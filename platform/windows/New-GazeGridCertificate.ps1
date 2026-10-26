[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][securestring]$Password
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
if ((Test-Path "$OutputDirectory/GazeGrid.pfx") -or (Test-Path "$OutputDirectory/GazeGrid.cer")) {
    throw 'Certificate output already exists; preserve the existing signing identity.'
}
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=GazeGrid' `
    -FriendlyName 'GazeGrid' -CertStoreLocation Cert:\CurrentUser\My `
    -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 `
    -KeyExportPolicy Exportable -NotAfter (Get-Date).AddYears(3)
Export-PfxCertificate -Cert $cert -FilePath "$OutputDirectory/GazeGrid.pfx" -Password $Password | Out-Null
Export-Certificate -Cert $cert -FilePath "$OutputDirectory/GazeGrid.cer" | Out-Null
Write-Output "GazeGrid signing thumbprint: $($cert.Thumbprint)"
