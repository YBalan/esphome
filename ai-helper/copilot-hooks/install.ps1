<#
Installs the AI Helper GitHub Copilot hooks at USER level (Copilot CLI + the Copilot desktop app).

  powershell -NoProfile -ExecutionPolicy Bypass -File install.ps1           install / update
  powershell -NoProfile -ExecutionPolicy Bypass -File install.ps1 -Check    report differences only (exit 1 = not in sync)

What it does:
  1. copies ai-helper.json to <CopilotDir>\hooks\ai-helper.json (Copilot loads every *.json in that directory)
  2. copies ai-helper-status-sync.ps1 from ..\claude-hooks\ (the one shared script) and this folder's README
     to <CopilotDir>\hooks\ai-helper\ (a subdirectory; only top-level *.json files are read as hook configs)
  3. whatever it changes is first copied to <CopilotDir>\backups\ai-helper-install-<time>\
<CopilotDir> is $env:COPILOT_HOME when set, otherwise ~\.copilot (where Copilot looks for user-level hooks).
Running it again when nothing differs changes nothing. The usage-sync script is NOT installed: it reports the
Claude plan and does nothing in a Copilot session anyway.
Works on Windows PowerShell 5.1 (what a fresh Windows has). Keep this file ASCII.
#>
param(
  [string]$CopilotDir = $(if ($env:COPILOT_HOME) { $env:COPILOT_HOME } else { Join-Path $env:USERPROFILE '.copilot' }),
  [switch]$Check
)

$ErrorActionPreference = 'Stop'
$src         = $PSScriptRoot
$claudeHooks = Join-Path (Split-Path -Parent $src) 'claude-hooks'
$hooksDir    = Join-Path $CopilotDir 'hooks'
$scriptDir   = Join-Path $hooksDir 'ai-helper'
$backupDir   = Join-Path $CopilotDir ('backups\ai-helper-install-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$todo        = New-Object System.Collections.Generic.List[string]

function Read-Text([string]$path) { ([IO.File]::ReadAllText($path)) -replace "`r`n", "`n" }

# never install a hook definition that does not parse
$definition = Join-Path $src 'ai-helper.json'
$parsed = (Get-Content -Raw -Encoding UTF8 $definition) | ConvertFrom-Json
if ($parsed.version -ne 1 -or -not $parsed.hooks) { throw 'ai-helper.json must have version 1 and a "hooks" block' }

$map = @(
  @{ from = $definition;                                           to = (Join-Path $hooksDir 'ai-helper.json');             rel = 'hooks\ai-helper.json' },
  @{ from = (Join-Path $claudeHooks 'ai-helper-status-sync.ps1');  to = (Join-Path $scriptDir 'ai-helper-status-sync.ps1'); rel = 'hooks\ai-helper\ai-helper-status-sync.ps1' },
  @{ from = (Join-Path $src 'README.md');                          to = (Join-Path $scriptDir 'README.md');                 rel = 'hooks\ai-helper\README.md' }
)

$changes = @()
foreach ($m in $map) {
  if (-not (Test-Path $m.from)) { throw "Missing source file: $($m.from)" }
  if (-not (Test-Path $m.to)) { $todo.Add("add     $($m.rel)"); $changes += , $m }
  elseif ((Read-Text $m.from).TrimEnd() -cne (Read-Text $m.to).TrimEnd()) { $todo.Add("update  $($m.rel)"); $changes += , $m }
}

if ($todo.Count -eq 0) { Write-Host "Already in sync: $CopilotDir"; exit 0 }
if ($Check) { $todo | ForEach-Object { Write-Host "differs: $_" }; exit 1 }

foreach ($m in $changes) {
  if (Test-Path $m.to) {
    $backup = Join-Path $backupDir $m.rel
    New-Item -ItemType Directory -Force -Path (Split-Path $backup) | Out-Null
    Copy-Item $m.to $backup -Force
  }
  New-Item -ItemType Directory -Force -Path (Split-Path $m.to) | Out-Null
  Copy-Item $m.from $m.to -Force
}

$todo | ForEach-Object { Write-Host "done: $_" }
if (Test-Path $backupDir) { Write-Host "previous versions: $backupDir" }
Write-Host 'Restart Copilot sessions: hooks are read when a session starts.'
exit 0
