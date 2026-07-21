# GW2 Map Completion Tracker

A [Nexus](https://raidcore.gg/gw2/nexus) addon for Guild Wars 2 that
provides a per-character map-completion checklist.

## How it works

- **Manual tracker** — a checklist of every explorable map, grouped by
  expansion and region, with per-character progress, search and filter,
  and an overall progress bar. Always available; check maps off as you
  complete them.
- **Auto-detect** — the addon hooks GW2's reward dispatcher directly
  to detect map-completion events. The current map is auto-marked
  complete for the active character the moment GW2 delivers the
  map-completion chest. A toast appears bottom-right with a Revert
  button and self-dismisses after 5 seconds.

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
