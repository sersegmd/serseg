$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path -Parent $PSScriptRoot) 'editors\vscode'
if (-not (Test-Path -LiteralPath (Join-Path $source 'package.json') -PathType Leaf)) { throw 'SERSEG extension source not found.' }
if (-not $env:USERPROFILE) { throw 'USERPROFILE is not set.' }
$root = Join-Path $env:USERPROFILE '.vscode\extensions'
$target = Join-Path $root 'serseg.serseg-language-0.1.0'
if (Test-Path -LiteralPath $target) { throw "Extension directory already exists; nothing overwritten: $target" }
New-Item -ItemType Directory -Path $root -Force | Out-Null
Copy-Item -LiteralPath $source -Destination $target -Recurse
Write-Host "Installed SERSEG editor extension: $target"
Write-Host 'Restart VS Code. Diagnostics require serseg on PATH. LSP and formatting are not included.'
