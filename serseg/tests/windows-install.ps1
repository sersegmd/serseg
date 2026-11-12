param([Parameter(Mandatory=$true)][string]$InstallDir, [Parameter(Mandatory=$true)][string]$GccBin)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'This test modifies the user PATH and is restricted to an isolated GitHub Actions runner.' }
$installer = Join-Path (Split-Path -Parent $PSScriptRoot) 'scripts\install-windows.ps1'
$before = [Environment]::GetEnvironmentVariable('Path', 'User')
try {
    # Start with GCC already present so the test ensures the uninstaller preserves it.
    $testPath = if ([string]::IsNullOrEmpty($before)) { $GccBin } else { "$before;$GccBin" }
    [Environment]::SetEnvironmentVariable('Path', $testPath, 'User')
    & $installer -InstallDir $InstallDir -GccBin $GccBin
    $exe = Join-Path $InstallDir 'bin\serseg.exe'
    $uninstaller = Join-Path $InstallDir 'uninstall.ps1'
    if (-not (Test-Path -LiteralPath $exe)) { throw 'Compiler missing after installation.' }
    if (-not (Test-Path -LiteralPath (Join-Path $InstallDir 'stdlib\math.ser'))) { throw 'Standard math module missing after installation.' }
    $pathAfter = [Environment]::GetEnvironmentVariable('Path', 'User')
    if (@($pathAfter -split ';') -notcontains (Join-Path $InstallDir 'bin')) { throw 'Installed bin missing from user PATH.' }
    $refused = $false
    try { & $installer -InstallDir $InstallDir -GccBin $GccBin } catch { $refused = $true }
    if (-not $refused) { throw 'Existing installation was not rejected.' }
    $original = [IO.File]::ReadAllBytes($exe)
    $stream = [IO.File]::Open($exe, [IO.FileMode]::Append)
    try { $stream.WriteByte(0) } finally { $stream.Dispose() }
    $refused = $false
    try { & $uninstaller -InstallDir $InstallDir } catch { $refused = $true }
    if (-not $refused -or -not (Test-Path -LiteralPath $exe)) { throw 'Changed file was not protected.' }
    [IO.File]::WriteAllBytes($exe, $original)
    Set-Content -LiteralPath (Join-Path $InstallDir 'personal.txt') -Value 'preserve'
    & $uninstaller -InstallDir $InstallDir
    if (Test-Path -LiteralPath $exe) { throw 'Compiler remains after uninstall.' }
    if (-not (Test-Path -LiteralPath (Join-Path $InstallDir 'personal.txt'))) { throw 'Extra file was removed.' }
    if ([Environment]::GetEnvironmentVariable('Path', 'User') -ne $testPath) { throw 'User PATH was not restored as expected.' }
    Write-Host 'PASS Windows install, PATH, modified-file protection and uninstall'
} finally {
    [Environment]::SetEnvironmentVariable('Path', $before, 'User')
}
