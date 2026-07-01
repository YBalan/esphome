param(
  [ValidateSet("reasoning", "need_user_action", "allow_action", "done", "idle", "classify-stop")]
  [string]$Mode = "reasoning"
)

$ErrorActionPreference = "Stop"

function Read-HookPayload {
  $raw = [Console]::In.ReadToEnd()
  if ([string]::IsNullOrWhiteSpace($raw)) {
    return $null
  }

  try {
    return $raw | ConvertFrom-Json -Depth 32
  } catch {
    return $null
  }
}

function Get-WorkspaceRoot {
  $scriptDir = Split-Path -Parent $PSCommandPath
  return Split-Path -Parent (Split-Path -Parent $scriptDir)
}

function Get-StateFile {
  $stateDir = Join-Path $env:TEMP "ai-helper-status-hooks"
  New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
  return Join-Path $stateDir "esphome-ai-helper-status.txt"
}

function Get-TextFromPayload($payload) {
  if ($null -eq $payload) {
    return ""
  }

  $candidates = @(
    $payload.last_assistant_message,
    $payload.lastAssistantMessage,
    $payload.message,
    $payload.delta,
    $payload.prompt,
    $payload.reason
  ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

  return ($candidates -join "`n")
}

function Resolve-Status($mode, $payload) {
  switch ($mode) {
    "reasoning" { return "reasoning" }
    "need_user_action" { return "need_user_action" }
    "allow_action" { return "allow_action" }
    "done" { return "done" }
    "idle" { return "idle" }
    "classify-stop" {
      $text = (Get-TextFromPayload $payload).ToLowerInvariant()
      if ([string]::IsNullOrWhiteSpace($text)) {
        return "done"
      }

      $allowPattern = "\b(allow|approve|permission|confirm|should i|may i|can i proceed|do you want me to|want me to proceed)\b"
      if ($text -match $allowPattern) {
        return "allow_action"
      }

      $needActionPattern = "\b(need you to|you need to|please (run|check|flash|verify|provide|send|look|test|press|click|type|confirm)|manual step|required from you|waiting for you|enter |turn on|connect the device)\b"
      if ($text -match $needActionPattern) {
        return "need_user_action"
      }

      return "done"
    }
  }

  return "reasoning"
}

function Invoke-StatusWebhook($status) {
  $endpointMap = @{
    reasoning = "webhook_reasoning"
    need_user_action = "webhook_need_user_action"
    allow_action = "webhook_allow_action"
    done = "webhook_done"
    idle = "webhook_idle"
  }

  if (-not $endpointMap.ContainsKey($status)) {
    return
  }

  $stateFile = Get-StateFile
  if ((Test-Path $stateFile) -and ((Get-Content $stateFile -Raw).Trim() -eq $status)) {
    return
  }

  $uri = "http://172.16.1.15/button/{0}/press" -f $endpointMap[$status]

  try {
    Invoke-WebRequest -Uri $uri -Method Post -Body "" -TimeoutSec 5 | Out-Null
    Set-Content -Path $stateFile -Value $status -NoNewline
  } catch {
    # Status sync must never block the agent.
  }
}

$null = Get-WorkspaceRoot
$payload = Read-HookPayload
$status = Resolve-Status -mode $Mode -payload $payload
Invoke-StatusWebhook -status $status