# event_log.md

This workspace has a project-wide event log: `event_log.txt` (next to this file, inside the
`ox` repo).

It is a single, shared record of everything notable that happens in the codebase - new
features, bugfixes, performance work, releases, and so on - across the repos in this
workspace (`ox`, `ox-runtime`, `ox-ipc-proxy`, `ox-sim-driver`, `ox-driver-example`, `vog`,
`blender-xr-regression-tests` and `ox-runtime.github.io`).

The project started in January 2026 as a single repo. In March 2026 it was split into the
`ox-runtime`, `ox-ipc-proxy` and `ox-sim-driver` repos, so history from before the split comes
from the `ox` repo.

## What a line looks like

    [2026-04-09] [SEV-1] [RELEASE]   ox v0.6.0: XR testing fully inside the application process (no IPC), a redesigned GUI, and more release formats.

Each line has four parts:

1. **Date** in `YYYY-MM-DD` format - taken from the git commit date, or the date in
   `CHANGES.md` if we ever add one.
2. **Severity tag** - `SEV-1` down to `SEV-5`. See below.
3. **Category label** - one fixed word in brackets. See below.
4. **One-line description** - plain language. Links (GitHub issues, PRs, docs pages) go at
   the end.

## Severity levels

| Tag   | Meaning                                                                    |
| ----- | -------------------------------------------------------------------------- |
| SEV-1 | Project-impacting. Project start, repo splits, major releases, headline features. |
| SEV-2 | Big user-facing additions: new repos, major features, big fixes.           |
| SEV-3 | Normal bugfixes, performance wins, and useful improvements.                |
| SEV-4 | Minor fixes, dependency bumps, small internal cleanups.                    |
| SEV-5 | Trivial stuff: styling, typos, tweaks only relevant to the team.           |

## Categories

`RELEASE`, `FEATURE`, `BUGFIX`, `PERF`, `UI`, `ENGINE`, `SECURITY`, `INSTALLER`, `INTERNAL`,
`DEPS`, `CI`, `DOCS`.

Use one category per entry. If a change fits two (e.g. a UI bugfix), pick the one a reader
would care about most. Not every category is in use yet.

## How entries are ordered

Oldest first, newest last - like an append-only log. When one change touched several repos on
the same day (e.g. the March 2026 split), it gets **one** entry, not one per repo.

## Adding a new entry

1. Pick the date of the change (commit date, or today for non-code events).
2. Pick the lowest severity that honestly describes the impact. When in doubt, go one level
   down.
3. Write one line. Plain sentences, no jargon, no filler. Add a link if the change has an
   issue, PR, or docs page.
4. Append it at the bottom of `event_log.txt`, keeping the same format.

That's it - no build step, no tooling. The file is the log.
