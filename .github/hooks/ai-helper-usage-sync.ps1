# Pushes the real Claude plan "usage remaining" percentage to the AI Helper.
#
# The live plan figure the Claude Code UI shows ("N% used, resets in ...") is NOT
# exposed to hooks, persisted to disk, or available via any CLI. It only exists
# in Anthropic's `anthropic-ratelimit-unified-*` response headers. So this hook
# reproduces exactly what the UI does: it makes one minimal authenticated request
# with the Claude Code OAuth token from ~/.claude/.credentials.json and reads the
# rate-limit headers off the response.
#
#   utilization = anthropic-ratelimit-unified-<Window>-utilization   (0.0 - 1.0+)
#   remaining%  = round(100 * (1 - utilization))                     (clamped 0..100)
#
# then POSTs it to the device's "Usage Remaining" number entity. `-Window 5h`
# mirrors the UI's "Current session" bar; `-Window 7d` is the weekly window.
#
# Everything is wrapped so any failure (offline device, expired token, network)
# is swallowed - status sync must never block or fail an agent turn. On failure
# the gauge is simply left at its previous value rather than showing a wrong one.
#
# `-Override <percent>` skips the ping and posts a fixed value (manual/testing).

param(
  [string]$DeviceIp = "172.16.1.15",
  [string]$EntityName = "AI Helper Usage Remaining",
  [string]$SourceEntityName = "AI Helper Usage Source",
  [string]$Source = "Claude Code",
  [ValidateSet("5h", "7d")]
  [string]$Window = "5h",
  [string]$CredentialsPath = "$env:USERPROFILE\.claude\.credentials.json",
  [string]$Model = "claude-haiku-4-5-20251001",
  [double]$Override = -1
)

$ErrorActionPreference = "Stop"
# Windows PowerShell 5.1 defaults can negotiate < TLS 1.2; api.anthropic.com needs 1.2+.
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

function Get-HeaderValue($response, $name) {
  if ($null -eq $response) { return $null }
  foreach ($key in $response.Headers.Keys) {
    if ($key -ieq $name) {
      $v = $response.Headers[$key]
      if ($v -is [array]) { return ($v -join ',') }
      return $v
    }
  }
  return $null
}

function Get-Utilization {
  if (-not (Test-Path $CredentialsPath)) { return $null }

  $token = $null
  try {
    $cred = Get-Content -Path $CredentialsPath -Raw | ConvertFrom-Json
    $token = $cred.claudeAiOauth.accessToken
  } catch {
    return $null
  }
  if ([string]::IsNullOrWhiteSpace($token)) { return $null }

  $headers = @{
    "authorization"     = "Bearer $token"
    "anthropic-version" = "2023-06-01"
    # OAuth (subscription) tokens require this beta flag and the Claude Code
    # identity system prompt below, otherwise the request is rejected.
    "anthropic-beta"    = "oauth-2025-04-20"
    "content-type"      = "application/json"
  }
  $body = @{
    model      = $Model
    max_tokens = 1
    system     = "You are Claude Code, Anthropic's official CLI for Claude."
    messages   = @(@{ role = "user"; content = "." })
  } | ConvertTo-Json -Depth 6

  $response = $null
  try {
    $response = Invoke-WebRequest -Uri "https://api.anthropic.com/v1/messages" `
      -Method Post -Headers $headers -Body $body -TimeoutSec 20 -UseBasicParsing
  } catch {
    # Rate-limit headers are still present on throttled/error responses (429 etc.).
    if ($_.Exception.Response) { $response = $_.Exception.Response } else { return $null }
  }

  $raw = Get-HeaderValue $response ("anthropic-ratelimit-unified-{0}-utilization" -f $Window)
  if ([string]::IsNullOrWhiteSpace($raw)) { return $null }

  try { return [double]$raw } catch { return $null }
}

function Invoke-DeviceSet($entityKind, $entityName, $value) {
  $encodedName = [uri]::EscapeDataString($entityName)
  $encodedValue = [uri]::EscapeDataString([string]$value)
  $uri = "http://{0}/{1}/{2}/set?value={3}" -f $DeviceIp, $entityKind, $encodedName, $encodedValue
  try {
    Invoke-WebRequest -Uri $uri -Method Post -Body "" -TimeoutSec 5 -UseBasicParsing | Out-Null
  } catch {
    # Usage sync must never block the agent.
  }
}

function Send-Usage([int]$percent) {
  if ($percent -lt 0) { $percent = 0 }
  if ($percent -gt 100) { $percent = 100 }
  # Percentage first, then tag which tool set it - only after a value we trust.
  Invoke-DeviceSet "number" $EntityName $percent
  Invoke-DeviceSet "text" $SourceEntityName $Source
}

if ($Override -ge 0) {
  Send-Usage ([int][math]::Round($Override))
  return
}

$utilization = Get-Utilization
if ($null -eq $utilization) { return }

$remaining = 100.0 * (1.0 - $utilization)
Send-Usage ([int][math]::Round($remaining))
