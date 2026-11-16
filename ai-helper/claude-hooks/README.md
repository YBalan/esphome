# AI Helper hooks

These hooks are **user level**: the definition lives in `~/.claude/settings.json` and the scripts in
`~/.claude/hooks/ai-helper/`, so every repo and the Claude desktop app's Code tab run the same copy. Do **not**
add the same events to a repo's `.claude/settings.json`: user and project hooks are both applied, so each hook
would run twice (and the usage sync would ping the API twice).

**Source of truth: the esphome repo, `ai-helper/claude-hooks/`.** `~/.claude/hooks/ai-helper/` is an installed
copy of it (edit the repo copy, not the installed one). After a Windows reinstall or on a new PC, clone the repo
and run

    powershell -NoProfile -ExecutionPolicy Bypass -File ai-helper\claude-hooks\install.ps1

It copies the scripts and this README to `~/.claude/hooks/ai-helper/` and merges `settings-hooks.json` into
`~/.claude/settings.json`: only hook groups that run these scripts are replaced, every other hook and setting is
kept, and whatever it changes is first copied to `~/.claude/backups/ai-helper-install-<time>/`. Safe to run again
(changes nothing when already in sync). To change a hook, edit the repo copy and run it again;
`install.ps1 -Check` lists differences without changing anything (exit code 1 = not in sync). Claude sessions
read hooks at start, so restart them afterwards.

GitHub Copilot (CLI and the desktop app) has its **own user-level hooks** in `~/.copilot/hooks/`, installed
from the esphome repo's `ai-helper/copilot-hooks/` — see its README for the mapping and installer. They reuse
`ai-helper-status-sync.ps1` from this folder (the Copilot installer copies it; there is no second
implementation), so every rule below about the script applies there too. Copilot does not read
`~/.claude/settings.json` and Claude does not read `~/.copilot/hooks/`, so the two pipelines never double-fire.

| Script | What it does |
|---|---|
| `ai-helper-status-sync.ps1 -Mode <status>` | Presses the status webhook on the device (skips when unchanged; the last status is cached in `%TEMP%\ai-helper-status-hooks\ai-helper-status.txt`, one file for every workspace). Always exits 0 and logs to `%TEMP%\ai-helper-status-hooks\hook.log` (mode, filter, status, result, `cwd` of the workspace; the `[claude]`/`[other]` tag is only a guess from environment variables, see below). |
| `ai-helper-usage-sync.ps1` | Pushes the Claude plan usage gauge. Does nothing unless run by Claude Code (`CLAUDE_PROJECT_DIR`/`CLAUDECODE` set), or with `-Override`. |

Event mapping in `~/.claude/settings.json`:

| Event | `-Mode` / script |
|---|---|
| `UserPromptSubmit` | status `reasoning`, plus usage sync |
| `PreToolUse` (matcher `AskUserQuestion\|ExitPlanMode\|ask_user\|exit_plan_mode`) | status `need_user_action` |
| `PostToolUse` | status `reasoning` |
| `Notification` `permission_prompt` | status `allow_action` |
| `Notification` `elicitation_dialog` | status `need_user_action` |
| `Stop` | status `classify-stop`, plus usage sync |

`PreToolUse` and `Notification` commands also pass a `-PayloadFilter` to the status script. Copilot loads
Claude-style settings but ignores the `matcher` (observed: `preToolUse` fired for every tool call, so the
device flipped to `need_user_action` before each one). The script therefore re-checks the real payload and
exits without touching the device status when it does not match:

- `InteractiveTool` needs `tool_name`/`toolName` to be `AskUserQuestion`/`ask_user` or
  `ExitPlanMode`/`exit_plan_mode` (Claude Code and the VS Code compatibility runner send this flat field).
- `PermissionPrompt` / `ElicitationDialog` need `notification_type`/`notificationType` to be
  `permission_prompt` / `elicitation_dialog`.
- `None` (the default, used by `UserPromptSubmit`, `PostToolUse` and `Stop`) accepts every payload.
- Copilot CLI's `preToolUse` payload has no `toolName`; it sends `toolCalls: [{name, args}]`. The filter
  therefore always ignores it there. That is intended: Copilot CLI reports `ask_user` through the
  `elicitation_dialog` notification instead.
- A payload that is missing or unparsable is ignored too (fail closed: no status change, exit 0).

`classify-stop` (the `Stop` hook) looks only at the **closing paragraph** of the last assistant message
(`Get-LastParagraph`) for its keyword lists: `allow_action` for "should I", "permission", "confirm", ...,
`need_user_action` for "please run", "need you to", ..., otherwise `done`. A message that merely mentions those
words earlier on, or quotes them, is `done`. Before this rule a message that described the classifier itself
("Should I proceed?" in a test table, the word "permission") lit the red LED and sounded the buzzer.
The trade-off: a question that is followed by a closing paragraph without any keyword is read as `done`.

An ignored hook writes one log line (`mode`, `filter`, `payloadLength`, `tool`, `notification`,
`result=ignored`) and leaves the webhook and the state file alone. `-DryRun` resolves the status and logs
`result=dry-run` without calling the device; use it, with `-PayloadJson` or a piped payload, to test a hook.

Rules for editing the hook commands in `~/.claude/settings.json`:

- **No double quotes inside `command`.** A PowerShell parent strips them when it starts the child
  `powershell.exe`, the script path breaks and the hook fails. Wrap the whole `-Command` in single quotes
  and use barewords inside.
- **The wrapper reads stdin and passes it as `-PayloadJson`.** Filters and stop classification depend on the
  payload. This is belt and braces: the script falls back to reading stdin itself when `-PayloadJson` is
  empty.
- **Keep the scripts compatible with Windows PowerShell 5.1.** The hook commands call
  `powershell.exe`; do not use PowerShell 7-only parameters such as `ConvertFrom-Json -Depth`. On 5.1 that
  throws, the script's `catch` turns it into a null payload, and every payload-based decision silently
  breaks (every filter fails closed and `classify-stop` always says `done`). This script did exactly that
  until the `-Depth` was removed.
- **Never print to stdout from the status script.** `UserPromptSubmit` stdout is injected into Claude's
  context. It only logs to the file.
- **Don't rely on `${CLAUDE_PROJECT_DIR}` or `args`.** Copilot doesn't define the variable, and the scripts no
  longer live in the project. The commands find them through `$env:USERPROFILE`, so they work from any
  working directory.
- **Keep `; exit 0`.** Copilot CLI turns a failing `PermissionRequest`/`PreToolUse` hook into a denial, so a
  status light must never be able to fail. The status script also wraps its body in try/catch and always exits 0.
- **Use `Notification` (`permission_prompt`, `elicitation_dialog`), not `PermissionRequest`.** Both agents
  send these. `PermissionRequest` fires before every Copilot tool call, even auto-approved ones.
- **Only use events Copilot really has** (`sessionStart`, `userPromptSubmitted`, `preToolUse`,
  `permissionRequest`, `postToolUse`, `agentStop`, `notification`, ...). `AllowAction` and `Done` are not events.
- Copilot tool names are lower case (`ask_user`), Claude's are capitalised (`AskUserQuestion`); matchers list both.
- **Do not identify the runner only from `CLAUDE_PROJECT_DIR` or `CLAUDECODE`.** VS Code's Claude-hook
  compatibility runner can set Claude-style environment variables, so the `[claude]`/`[other]` log tag is
  only a guess. Filter on the hook payload instead. (`ai-helper-usage-sync.ps1` still decides this from the
  environment alone; see its header.)
- The usage gauge is the **Claude** plan's. Don't make it run for Copilot or relabel it "GitHub Copilot";
  nothing sets Copilot's gauge automatically (it has no hook-accessible usage feed).

VS Code Copilot Chat (not the CLI agent) only loads Claude-style hook settings when the user setting
`chat.useClaudeHooks` is on.
