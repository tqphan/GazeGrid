[CmdletBinding()]
param([Parameter(Mandatory)][string]$Msi)
$ErrorActionPreference = 'Stop'
# Inspect the packaged MSI File table, rather than just the deployment directory.
$installer = New-Object -ComObject WindowsInstaller.Installer
$database = $installer.OpenDatabase((Resolve-Path -LiteralPath $Msi).Path, 0)
$view = $database.OpenView('SELECT `FileName` FROM `File`')
try {
    $view.Execute()
    while ($record = $view.Fetch()) {
        $name = ($record.StringData(1) -split '\|')[-1]
        if ($name -like '*tobii*.dll') { throw "Forbidden Tobii runtime in MSI: $name" }
    }
} finally { $view.Close() }
Write-Host 'Verified MSI contains no Tobii DLLs.'
