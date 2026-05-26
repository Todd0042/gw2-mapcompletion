# GW2 Map Completion Tracker

A [Nexus](https://raidcore.gg/gw2/nexus) addon for Guild Wars 2 that
provides a per-character map-completion checklist.

## How it works

- **Manual tracker** — a checklist of every explorable map, grouped by
  expansion and region, with per-character progress, search and filter,
  and an overall progress bar. Always available; check maps off as you
  complete them.
- **Optional auto-detect** — if the
  [gw2-events-xp](https://github.com/Todd0042/gw2-events-xp) addon is
  also installed, a confirmation popup appears the moment GW2 awards
  map-completion XP. One click marks the current map complete for the
  active character. Without gw2-events-xp the tracker simply runs as
  a manual checklist.

Characters are detected automatically via the standard Nexus MumbleLink
identity event — log in to each character at least once and they're
added to the dropdown.

## Installing

Place the DLL at:

```
<GW2 install>\addons\MapCompletionTracker.dll
```

Not in a subfolder — Nexus does not recurse. Enable in the Nexus addon
manager. Default toggle keybind is **Ctrl+M** (configurable in Nexus
keybind settings as `KB_MAPCOMPLETION_TOGGLE`).

Per-character completion state is saved to
`addons/MapCompletionTracker/completion.json`.

## AI notice

This addon includes AI-generated content and/or AI-assisted code. AI
may have been used during development for tasks such as code
generation, suggestions, documentation, or content creation. While
efforts are made to review and validate all AI-generated output, it
may not always be fully accurate, optimal, or free of unintended
behavior. The developer remains responsible for ensuring compliance
with the game's Terms of Service.
