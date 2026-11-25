<#
.SYNOPSIS
  Promove a branch dev para a main SEM levar os arquivos do Claude.

.DESCRIPTION
  CLAUDE.md e a pasta .claude/ existem somente na dev. Este script faz o merge
  dev -> main, remove esses arquivos do resultado e cria o commit de merge.
  Com -Version cria a tag v<versao>+ets2.<jogo> (mesmo padrao do Trailer Control; sem tag simples vX.Y.Z).

.EXAMPLE
  ./scripts/promote-to-main.ps1 -Version 2.0.0 -Game 1.62
#>
param(
    [string]$Version,
    [string]$Game,
    [switch]$Push
)

$ErrorActionPreference = 'Stop'
if ($Version -and -not $Game) { throw "Informe -Game (versao do jogo, ex.: 1.62) junto com -Version." }
$ClaudePaths = @('CLAUDE.md', '.claude')

function Invoke-Git {
    git @args
    if ($LASTEXITCODE -ne 0) { throw "git $($args -join ' ') falhou" }
}

if (git status --porcelain) { throw 'Working tree com alteracoes. Commit ou stash antes.' }

Invoke-Git fetch origin
Invoke-Git checkout main
Invoke-Git pull --ff-only origin main

git merge --no-ff --no-commit origin/dev
if ($LASTEXITCODE -ne 0) {
    Write-Warning 'Conflitos no merge. Resolva, remova os arquivos do Claude e faca o commit manualmente.'
    exit 1
}

foreach ($p in $ClaudePaths) {
    if (Test-Path $p) { Invoke-Git rm -r -q --cached --ignore-unmatch -- $p; Remove-Item -Recurse -Force $p }
}

$tagName = if ($Version) { "v$Version+ets2.$Game" }
$msg = if ($Version) { "chore(release): $tagName" } else { 'chore: merge dev into main' }
Invoke-Git commit -m $msg

if ($Version) { Invoke-Git tag -a $tagName -m $tagName }

if ($Push) {
    Invoke-Git push origin main
    if ($Version) { Invoke-Git push origin $tagName }
}

Invoke-Git checkout dev
Write-Host 'main atualizada (sem arquivos do Claude). Voltou para a dev.'
