[CmdletBinding()]
param([Parameter(Mandatory)][string]$Msi)
$ErrorActionPreference = 'Stop'
# Inspect the packaged MSI File table, rather than just the deployment directory.
$installer = $database = $view = $record = $null
try {
    $installer = New-Object -ComObject WindowsInstaller.Installer
    $database = $installer.OpenDatabase((Resolve-Path -LiteralPath $Msi).Path, 0)
    $view = $database.OpenView('SELECT `FileName` FROM `File`')
    $view.Execute()
    while ($record = $view.Fetch()) {
        try {
            $name = ($record.StringData(1) -split '\|')[-1]
            if ($name -like '*tobii*.dll') { throw "Forbidden Tobii runtime in MSI: $name" }
        } finally {
            [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($record)
            $record = $null
        }
    }
} finally {
    if ($view) { $view.Close(); [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($view) }
    if ($database) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($database) }
    if ($installer) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($installer) }
}
Write-Host 'Verified MSI contains no Tobii DLLs.'
