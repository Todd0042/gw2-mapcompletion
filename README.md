# GW2 Map Completion Tracker

A Nexus addon for Guild Wars 2 that provides a per-character map-completion checklist.

## Important

This addon falls under the **"memory reading"** category: it reads map-completion status directly from the game's memory to auto-detect completion.

## How it works

- **Auto-detect** — the current map is auto-marked complete for the active character the moment GW2 delivers the map-completion chest from the server. A toast appears bottom-right with a **Revert** button and self-dismisses after 5 seconds. Works regardless of character level.
- **Manual tracker** — a checklist of every explorable map, grouped by expansion and region, with per-character progress, search and filter, and an overall progress bar.

Characters are detected automatically via the standard Nexus MumbleLink identity event — log in to each character at least once and they're added to the dropdown.

## Installing

Place the DLL at:
<GW2 install>\addons\MapCompletionTracker.dll


Per-character completion state is saved to `addons/MapCompletionTracker/completion.json`.
