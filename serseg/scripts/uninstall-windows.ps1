param([string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'Programs\SERSEG'))
$ErrorActionPreference = 'Stop'
$InstallDir = [IO.Path]::GetFullPath($InstallDir)
$bin = Join-Path $InstallDir 'bin'
$receiptPath = Join-Path $InstallDir 'receipt.json'
foreach ($p in @($InstallDir, $bin, $receiptPath, (Join-Path $bin 'serseg.exe'), (Join-Path $InstallDir 'uninstall.ps1'))) {
    if (-not (Test-Path -LiteralPath $p)) { throw "Missing installation item: $p. Nothing removed." }
    if ((Get-Item -Force -LiteralPath $p).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing reparse point: $p. Nothing removed." }
}
$receipt = Get-Content -Raw -LiteralPath $receiptPath | ConvertFrom-Json
if ($receipt.version -ne 1 -or -not $receipt.compilerHash -or -not $receipt.uninstallHash) { throw 'Invalid receipt; nothing removed.' }
$compiler = Join-Path $bin 'serseg.exe'
$uninstaller = Join-Path $InstallDir 'uninstall.ps1'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $compiler).Hash -ne $receipt.compilerHash -or
    (Get-FileHash -Algorithm SHA256 -LiteralPath $uninstaller).Hash -ne $receipt.uninstallHash) {
    throw 'Installed file modified; nothing removed.'
}
$stdlib = Join-Path $InstallDir 'stdlib'
$math = Join-Path $stdlib 'math.ser'
if ($receipt.stdlibMathHash) {
    foreach ($p in @($stdlib, $math)) {
        if (-not (Test-Path -LiteralPath $p) -or ((Get-Item -Force -LiteralPath $p).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Missing or unsafe library item: $p. Nothing removed." }
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $math).Hash -ne $receipt.stdlibMathHash) { throw 'Installed library modified; nothing removed.' }
}
$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$parts = if ([string]::IsNullOrEmpty($userPath)) { @() } else { @($userPath -split ';') }
foreach ($dir in @($receipt.addedPath)) { $parts = @($parts | Where-Object { $_ -ine $dir }) }
[Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User')
$processParts = if ([string]::IsNullOrEmpty($env:Path)) { @() } else { @($env:Path -split ';') }
foreach ($dir in @($receipt.addedPath)) { $processParts = @($processParts | Where-Object { $_ -ine $dir }) }
$env:Path = $processParts -join ';'
if ($receipt.stdlibMathHash) {
    Remove-Item -LiteralPath $math
    if (@(Get-ChildItem -Force -LiteralPath $stdlib).Count -eq 0) { Remove-Item -LiteralPath $stdlib }
}
Remove-Item -LiteralPath $compiler, $uninstaller, $receiptPath
if (@(Get-ChildItem -Force -LiteralPath $bin).Count -eq 0) { Remove-Item -LiteralPath $bin }
if (@(Get-ChildItem -Force -LiteralPath $InstallDir).Count -eq 0) { Remove-Item -LiteralPath $InstallDir }
else { Write-Host "Additional files preserved in $InstallDir" }
Write-Host 'Removed recorded SERSEG files and PATH entries; user projects were not touched.'
