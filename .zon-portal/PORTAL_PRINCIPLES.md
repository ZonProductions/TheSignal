# ZonPortalDev — Engineering Principles

You are running inside the **ZonPortalDev Shell**. These rules apply to
every turn. They are NOT advisory — follow them as strictly as you'd
follow the project's tech specs.

## ABSOLUTE RULE #1: Rebuild after code/binary changes

If you edit any source that produces a runtime artifact (`.cpp`, build
scripts, packaging scripts, exe-producing Python, shaders, etc.), you
**MUST** rebuild the artifact in the same turn. This is non-negotiable.

- If you don't know the build command, read `build.bat` / `build_local.ps1`
  / `Makefile` / `CMakeLists.txt` / project README to find it.
- If a build script exists in the project root, run it.
- Do not wait for the dev to ask. They have been burned by stale binaries
  more than once and have explicitly told you to do this.
- Report build success/failure and the artifact location.

## ABSOLUTE RULE #2: First response of every session = audit hygiene pass

The **very first response** you produce in a new conversation, before
addressing the dev's prompt, must do an audit hygiene pass:

1. Scan `.zon-portal/revert_requests/` — surface any file missing a final
   `Status: DONE at …` line. Those are interrupted reverts. Ask the dev
   if they want to resume.
2. If you spot tool calls in your recent session log that don't appear to
   be represented in the project (e.g. you edited a BP but the file's
   mtime says otherwise), call it out up front.

After the pass, address the dev's prompt. If nothing needs cleanup, a
single line ("Audit clean — no unfinished revert requests.") is enough.

## On every turn

1. **Verify, don't assume.** Read files or call `get_pin_info` /
   `get_nodes` / `git status` / etc. before claiming a state. If you
   can't programmatically verify something, say so — do not invent.
2. **One change at a time.** Multi-part prompts: complete and confirm
   each step before moving to the next.
3. **Save & compile after BP changes.** Every BlueprintMCP mutation must
   be followed by `save_blueprint` then `compile_blueprint` on the
   affected asset. PIE will use stale bytecode otherwise.
4. **End-of-turn summary line.** Last line of every response: a single
   imperative summary like `Set walk speed to 280`. The ZonPortalDev
   audit panel uses this as the row label.

## On every revert

Follow `.zon-portal/REVERT_PROTOCOL.md`. After completing, append
`**Status: DONE at <ISO timestamp>**` to the request file via `Edit`. If
anything couldn't be reversed exactly, add an `**Unresolved:**` section
listing what's left.

## Audit hygiene (continuous)

- If you notice this turn might slip past the audit (subtle changes, slow
  log flush), say so so the dev can verify.
- If the dev says "audit looks wrong" or "this should be in the audit,"
  reconcile against the session log and explain gaps. Don't guess.

## When in doubt

- File missing / unexpected state → **stop and ask**. No destructive
  guesses.
- Tool call fails / unexpected output → surface the error, don't retry
  blindly.
- `W:\` is **read-only**. The canonical project lives on the desktop;
  mutations land on the laptop copy first. Don't write to W:\ unless the
  dev explicitly authorizes it for a specific path.

---

Project-specific guidance follows below.
