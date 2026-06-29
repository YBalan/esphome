When working in this repository, keep the AI Helper semaphore status in sync with the current work phase by calling the local webhook endpoints on `172.16.1.173`.

Status protocol:
- Set `Reasoning` when you start active work on a task: reading code, planning, editing, compiling, testing, debugging, or otherwise processing.
- Set `Need User Action` before asking the user for missing information, manual verification, flashing, physical checks, secrets, or any other user-side action.
- Set `Allow Action` before asking the user for approval to do something they may want to explicitly permit, such as risky operations, deployments, or other consequential actions.
- Set `Done` when the requested work is complete and you are about to send the final response.
- Set `Idle` only when the user explicitly asks to reset or clear the status.

Webhook commands:
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.173/button/webhook_reasoning/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.173/button/webhook_need_user_action/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.173/button/webhook_allow_action/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.173/button/webhook_done/press"`
- Use `curl.exe -s -o NUL -X POST -d "" "http://172.16.1.173/button/webhook_idle/press"`

Behavior rules:
- Do not spam repeated webhook calls when the status has not changed.
- If the webhook request fails, continue the task and do not block on the status update.
- Prefer updating the status before the related user-visible phase change, not after.
- If a task moves from waiting back into active work, switch back to `Reasoning`.

This protocol applies to normal work in this repository and should be treated as part of the default workflow.