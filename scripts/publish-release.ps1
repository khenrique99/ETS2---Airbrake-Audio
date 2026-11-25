<#
.SYNOPSIS
  Cria a release no GitHub com o zip de dist\ (rode scripts\build.ps1 antes, na main).

.DESCRIPTION
  A tag segue  v<versao do plugin>+ets2.<versao do jogo>  (ex.: v2.0.0+ets2.1.62): a versao do plugin
  vem de src\version.h e a do jogo do parametro -Game. O zip tem de ter o mesmo -GameTag do build
  (ETS2-1.62 <-> ets2.1.62). A tag precisa existir na main (scripts\promote-to-main.ps1 -Version X.Y.Z cria
  a tag simples vX.Y.Z; este script cria a tag com o jogo).

.EXAMPLE
  ./scripts/publish-release.ps1 -Game 1.62 -NotesFile docs\release-notes.md
  ./scripts/publish-release.ps1 -Game 1.62 -NotesFile docs\release-notes.md -Prerelease
#>
param(
    [Parameter(Mandatory)] [string]$Game,
    [Parameter(Mandatory)] [string]$NotesFile,
    [switch]$Prerelease
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$version = ([regex]::Match((Get-Content "$root\src\version.h" -Raw), 'AIRBRAKE_AUDIO_VERSION\s+"(\d+\.\d+\.\d+)"')).Groups[1].Value
if (-not $version) { throw 'Versao nao encontrada em src\version.h' }
$tag = "v$version+ets2.$Game"
$zip = "dist\AirbrakeAudio-v$version-ETS2-$Game-plugins.zip"
if (-not (Test-Path $zip)) { throw "Zip nao encontrado: $zip. Rode scripts\build.ps1 -GameTag ETS2-$Game." }
if (-not (Test-Path $NotesFile)) { throw "Notas nao encontradas: $NotesFile" }
if ((git branch --show-current) -ne 'main') { throw 'Publique a release a partir da main.' }
if (git status --porcelain) { throw 'Working tree com alteracoes.' }

$args = @('release', 'create', $tag, $zip, '--title', "Airbrake Audio v$version - ETS2 $Game.x", '--notes-file', $NotesFile, '--target', 'main')
if ($Prerelease) { $args += '--prerelease' }
gh @args
if ($LASTEXITCODE -ne 0) { throw 'gh release create falhou' }
Write-Host "Release $tag criada."
