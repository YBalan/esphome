# AI Helper hooks for GitHub Copilot

These hooks are **user level** for GitHub Copilot (CLI and the desktop app): the definition is one JSON file in
Copilot's user hooks directory — `~/.copilot/hooks/ai-helper.json` (`$COPILOT_HOME/hooks/` when `COPILOT_HOME`
is set) — and the status script lives next to it in `~/.copilot/hooks/ai-helper/`. Copilot loads every top-level
`*.json` in that directory for every session, so the semaphore follows Copilot in any repo. This is completely
separate from the Claude hooks (`~/.claude/`, see [`../claude-hooks/README.md`](../claude-hooks/README.md));
neither agent reads the other's user-level files, so nothing runs twice.

**Source of truth: the esphome repo, `ai-helper/copilot-hooks/`.** The installed copy must not be edited. The
status script itself is shared with the Claude hooks — the installer copies
[`../claude-hooks/ai-helper-status-sync.ps1`](../claude-hooks/ai-helper-status-sync.ps1), there is no second
implementation. After a Windows reinstall or on a new PC, clone the repo and run

    powershell -NoProfile -ExecutionPolicy Bypass -File ai-helper\copilot-hooks\install.ps1

or the VS Code task **`AI Helper: install Copilot hooks`** (`AI Helper: check Copilot hooks (no changes)` is the
read-only check; exit code 1 = not in sync). The installer backs up whatever it changes to
`~/.copilot/backups/ai-helper-install-<time>/` and is safe to repeat. To change a hook, edit the repo copy, run
`install.ps1` again, and restart the Copilot session (hooks are read at session start). The usage-sync script is
**not** installed: it reports the **Claude** plan and Copilot has no hook-accessible usage feed, so nothing sets
the gauge for Copilot.

Event mapping in `ai-helper.json` (PascalCase event names — Copilot then sends the snake_case
"VS Code compatible" payloads that `ai-helper-status-sync.ps1` already understands from Claude Code):

| Event | `-Mode` | Resulting status |
|---|---|---|
| `UserPromptSubmit`, `PostToolUse` | `reasoning` | `Reasoning` |
| `Notification` (matcher `permission_prompt`) | `allow_action` | `Allow Action` |
| `Notification` (matcher `elicitation_dialog`) | `need_user_action` | `Need User Action` |
| `Stop` | `classify-stop` | `Done` in practice (see below) |

Differences from the Claude wiring, on purpose:

- **No `PreToolUse` hook.** Copilot command `preToolUse` hooks are *fail-closed*: a crash or non-zero exit
  denies the tool call. A status light must never be able to block a tool, and Copilot already reports
  `ask_user` through the `elicitation_dialog` notification, which sets `Need User Action` anyway.
- **`Stop` lands on `Done`.** Copilot's `Stop` payload has no `last_assistant_message` (only
  `transcript_path`), so `classify-stop` finds no text and falls back to `done`. The mode is kept so the
  classification starts working by itself if Copilot ever adds the field.
- **`matcher` is honored natively** (regex on `notification_type`), unlike the old Claude-settings
  compatibility path that ignored it. The `-PayloadFilter` re-check in the script stays as belt and braces.
- **No usage sync** (see above).

Rules for editing the `powershell` commands in `ai-helper.json`:

- **No double quotes inside the command string** and keep everything as barewords/single quotes — same rule as
  the Claude hook commands (a PowerShell parent can strip double quotes when starting the child shell).
- **Resolve the script through `COPILOT_HOME`/`USERPROFILE`**, never a repo-relative path: user-level hooks run
  in every workspace.
- **Start every command with `Set-ExecutionPolicy Bypass -Scope Process -Force -ErrorAction SilentlyContinue;`.**
  The Claude commands pass `-ExecutionPolicy Bypass` to `powershell.exe`, but Copilot only gives us the command
  body, and a fresh Windows is `Restricted`: `& script.ps1` then fails with "running scripts is disabled", the
  `; exit 0` hides it, and the light silently never follows Copilot (nothing is logged either). The process-scope
  override needs no admin rights, changes nothing on disk, and stays in-process (no second `powershell.exe` per
  hook). A group policy that sets the policy still wins, exactly as it does for the Claude hooks.
- **Keep `; exit 0`** and keep the script PowerShell 5.1 compatible, silent on stdout, always exiting 0 —
  identical reasons as in [`../claude-hooks/README.md`](../claude-hooks/README.md).
- **Use PascalCase event names** (`UserPromptSubmit`, `Stop`, ...). camelCase names (`userPromptSubmitted`,
  `agentStop`) deliver camelCase payloads instead; the script handles both field spellings, but keep one format.
- **Stay away from `preToolUse`/`permissionRequest`** (fail-closed / fires for every tool call; see above).

Diagnostics are shared with the Claude hooks: every run appends one line to
`%TEMP%\ai-helper-status-hooks\hook.log`, tagged `[other]` in Copilot sessions (no `CLAUDE_PROJECT_DIR` /
`CLAUDECODE` in the environment), and the last sent status is cached in
`%TEMP%\ai-helper-status-hooks\ai-helper-status.txt` — one state for both agents, so whichever agent changes
the status last wins and unchanged statuses are not re-sent. If the light does not follow Copilot, check that
log first; in a **sandboxed** Copilot CLI session the hook runs under the sandbox network policy, which may
block the plain-HTTP call to the device (`result=device unreachable` in the log).
