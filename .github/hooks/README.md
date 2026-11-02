# AI Helper hooks

One hook definition serves both agents: `.claude/settings.json`. Claude Code reads it natively and
Copilot CLI reads it too (as "repo settings"), so a second Copilot-only file here would make every hook
run twice. Do not add `.github/hooks/*.json` files for the same events.

| Script | What it does |
|---|---|
| `ai-helper-status-sync.ps1 -Mode <status>` | Presses the status webhook on the device (skips when unchanged). Always exits 0 and logs to `%TEMP%\ai-helper-status-hooks\hook.log` (`[claude]` or `[other]`, mode, status, result). |
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

Rules for editing the hook commands in `.claude/settings.json`:

- **No double quotes inside `command`.** A PowerShell parent strips them when it starts the child
  `powershell.exe`, the script path breaks and the hook fails. Wrap the whole `-Command` in single quotes
  and use barewords inside.
- **Don't rely on `${CLAUDE_PROJECT_DIR}` or `args`.** Copilot doesn't define the variable. The command falls
  back to the current directory (Copilot starts hooks in the repo root).
- **Keep `; exit 0`.** Copilot CLI turns a failing `PermissionRequest`/`PreToolUse` hook into a denial, so a
  status light must never be able to fail. The status script also wraps its body in try/catch and always exits 0.
- **Use `Notification` (`permission_prompt`, `elicitation_dialog`), not `PermissionRequest`.** Both agents
  send these. `PermissionRequest` fires before every Copilot tool call, even auto-approved ones.
- **Only use events Copilot really has** (`sessionStart`, `userPromptSubmitted`, `preToolUse`,
  `permissionRequest`, `postToolUse`, `agentStop`, `notification`, ...). `AllowAction` and `Done` are not events.
- Copilot tool names are lower case (`ask_user`), Claude's are capitalised (`AskUserQuestion`); matchers list both.
- The usage gauge is the **Claude** plan's. Don't make it run for Copilot or relabel it "GitHub Copilot";
  Copilot's own gauge is set by the curl protocol in `.github/copilot-instructions.md`.

VS Code Copilot Chat (not the CLI agent) only loads `.claude/settings.json` when the user setting
`chat.useClaudeHooks` is on.
