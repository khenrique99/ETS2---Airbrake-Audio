<#
.SYNOPSIS
  Cria a release no GitHub com o zip de dist\ (rode scripts\build.ps1 antes, na main).

.DESCRIPTION
  A tag segue  v<versao do plugin>+ets2.<versao do jogo>  (ex.: v2.0.0+ets2.1.62): a versao do plugin
  vem de src\version.h e a do jogo do parametro -Game. O zip tem de ter o mesmo -GameTag do build
  (ETS2-1.62 <-> ets2.1.62). A tag precisa existir na main (scripts\promote-to-main.ps1 -Version X.Y.Z cria
  -Version X.Y.Z -Game 1.62 (e enviada com -Push); este script so cria a release em cima dela.

.EXAMPLE
  ./scripts/publish-release.ps1 -Game 1.62 -NotesFile docs\release-notes.md
  ./scripts/publish-release.ps1 -Game 1.62 -NotesFile docs\release-notes.md -Prerelease
#>
param(
    [Parameter(Mandatory)] [string]$Game,
    [Parameter(Mandatory)] [string]$NotesFile,
    [switch]$Prerelease,
    # Branch de onde sai a release (main para estavel; dev para beta, como no Trailer Control)
    [string]$Branch = 'main',
    # Sufixo do zip igual ao -GameTag do build (ex.: -GameTag ETS2-1.62-beta -> -ZipSuffix -beta)
    [string]$ZipSuffix = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# Versao completa (pode ter sufixo de beta: 2.2.0-beta.1) para a tag; so a parte numerica no nome do zip.
$full = ([regex]::Match((Get-Content "$root\src\version.h" -Raw), 'AIRBRAKE_AUDIO_VERSION\s+"([^"]+)"')).Groups[1].Value
if (-not $full) { throw 'Versao nao encontrada em src\version.h' }
$version = ([regex]::Match($full, '^\d+\.\d+\.\d+')).Value
$tag = "v$full+ets2.$Game"
$zip = "dist\AirbrakeAudio-v$version-ETS2-$Game$ZipSuffix-plugins.zip"
if (-not (Test-Path $zip)) { throw "Zip nao encontrado: $zip. Rode scripts\build.ps1 -GameTag ETS2-$Game." }
if (-not (Test-Path $NotesFile)) { throw "Notas nao encontradas: $NotesFile" }
if ((git branch --show-current) -ne $Branch) { throw "Publique a release a partir da $Branch." }
if (git status --porcelain) { throw 'Working tree com alteracoes.' }

$args = @('release', 'create', $tag, $zip, '--title', ("Airbrake Audio v$full - ETS2 $Game.x" + $(if ($Prerelease) { ' (BETA de teste)' } else { '' })), '--notes-file', $NotesFile, '--target', $Branch)
if ($Prerelease) { $args += '--prerelease' }
gh @args
if ($LASTEXITCODE -ne 0) { throw 'gh release create falhou' }
Write-Host "Release $tag criada."
