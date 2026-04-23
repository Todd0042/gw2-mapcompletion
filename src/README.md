# GW2 Map Completion Tracker – Nexus Addon

A **Guild Wars 2** addon for the [Nexus addon loader](https://raidcore.gg/gw2/nexus) that
tracks map completion across all explorable maps. It combines **manual input** (checkboxes
in the UI) with **automatic detection** via the Unofficial Extras chat event API.

---

## Features

| Feature | Details |
|---|---|
| Progress bar | Shows `X / Y maps (Z%)` at a glance |
| Per-map checkboxes | Manually mark any map complete/incomplete |
| Region groups | Maps are collapsed by region (Core Tyria, HoT, PoF, EoD, …) |
| Search & filter | Search by name; filter All / Completed / Incomplete |
| Current map highlight | Highlights the map you're currently standing in |
| Auto-detection | Listens for the system chat message GW2 sends on map completion |
| On-screen alert | Nexus yellow alert banner fires when a new map is detected complete |
| Persistent save | Completion state saved to `addons/MapCompletionTracker/completion.json` |
| Keybind | Default **Ctrl+M** (configurable in Nexus keybind settings) |

---

## How auto-detection works

When you complete all exploration objectives on a map, GW2 sends a system
message on the **Local** chat channel:

```
You have completed <Map Name>!
```

The [Unofficial Extras](https://github.com/Krappa322/arcdps_unofficial_extras_releases)
addon relays every chat message as the Nexus event
`EV_UNOFFICIAL_EXTRAS_CHAT_MESSAGE`.  
This addon subscribes to that event, checks for the `"You have completed"` string,
identifies which map was completed, marks it in the tracker, and fires an
on-screen alert via `APIDefs->SendAlert(...)`.

> **Important:** Auto-detection requires **Unofficial Extras** to be installed.
> Without it the addon still works – you just mark maps manually via checkboxes.

---

## Prerequisites

| Requirement | Notes |
|---|---|
| [Nexus](https://raidcore.gg/gw2/nexus) | The addon loader |
| [Unofficial Extras](https://github.com/Krappa322/arcdps_unofficial_extras_releases) | Needed for auto-detection; optional but recommended |
| [ArcDPS](https://www.deltaconnected.com/arcdps/) | Required by Unofficial Extras |

---

## Building

### Dependencies to place in `third_party/`

```
third_party/
  nlohmann/
    json.hpp          ← https://github.com/nlohmann/json/releases (single-header)
  imgui/
    imgui.h
    imgui_internal.h
    … (other ImGui headers)  ← https://github.com/RaidcoreGG/imgui-v1.80
```

### Steps

```sh
# 1. Clone this repo
git clone https://github.com/YourUser/gw2-mapcompletion
cd gw2-mapcompletion

# 2. Place third_party deps (see above)

# 3. Configure with CMake (Visual Studio 2022 / MSVC recommended)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 4. Build
cmake --build build --config Release

# 5. Install
#    Copy build/Release/MapCompletionTracker.dll  →
#         <GW2Install>/addons/MapCompletionTracker/MapCompletionTracker.dll
```

Nexus will auto-detect the DLL when it's placed in the `addons` subfolder.

---

## File layout

```
src/
  Nexus.h              – Nexus API structs & typedefs (from RaidcoreGG/RCGG-lib-nexus-api)
  Shared.h / .cpp      – Global APIDefs & MumbleLink pointers
  MapData.h            – All explorable map IDs, names, regions, completion strings
  CompletionTracker.h  – Thread-safe completion state + JSON persistence
  UI.h                 – ImGui window (progress bar, region groups, checkboxes, search)
  AddonMain.cpp        – DllMain, GetAddonDef, AddonLoad/Unload, event/keybind handlers
CMakeLists.txt
README.md
```

---

## Adding / updating maps

Open `src/MapData.h` and add entries to the `GetAllMaps()` function:

```cpp
{ 1234, "My New Map", "Region Name", "My New Map" },
//  ^id   ^display name  ^region        ^completionText (substring of chat msg)
```

The `completionText` field should match the map name as GW2 prints it in the
completion chat message, e.g. `"Dragon's End"` for Dragon's End.

---

## License

MIT – do whatever you want, attribution appreciated.
