param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'Programs\SERSEG'),
    [string]$GccBin = 'C:\msys64\ucrt64\bin'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $root 'build\serseg.exe'
$bin = Join-Path $InstallDir 'bin'
$stdlibMath = Join-Path $root 'stdlib\math.ser'
if (-not (Test-Path -LiteralPath $stdlibMath -PathType Leaf)) { throw 'stdlib\math.ser not found in source package.' }
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw 'build\serseg.exe not found. Build with make in MSYS2 UCRT64 first.' }
if (-not (Test-Path -LiteralPath (Join-Path $GccBin 'gcc.exe') -PathType Leaf)) { throw "gcc.exe not found in $GccBin. Install MSYS2 UCRT64 GCC or provide -GccBin." }
if (Test-Path -LiteralPath $InstallDir) { throw "Installation directory already exists; nothing overwritten: $InstallDir" }
$InstallDir = [IO.Path]::GetFullPath($InstallDir)
$GccBin = [IO.Path]::GetFullPath($GccBin)
$bin = Join-Path $InstallDir 'bin'
$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$parts = if ([string]::IsNullOrEmpty($userPath)) { @() } else { @($userPath -split ';') }
$added = @()
foreach ($dir in @($bin, $GccBin)) {
    if ($parts -notcontains $dir) { $parts += $dir; $added += $dir }
}
New-Item -ItemType Directory -Path (Split-Path -Parent $InstallDir) -Force | Out-Null
New-Item -ItemType Directory -Path $InstallDir -ErrorAction Stop | Out-Null
New-Item -ItemType Directory -Path $bin -ErrorAction Stop | Out-Null
New-Item -ItemType Directory -Path (Join-Path $InstallDir 'stdlib') -ErrorAction Stop | Out-Null
[IO.File]::Copy($compiler, (Join-Path $bin 'serseg.exe'), $false)
[IO.File]::Copy($stdlibMath, (Join-Path $InstallDir 'stdlib\math.ser'), $false)
[IO.File]::Copy((Join-Path $PSScriptRoot 'uninstall-windows.ps1'), (Join-Path $InstallDir 'uninstall.ps1'), $false)
$receipt = [ordered]@{
    version = 1
    compilerHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $bin 'serseg.exe')).Hash
    stdlibMathHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $InstallDir 'stdlib\math.ser')).Hash
    uninstallHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $InstallDir 'uninstall.ps1')).Hash
    addedPath = @($added)
}
$receipt | ConvertTo-Json | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $InstallDir 'receipt.json')
[Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User')
$processParts = if ([string]::IsNullOrEmpty($env:Path)) { @() } else { @($env:Path -split ';') }
foreach ($dir in @($bin, $GccBin)) { if ($processParts -notcontains $dir) { $processParts += $dir } }
$env:Path = $processParts -join ';'
Write-Host "Installed compiler: $bin\serseg.exe"
& (Join-Path $bin 'serseg.exe') --version
if ($LASTEXITCODE -ne 0) { throw "Compiler verification failed (exit $LASTEXITCODE). Files and receipt remain for diagnosis or uninstall." }
$resolved = Get-Command serseg.exe -ErrorAction SilentlyContinue
if ($resolved -and $resolved.Source -ine (Join-Path $bin 'serseg.exe')) { Write-Warning "Another serseg.exe is first on PATH: $($resolved.Source)" }
Write-Host 'Open a new PowerShell window and run: serseg --version'
Write-Host "Uninstall with: & '$InstallDir\uninstall.ps1' -InstallDir '$InstallDir'"
