<#
.SYNOPSIS
    Compila plugin + app, roda os testes e gera o zip da release em dist\ (so os arquivos da pasta plugins).
.DESCRIPTION
    Precisa do Visual Studio 2022 (ou Build Tools) com C++. Nao precisa baixar nenhum SDK.
    Uso:  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 [-SkipTests] [-GameTag ETS2-1.62]
    GameTag vai no nome do arquivo: a versao do jogo para a qual a release foi feita.
    A versao do plugin vem de src\version.h.
#>
param([switch]$SkipTests, [string]$GameTag = 'ETS2-1.62')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# Localiza o CMake que vem com o Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake -and (Test-Path $vswhere)) {
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cand = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (Test-Path $cand) { $cmake = $cand }
}
if (-not $cmake) { throw 'CMake nao encontrado. Instale o Visual Studio 2022 com "Desenvolvimento para desktop com C++".' }
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'

& $cmake -S $root -B "$root\build" -G 'Visual Studio 17 2022' -A x64 | Out-Host
if ($LASTEXITCODE) { throw 'Falha ao configurar' }
& $cmake --build "$root\build" --config Release | Out-Host
if ($LASTEXITCODE) { throw 'Falha ao compilar' }

if (-not $SkipTests) {
    & $ctest --test-dir "$root\build" -C Release --output-on-failure | Out-Host
    if ($LASTEXITCODE) { throw 'Testes falharam' }
}

# Versao a partir de src\version.h
$version = ([regex]::Match((Get-Content "$root\src\version.h" -Raw), 'AIRBRAKE_AUDIO_VERSION\s+"(\d+\.\d+\.\d+)')).Groups[1].Value # so a parte numerica (2.2.0-beta.1 -> 2.2.0)
if (-not $version) { throw 'Versao nao encontrada em src\version.h' }
$base = "AirbrakeAudio-v$version-$GameTag"
$rel = "$root\build\Release"
$dist = Join-Path $root 'dist'
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force $dist | Out-Null

# O que vai para a release: so o zip para extrair dentro de ...\bin\win_x64\plugins
# (o app fica junto da DLL: o atalho no jogo abre ele de la).
$stage = Join-Path $root "build\stage\$base"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force "$stage\airbrake_audio" | Out-Null
Copy-Item "$rel\airbrake_audio.dll", "$rel\AirbrakeAudio.exe", "$root\config\airbrake_audio.ini" $stage
Copy-Item "$root\assets\sounds\*.wav" "$stage\airbrake_audio\"
Copy-Item "$root\CREDITS.md", "$root\LICENSE", "$root\THIRD-PARTY-NOTICES.txt" $stage
Compress-Archive -Path "$stage\*" -DestinationPath (Join-Path $dist "$base-plugins.zip")

Write-Host "Release em $dist :"
Get-ChildItem $dist -File | ForEach-Object { Write-Host ("  {0,-48} {1,10:N0} bytes" -f $_.Name, $_.Length) }
