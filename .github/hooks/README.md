# AI Helper hooks

One hook definition serves both agents: `.claude/settings.json`. Claude Code reads it natively and
Copilot CLI reads it too (as "repo settings"), so a second Copilot-only file here would make every hook
run twice. Do not add `.github/hooks/*.json` files for the same events.

| Script | What it does |
|---|---|
| `ai-helper-status-sync.ps1 -Mode <status>` | Presses the status webhook on the device (skips when unchanged). Always exits 0 and logs to `%TEMP%\ai-helper-status-hooks\hook.log`, which is shared by every workspace using a copy of this script (mode, filter, status, result; the `[claude]`/`[other]` tag is only a guess from environment variables, see below). |
| `ai-helper-usage-sync.ps1` | Pushes the Claude plan usage gauge. Does nothing unless run by Claude Code (`CLAUDE_PROJECT_DIR`/`CLAUDECODE` set), or with `-Override`. |

Event mapping in `.claude/settings.json`:

| Event | `-Mode` / script |
|---|---|
| `UserPromptSubmit` | status `reasoning`, plus usage sync |
| `PreToolUse` (matcher `AskUserQuestion\|ExitPlanMode\|ask_user\|exit_plan_mode`) | status `need_user_action` |
| `PostToolUse` | status `reasoning` |
| `Notification` `permission_prompt` | status `allow_action` |
| `Notification` `elicitation_dialog` | status `need_user_action` |
| `Stop` | status `classify-stop`, plus usage sync |

`PreToolUse` and `Notification` commands also pass a `-PayloadFilter` to the status script. Copilot loads
`.claude/settings.json` but ignores the `matcher` (observed: `preToolUse` fired for every tool call, so the
device flipped to `need_user_action` before each one). The script therefore re-checks the real payload and
exits without touching the device status when it does not match:

- `InteractiveTool` needs `tool_name`/`toolName` to be `AskUserQuestion`/`ask_user` or
  `ExitPlanMode`/`exit_plan_mode` (Claude Code and the VS Code compatibility runner send this flat field).
- `PermissionPrompt` / `ElicitationDialog` need `notification_type`/`notificationType` to match.
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

Rules for editing the hook commands in `.claude/settings.json`:

- **No double quotes inside `command`.** A PowerShell parent strips them when it starts the child
  `powershell.exe`, the script path breaks and the hook fails. Wrap the whole `-Command` in single quotes
  and use barewords inside.
- **The wrapper reads stdin and passes it as `-PayloadJson`.** Filters and stop classification depend on the
  payload. This is belt and braces: the script falls back to reading stdin itself when `-PayloadJson` is
  empty, and that also works (tested through Git Bash). The earlier "empty payload" symptom was not stdin
  being swallowed but the next rule.
- **Keep the scripts compatible with Windows PowerShell 5.1.** The hook commands call
  `powershell.exe`; do not use PowerShell 7-only parameters such as `ConvertFrom-Json -Depth`. On 5.1 that
  throws, the script's `catch` turns it into a null payload, and every payload-based decision silently
  breaks.
- **Don't rely on `${CLAUDE_PROJECT_DIR}` or `args`.** Copilot doesn't define the variable. The command falls
  back to the current directory (Copilot starts hooks in the repo root).
- **Keep `; exit 0`.** Copilot CLI turns a failing `PermissionRequest`/`PreToolUse` hook into a denial, so a
  status light must never be able to fail. The status script also wraps its body in try/catch and always exits 0.
- **Use `Notification` (`permission_prompt`, `elicitation_dialog`), not `PermissionRequest`.** Both agents
  send these. `PermissionRequest` fires before every Copilot tool call, even auto-approved ones.
- **Only use events Copilot really has** (`sessionStart`, `userPromptSubmitted`, `preToolUse`,
  `permissionRequest`, `postToolUse`, `agentStop`, `notification`, ...). `AllowAction` and `Done` are not events.
- Copilot tool names are lower case (`ask_user`), Claude's are capitalised (`AskUserQuestion`); matchers list both.
- **Do not identify the runner only from `CLAUDE_PROJECT_DIR` or `CLAUDECODE`.** VS Code's Claude-hook
  compatibility runner can set Claude-style environment variables. Filter on the hook payload instead.
- The usage gauge is the **Claude** plan's. Don't make it run for Copilot or relabel it "GitHub Copilot";
  Copilot's own gauge is set by the curl protocol in `.github/copilot-instructions.md`.

VS Code Copilot Chat (not the CLI agent) only loads `.claude/settings.json` when the user setting
`chat.useClaudeHooks` is on.
