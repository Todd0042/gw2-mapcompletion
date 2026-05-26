#pragma once

#include "Nexus.h"

// Global Nexus API pointer – set in AddonLoad, cleared in AddonUnload
extern AddonAPI_t*  APIDefs;

// Raw MumbleLink shared memory — public, accessible via DL_MUMBLE_LINK
extern MumbleLink*  MumbleLinkData;

// Parsed character name from MumbleLink identity JSON
extern char         g_currentCharName[20];

// Most-recent MumbleLink-reported map ID. Updated on every identity event.
// Used by OnTotalXPSourceEvent so we can look up the just-completed map when
// the XPSourceEvent fires (the event payload has no map id of its own).
extern uint32_t     g_currentMapId;

// Pending auto-detected map completion event from total-events DLL.
// When EV_TOTAL:MapCompleted fires, this is populated and the UI shows a
// confirmation modal asking whether to mark the map complete for the active
// character. Cleared (mapId=0) once the user responds.
#include <atomic>
#include <string>
#include <mutex>
struct PendingMapComp {
    uint32_t    mapId   = 0;      // 0 = nothing pending
    std::string mapName;          // raw name from the event payload
    std::string character;        // character at the time of detection (Mumble)
};
extern PendingMapComp g_pendingMapComp;
extern std::mutex     g_pendingMapCompMtx;
