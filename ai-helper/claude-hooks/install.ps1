<#
Installs the AI Helper Claude Code hooks at USER level (every repo + the desktop app's Code tab).

  powershell -NoProfile -ExecutionPolicy Bypass -File install.ps1           install / update
  powershell -NoProfile -ExecutionPolicy Bypass -File install.ps1 -Check    report differences only (exit 1 = not in sync)

What it does:
  1. copies the scripts + README that sit next to this file to <ClaudeDir>\hooks\ai-helper\
  2. merges the "hooks" block of settings-hooks.json into <ClaudeDir>\settings.json. Hook groups whose command
     runs one of these scripts (the path contains "ai-helper/ai-helper-") are replaced; every other hook and
     every other setting is kept. A missing settings.json is created from settings-hooks.json as it is.
  3. whatever it changes is first copied to <ClaudeDir>\backups\ai-helper-install-<time>\
Running it again when nothing differs changes nothing. An unreadable settings.json is never touched.
Works on Windows PowerShell 5.1 (what a fresh Windows has, and what the hook commands run). Keep this file ASCII.
#>
param(
  [string]$ClaudeDir = (Join-Path $env:USERPROFILE '.claude'),
  [switch]$Check
)

$ErrorActionPreference = 'Stop'
$src          = $PSScriptRoot
$hooksDir     = Join-Path $ClaudeDir 'hooks\ai-helper'
$settingsPath = Join-Path $ClaudeDir 'settings.json'
$snippetPath  = Join-Path $src 'settings-hooks.json'
$files        = 'ai-helper-status-sync.ps1', 'ai-helper-usage-sync.ps1', 'README.md'
$marker       = 'ai-helper/ai-helper-'
$backupDir    = Join-Path $ClaudeDir ('backups\ai-helper-install-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$utf8NoBom    = New-Object System.Text.UTF8Encoding($false)
$todo         = New-Object System.Collections.Generic.List[string]

function Read-Text([string]$path) { ([IO.File]::ReadAllText($path)) -replace "`r`n", "`n" }

function Backup([string]$path, [string]$relative) {
  if (-not (Test-Path $path)) { return }
  $to = Join-Path $backupDir $relative
  New-Item -ItemType Directory -Force -Path (Split-Path $to) | Out-Null
  Copy-Item $path $to -Force
}

function Test-Owned($group) {
  foreach ($h in @($group.hooks)) { if ([string]$h.command -like "*$marker*") { return $true } }
  return $false
}

# Windows PowerShell 5.1 writes ' & < > as ' & < >: valid JSON but unreadable. Undo that.
function ConvertTo-ReadableJson($object) {
  $json = $object | ConvertTo-Json -Depth 20
  $json = [regex]::Replace($json, '(?<!\\)\\u0027', "'")
  $json = [regex]::Replace($json, '(?<!\\)\\u0026', '&')
  $json = [regex]::Replace($json, '(?<!\\)\\u003c', '<')
  [regex]::Replace($json, '(?<!\\)\\u003e', '>')
}

# ---- 1. scripts + README -------------------------------------------------------------------------------------
$copy = @()
foreach ($f in $files) {
  $from = Join-Path $src $f
  $to   = Join-Path $hooksDir $f
  if (-not (Test-Path $from)) { throw "Missing next to install.ps1: $f" }
  if (-not (Test-Path $to)) { $todo.Add("add     hooks\ai-helper\$f"); $copy += $f }
  elseif ((Read-Text $from).TrimEnd() -cne (Read-Text $to).TrimEnd()) { $todo.Add("update  hooks\ai-helper\$f"); $copy += $f }
}

# ---- 2. settings.json ----------------------------------------------------------------------------------------
$snippet = (Get-Content -Raw -Encoding UTF8 $snippetPath) | ConvertFrom-Json
if (-not $snippet.hooks) { throw "settings-hooks.json has no ""hooks"" block" }

$createSettings = -not (Test-Path $settingsPath)
$settingsText   = $null
if ($createSettings) {
  $settingsText = (Get-Content -Raw -Encoding UTF8 $snippetPath)
  $todo.Add('add     settings.json (new file, hooks only)')
} else {
  try { $current = (Get-Content -Raw -Encoding UTF8 $settingsPath) | ConvertFrom-Json }
  catch { throw "$settingsPath is not valid JSON, not touching it: $($_.Exception.Message)" }
  if ($null -eq $current) { $current = New-Object psobject }

  $before = $current | ConvertTo-Json -Depth 20 -Compress
  $merged = $before | ConvertFrom-Json                       # private copy to edit
  if (-not $merged.PSObject.Properties['hooks'] -or $null -eq $merged.hooks) {
    $merged | Add-Member -Force -NotePropertyName hooks -NotePropertyValue (New-Object psobject)
  }

  # replace our groups in place (an event keeps its position), keep everything that is not ours
  foreach ($name in @($merged.hooks.PSObject.Properties | ForEach-Object { $_.Name })) {
    $kept = @(@($merged.hooks.$name) | Where-Object { -not (Test-Owned $_) })
    if ($snippet.hooks.PSObject.Properties[$name]) { $merged.hooks.$name = [object[]]($kept + @($snippet.hooks.$name)) }
    elseif ($kept.Count -eq 0) { $merged.hooks.PSObject.Properties.Remove($name) }
    else { $merged.hooks.$name = [object[]]$kept }
  }
  foreach ($p in $snippet.hooks.PSObject.Properties) {
    if (-not $merged.hooks.PSObject.Properties[$p.Name]) {
      Add-Member -InputObject $merged.hooks -NotePropertyName $p.Name -NotePropertyValue ([object[]]@($p.Value))
    }
  }

  if (($merged | ConvertTo-Json -Depth 20 -Compress) -cne $before) {
    $todo.Add('update  settings.json (hooks block)')
    $settingsText = ConvertTo-ReadableJson $merged
  }
}

# ---- report / apply ------------------------------------------------------------------------------------------
if ($todo.Count -eq 0) { Write-Host "Already in sync: $ClaudeDir"; exit 0 }
if ($Check) { $todo | ForEach-Object { Write-Host "differs: $_" }; exit 1 }

foreach ($f in $copy) { Backup (Join-Path $hooksDir $f) "hooks\ai-helper\$f" }
if ($settingsText) { Backup $settingsPath 'settings.json' }

New-Item -ItemType Directory -Force -Path $hooksDir | Out-Null
foreach ($f in $copy) { Copy-Item (Join-Path $src $f) (Join-Path $hooksDir $f) -Force }

if ($settingsText) {
  New-Item -ItemType Directory -Force -Path $ClaudeDir | Out-Null
  [IO.File]::WriteAllText($settingsPath, $settingsText, $utf8NoBom)
  # read it back: every event of the snippet must now have one of our groups, otherwise put the old file back
  $ok = $true
  try {
    $written = (Get-Content -Raw -Encoding UTF8 $settingsPath) | ConvertFrom-Json
    foreach ($p in $snippet.hooks.PSObject.Properties) {
      $mine = @(@($written.hooks.($p.Name)) | Where-Object { $_ -and (Test-Owned $_) })
      if ($mine.Count -ne @($p.Value).Count) { $ok = $false }
    }
  } catch { $ok = $false }
  if (-not $ok) {
    $old = Join-Path $backupDir 'settings.json'
    if (Test-Path $old) { Copy-Item $old $settingsPath -Force }
    throw 'settings.json did not read back correctly; the previous file was restored.'
  }
}

$todo | ForEach-Object { Write-Host "done: $_" }
if (Test-Path $backupDir) { Write-Host "previous versions: $backupDir" }
Write-Host 'Restart Claude Code sessions: hooks are read when a session starts.'
exit 0
