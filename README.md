# GW2 Map Completion Tracker

A [Nexus](https://raidcore.gg/gw2/nexus) addon for Guild Wars 2 that provides a
per-character map-completion checklist.

## AI Notice

This addon includes AI-generated content and/or AI-assisted code.

AI may have been used during development for tasks such as code generation,
suggestions, documentation, or content creation. While efforts are made to
review and validate all AI-generated output, it may not always be fully
accurate, optimal, or free of unintended behavior.

Users should be aware that:

- AI-generated components may contain errors or inconsistencies, or may be
  unstable;
- behavior may change as the addon evolves or is updated;
- the developer remains responsible for ensuring compliance with the game's
  Terms of Service.

## Important: addon category change on 2026-07-07

On **2026-07-07** this addon will change to the **"memory reading"** category: it
will read map-completion status directly from the game to auto-detect completion
(see *Coming on 2026-07-07* below).

**This release does NOT read game memory.** It uses the manual checklist and the
official Guild Wars 2 API only. A one-time-per-session notice is shown in-game.
If you are not comfortable with an addon reading game memory, please uninstall
before 2026-07-07.

## How it works

- **Manual tracker** — a checklist of every explorable map, grouped by expansion
  and region, with per-character progress, search and filter, and an overall
  progress bar. Always available; check maps off as you complete them.

- **Coming on 2026-07-07 (auto-detect)** — once the addon moves to the
  memory-reading category, the current map is auto-marked complete for the active
  character the moment GW2 delivers the map-completion chest from the server. A
  toast appears bottom-right with a **Revert** button and self-dismisses after 5
  seconds. Works regardless of character level, and is built in — no companion
  addon required.

Characters are detected automatically via the standard Nexus MumbleLink identity
event — log in to each character once and they're added to the dropdown.

## Installing

Place the DLL at:

```
<GW2 install>\addons\MapCompletionTracker.dll
```

Not in a subfolder — Nexus does not recurse. Enable in the Nexus addon manager.
Default toggle keybind is **Ctrl+M** (configurable in Nexus keybind settings as
`KB_MAPCOMPLETION_TOGGLE`).

Per-character completion state is saved to
`addons/MapCompletionTracker/completion.json`.
