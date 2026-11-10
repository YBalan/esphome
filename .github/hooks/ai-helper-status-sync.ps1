param(
  [ValidateSet("reasoning", "need_user_action", "allow_action", "done", "idle", "classify-stop")]
  [string]$Mode = "reasoning",

  [ValidateSet("None", "InteractiveTool", "PermissionPrompt", "ElicitationDialog")]
  [string]$PayloadFilter = "None",

  [string]$PayloadJson,

  [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Read-HookPayload([string]$raw) {
  if ([string]::IsNullOrWhiteSpace($raw)) {
    $raw = [Console]::In.ReadToEnd()
  }

  if ([string]::IsNullOrWhiteSpace($raw)) {
    return $null
  }

  try {
    return $raw | ConvertFrom-Json
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

function Write-HookLog($message) {
  # Small log so a misbehaving hook can be diagnosed from either agent (Claude Code or Copilot).
  try {
    $log = Join-Path $env:TEMP "ai-helper-status-hooks\hook.log"
    New-Item -ItemType Directory -Path (Split-Path -Parent $log) -Force | Out-Null
    if ((Test-Path $log) -and (Get-Item $log).Length -gt 100KB) { Set-Content -Path $log -Value "" }
    $agent = if ($env:CLAUDE_PROJECT_DIR -or $env:CLAUDECODE) { "claude" } else { "other" }
    Add-Content -Path $log -Value ("{0:s} [{1}] {2}" -f (Get-Date), $agent, $message)
  } catch {
    # Logging is best effort.
  }
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

function Get-PayloadValue($payload, [string[]]$names) {
  if ($null -eq $payload) {
    return $null
  }

  foreach ($name in $names) {
    $property = $payload.PSObject.Properties[$name]
    if ($null -ne $property -and -not [string]::IsNullOrWhiteSpace([string]$property.Value)) {
      return [string]$property.Value
    }
  }

  return $null
}

function Normalize-ToolName([string]$toolName) {
  switch ($toolName) {
    { $_ -in @("AskUserQuestion", "ask_user") } { return "ask_user" }
    { $_ -in @("ExitPlanMode", "exit_plan_mode") } { return "exit_plan_mode" }
    default { return $toolName.ToLowerInvariant() }
  }
}

function Test-PayloadFilter($payload, [string]$filter) {
  switch ($filter) {
    "None" { return $true }
    "InteractiveTool" {
      $toolName = Get-PayloadValue -payload $payload -names @("tool_name", "toolName")
      if ([string]::IsNullOrWhiteSpace($toolName)) {
        return $false
      }

      return (Normalize-ToolName $toolName) -in @("ask_user", "exit_plan_mode")
    }
    "PermissionPrompt" {
      $notificationType = Get-PayloadValue -payload $payload -names @("notification_type", "notificationType")
      return $notificationType -eq "permission_prompt"
    }
    "ElicitationDialog" {
      $notificationType = Get-PayloadValue -payload $payload -names @("notification_type", "notificationType")
      return $notificationType -eq "elicitation_dialog"
    }
  }

  return $false
}

# A message that really waits for the user ends with the question or request, so only the closing
# paragraph is classified. Keywords earlier on ("permission", a quoted "should I", ...) are just
# prose; matching them lit the red LED for a message that merely described this classifier.
function Get-LastParagraph([string]$text) {
  if ([string]::IsNullOrWhiteSpace($text)) {
    return ""
  }

  $paragraphs = @($text.Trim() -split "\r?\n[ \t]*\r?\n" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
  if ($paragraphs.Count -eq 0) {
    return ""
  }

  return [string]$paragraphs[$paragraphs.Count - 1]
}

function Resolve-Status($mode, $payload) {
  switch ($mode) {
    "reasoning" { return "reasoning" }
    "need_user_action" { return "need_user_action" }
    "allow_action" { return "allow_action" }
    "done" { return "done" }
    "idle" { return "idle" }
    "classify-stop" {
      $text = (Get-LastParagraph (Get-TextFromPayload $payload)).ToLowerInvariant()
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
    return "ignored"
  }

  $stateFile = Get-StateFile
  # An empty state file (e.g. caught mid-write by a parallel hook) reads as $null.
  $previous = if (Test-Path $stateFile) { Get-Content $stateFile -Raw } else { $null }
  if ($previous -and $previous.Trim() -eq $status) {
    return "unchanged"
  }

  $uri = "http://172.16.1.15/button/{0}/press" -f $endpointMap[$status]

  try {
    Invoke-WebRequest -Uri $uri -Method Post -Body "" -TimeoutSec 5 -UseBasicParsing | Out-Null
    Set-Content -Path $stateFile -Value $status -NoNewline
    return "sent"
  } catch {
    # Status sync must never block the agent.
    return "device unreachable"
  }
}

# Claude Code and Copilot CLI both run this from the shared .claude/settings.json. Copilot CLI
# turns a failing permission-related hook into a denial, so nothing may escape: always exit 0.
try {
  $null = Get-WorkspaceRoot
  $payload = Read-HookPayload -raw $PayloadJson
  if (-not (Test-PayloadFilter -payload $payload -filter $PayloadFilter)) {
    $toolName = Get-PayloadValue -payload $payload -names @("tool_name", "toolName")
    $notificationType = Get-PayloadValue -payload $payload -names @("notification_type", "notificationType")
    Write-HookLog "mode=$Mode filter=$PayloadFilter payloadLength=$($PayloadJson.Length) tool=$toolName notification=$notificationType result=ignored"
    exit 0
  }

  $status = Resolve-Status -mode $Mode -payload $payload
  $result = if ($DryRun) { "dry-run" } else { Invoke-StatusWebhook -status $status }
  Write-HookLog "mode=$Mode filter=$PayloadFilter status=$status result=$result"
} catch {
  Write-HookLog "mode=$Mode filter=$PayloadFilter error=$($_.Exception.Message)"
}
exit 0