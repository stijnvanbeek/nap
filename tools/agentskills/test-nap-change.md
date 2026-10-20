---
name: test-nap-change
description: |
  Test a change to a file that a nap app loads by running the app in a background process on the person's computer, watching its console log for errors, editing the file, iterating until the log is clean, restarting the app if it fails to initialise, then killing it and reporting. Triggers on "test the change in <file> running <app>", "run napstudio and watch the log while editing <file>", "iterate on this <file> running <app>".
---

# Test a nap change against a live app log

Drive an edit-test loop: run the nap app <app> that references a changed <file> as a background process on the person's own computer, watch its console log for errors, make the requested edits to the file, re-check the log, and repeat until the log is clean. Kill the app at the end and report.

The file can be any text type the app reads at runtime: `.chai`, `.json` (objects.json / models), `.xml`, `.txt`, shader/config text, etc. The loop is identical regardless of type; only *how the app picks up the change* differs (see "Reload vs restart").

## When to use
- The person made or wants a change to a data/text file that a running nap app consumes, and wants it validated against the app's actual console output rather than assumed correct.
- Typical trigger: "test my change in <file>", "watch the log while I edit <file>", "iterate until the log is clean".

## When NOT to use
- Pure C++ source changes: those need a rebuild first (use build-cmake-target), not just a rerun. Only reach for this when the change is in a file the app loads at runtime without recompiling.
- A file no running app references. Confirm which app/binary loads the file before starting.

## Timing budget (keeps the loop fast)
- **Every wait is 5 seconds, never longer**: 5 s between launching the app and the first log scan, and 5 s between an edit/restart and the next scan. Nap apps initialise in a couple of seconds; longer sleeps just burn a turn.
- **One command per check.** Bundle the wait and all checks into a single `device_bash` call: `sleep 5`, then liveness (`pgrep`), CPU (`ps -o %cpu`), log size (`wc -l`), and the error `grep` — all together. Do NOT chain separate follow-up calls ("is it alive?" … "still alive??" … "log steady?") — that tripled the wall time per iteration when this loop was first driven.
- Only extend beyond one check if the single check is genuinely ambiguous (process died, or log still growing).

## Prerequisites to pin down first (ask only what's missing)
1. **The file** under test — absolute path.
2. **The app binary** that references it — e.g. `napstudio`. Locate it (usually a build dir like `cmake-build-debug/bin`). If unsure, ask.
3. **How the app reaches the file.** In nap, napstudio commonly loads another app's `objects.json` through a NestedResourceManager resource, so you do NOT copy or symlink the file — the running app already sees it in place. Confirm this holds for the target file; if the app instead needs the file copied into a data dir, do that once before starting.
4. **Which computer.** Use the conversation's computer; if it's offline, stop and tell the person rather than hopping to another machine on your own.

All commands below run via `device_bash` on the person's computer (the workspace shell cannot see their files). Every command starts with a one-line comment saying what it does.

## Step 0 — Baseline: confirm nothing stale is running
```
# Check whether the nap app is already running
pgrep -x napstudio || echo "not running"
```
If a stale instance from earlier work is alive, decide with the person whether to reuse or kill it. Prefer a fresh, single known instance you launched yourself so the log maps cleanly to your session.

## Step 1 — Launch the app in the background, capturing its log
Start it detached with stdout+stderr redirected to a known log file. Pick a fresh log path per session, e.g. `/tmp/napstudio_<shortname>.log`.

Launch style per OS (verified empirically):
- **Linux**: `setsid ./napstudio > log 2>&1 < /dev/null &` — own session, killable wholesale.
- **macOS**: `setsid` does NOT exist (`command not found`). Use `nohup ./napstudio > log 2>&1 < /dev/null &` instead. Expect the launching tool call to hit its timeout and move to the background because the nohup'd child inherits the pipes — that is normal and harmless; the app detaches fine.

```
# Start napstudio in the background, logging to /tmp/napstudio_test.log
cd <dir the binary expects to run from>   # napstudio runs relative to its data/ objects.json
nohup ./napstudio > /tmp/napstudio_test.log 2>&1 < /dev/null & echo $! > /tmp/napstudio_test.pid; echo "started pid $(cat /tmp/napstudio_test.pid)"
```

Notes:
- `$!` may be the wrapper shell's pid rather than the app's. Right after launch (or in the Step 2 command) resolve the real one with `pgrep -x napstudio` and write THAT to the pid file, so cleanup by pid works.
- Record the PID in the `.pid` file next to the log — the launch command itself may be aborted, and the pid file is the only durable record of what to kill.
- `< /dev/null` prevents the app from blocking on stdin if the parent shell disappears.

### Abort-recovery (run this FIRST after any interrupted launch)
If the previous turn ended with an aborted or timed-out launch command, the app may still be alive even though the shell that started it is gone. Before doing anything else:
```
# Recover: find and kill any orphaned instance from our last launch
[ -f /tmp/napstudio_test.pid ] && kill "$(cat /tmp/napstudio_test.pid)" 2>/dev/null
pkill -x napstudio 2>/dev/null
sleep 1
pgrep -x napstudio || echo "clean"
```
Only proceed to a fresh launch once `pgrep` confirms nothing is running. (On Linux also `pkill -f napstudio` children if setsid spawned a group.)

## Step 2 — Wait 5 s, then one bundled check of the log
Exactly 5 seconds after launch, run ONE command that reports liveness, CPU, log size and errors together:
```
# Wait for startup then scan the log for errors/warnings
sleep 5
pgrep -x napstudio >/dev/null && echo ALIVE || echo DEAD
grep -iE "error|fail|exception|couldn't|unable" /tmp/napstudio_test.log | grep -viE "no error|errors: 0" || echo "NO ERRORS"
echo "---"; wc -l < /tmp/napstudio_test.log; tail -n 5 /tmp/napstudio_test.log
```
A clean baseline means only benign lines (module-duplicate warnings, Vulkan layer notices, audio "Late callback" notices), typically ending at the `Loading .../objects.json` line. Note the line count so you can detect new output later. Per-frame script errors (the kind that only appear once the app draws) show up within this window; if the app were going to throw per-frame, this check catches it.

## Step 3 — Make the requested change to <file>
Apply the edit the person asked for using `device_edit` (exact string replace) or `device_write` (full rewrite) on the absolute path. Read the file first (`device_read`) if you haven't seen its current content — the person may have edited it between turns. Keep changes surgical.

## Step 4 — Reload vs restart (decide per file type)
- **Hot-reloadable** (many nap data files are re-read, or the app watches): after editing, re-run the Step 2 bundled check and compare line count against the baseline.
- **Not hot-reloaded** (parsed once at startup — `.chai` scripts are like this): the running instance won't see the change. Restart it — `pkill -x <app>`, relaunch per Step 1, then re-run the bundled check.
When in doubt, restart: it's deterministic and cheap.

## Step 5 — Iterate until the log is clean
Loop Steps 3–4. Each iteration is: edit → restart if needed → ONE bundled check 5 s later (the Step 2 command). The check already proves the three things a full sign-off needs — alive, consuming CPU (drawing frames), no error lines, steady log size versus last read:
```
# 5 s after the (re)start: alive + CPU + clean log in one go
sleep 5
NP=$(pgrep -x napstudio); [ -n "$NP" ] && echo "ALIVE $NP" || echo DEAD
[ -n "$NP" ] && ps -o %cpu,pid -p "$NP"
grep -iE "error|fail|exception|couldn't|unable" /tmp/napstudio_test.log | grep -viE "no error|errors: 0" || echo "NO ERRORS"
wc -l < /tmp/napstudio_test.log
```
- If errors reference the file (e.g. `Error evaluating <file>` at a line/col, a JSON parse failure, a missing symbol), diagnose, fix the file, and repeat.
- **If the app failed to initialise / crashed** (process gone, or log ends in a fatal/init error): treat that as a failed iteration. Fix the file, then **restart the app** (kill any remnant, relaunch per Step 1) before re-checking.
- Stop when the bundled check shows ALIVE with non-zero CPU and zero error lines. Do not add extra "confirm it's still fine" waits beyond this one check.

## Step 6 — Kill the app and report
Kill by PID first (precise), then sweep by name in case of forked children:
```
# Stop the nap app we started
kill "$(cat /tmp/napstudio_test.pid)" 2>/dev/null
pkill -x napstudio 2>/dev/null
sleep 1
pgrep -x napstudio || echo "stopped"
rm -f /tmp/napstudio_test.pid
```
Then report concisely: what was changed in the file, how many iterations it took, that the final log is clean (quote the decisive check: zero matching error lines, app alive drawing frames), and that the app was stopped. Be honest if you could not visually confirm on-screen output — the log is the acceptance signal, not a guarantee pixels rendered correctly.

Exception: when the person is iterating on the file's *visual* result (draw code, layout, styling), they usually want the window left open to judge it — leave the app running, say which pid it is, and offer to kill it when they're done.

## Pitfalls (learned driving napstudio)
- **Subagents can't run the app.** A `task` subagent has no `device_bash`, so it cannot launch or kill processes on the person's Mac. Do all process control directly with `device_bash`; optionally use a subagent only to *read and interpret* a log file you've told it the path to.
- **`setsid` is Linux-only.** On macOS it fails with "command not found" and the app never starts; use `nohup` there.
- **Wrong computer / offline computer.** If the conversation's computer is disconnected, don't silently switch machines — tell the person and ask. Reopening the Thaura app on the same Mac mints a NEW computer id; confirm with the person before using it.
- **Stale instances confuse the log.** Always know which PID you launched; kill strays first so the log maps to your session.
- **Abort/orphan hazard.** A `device_bash` call that launches a GUI app can be aborted mid-execution (person hits escape, timeout, network blip). With `nohup`, the child outlives the wrapper and keeps running invisibly — its window appears on the person's desktop and its log keeps growing with no owner. Use the pid file plus `pkill -x` sweep at the start of every turn before launching anything new. Never assume a launch succeeded just because the previous turn's tool result looked normal; verify with `pgrep` at the start of the next interaction if there was any interruption.
- **Log buffering.** Redirects can lag a second; the 5 s sleep covers it — compare line counts rather than assuming emptiness.
- **"$! is not always the app."** The pid from `$!` can be a wrapper shell; resolve the app's real pid with `pgrep -x <app>` and store that.
- **"Clean log" ≠ "renders correctly."** Absence of errors proves the file parsed and the per-frame callback didn't throw; it doesn't prove the visual result is right. Say so in the report and invite the person to look at the window.
- **Don't leave the app running** when the goal was just validation; end with the process killed unless the person says otherwise or needs to visually judge the result.

## Style
- One short comment line atop every shell command.
- Keep edits minimal and targeted to the reported error.
- Keep the loop tight: 5 s waits, one bundled check per iteration, no redundant confirmation commands.
- Report tersely: change → iterations → clean-log evidence → app stopped (or left running for visual check). No narration of every intermediate attempt unless something went sideways.
