When working in this repository, keep the AI Helper semaphore status in sync with the current work phase by calling the local webhook endpoints on `172.16.1.15`.

Status protocol:
- Set `Reasoning` when you start active work on a task: reading code, planning, editing, compiling, testing, debugging, or otherwise processing.
- Set `Need User Action` before asking the user for missing information, manual verification, flashing, physical checks, secrets, or any other user-side action.
- Set `Allow Action` before asking the user for approval to do something they may want to explicitly permit, such as risky operations, deployments, or other consequential actions.
- Set `Allow Action` before asking user to Allow Reading external files, such as reading from the filesystem, network, or other external sources.
- Set `Done` when the requested work is complete and you are about to send the final response.
- Set `Idle` only when the user explicitly asks to reset or clear the status.

Webhook commands:
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/button/webhook_reasoning/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/button/webhook_need_user_action/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/button/webhook_allow_action/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/button/webhook_done/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/button/webhook_idle/press"`

Remaining-usage gauge:
- The device exposes a `Usage Remaining` percentage (0-100 %) and a `Usage Source` text label, both settable. When the requested work is done (at the same time you set `Done`), report your remaining usage and tag it as GitHub Copilot. The entity names are URL-encoded, so keep the `%20` spaces:
  - `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/number/AI%20Helper%20Usage%20Remaining/set?value=75"`
  - `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.15/text/AI%20Helper%20Usage%20Source/set?value=GitHub%20Copilot"`
- Only send a percentage you can actually estimate. If you have no reliable number, skip the percentage call (but you may still set the source). Send at most once per completed turn.

Behavior rules:
- Do not spam repeated webhook calls when the status has not changed.
- If the webhook request fails, continue the task and do not block on the status update.
- Prefer updating the status before the related user-visible phase change, not after.
- If a task moves from waiting back into active work, switch back to `Reasoning`.
- Customization preflight: before editing repository customization assets (`.github/copilot-instructions.md`, `.github/agents/*`, `.github/skills/*`, `.claude/agents/*`, `.claude/skills/*`, and related instruction/agent/skill files), set `Allow Action` first, then return to `Reasoning` once edits begin.
- External-read preflight: if a command/tool output references a path outside the current workspace (for example under `%APPDATA%`, temp folders, or network paths), set `Allow Action` before requesting or performing any read/search on that path.
- Never call file-read/search tools on outside-workspace paths until user approval is requested and granted.
- After outside-workspace approval is granted and the read is complete, switch back to `Reasoning` for normal task work.

This protocol applies to normal work in this repository and should be treated as part of the default workflow.