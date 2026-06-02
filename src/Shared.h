#pragma once

#include "Nexus.h"

// Global Nexus API pointer – set in AddonLoad, cleared in AddonUnload
extern AddonAPI_t*  APIDefs;

// Raw MumbleLink shared memory — public, accessible via DL_MUMBLE_LINK
extern MumbleLink*  MumbleLinkData;

// Parsed character name from MumbleLink identity JSON
extern char         g_currentCharName[20];

// Most-recent MumbleLink-reported map ID. Updated on every identity event.
// Retained for telemetry; the new EV_REWARD:MapCompletion event carries
// the map id in its payload, so we no longer need it for detection.
extern uint32_t     g_currentMapId;

// Pending auto-detected map completion. Populated when EV_REWARD:MapCompletion
// fires (from the char-events addon). The popup auto-marks the map complete
// for the active character on first render and shows a confirmation toast
// with a Revert button. The toast self-destructs after 5 seconds.
//
//   firedAt        — GetTickCount() when the event arrived; used for the
//                    5-second auto-destroy.
//   markedComplete — true after the popup has applied the auto-mark. Prevents
//                    re-marking on subsequent renders.
#include <atomic>
#include <string>
#include <mutex>
#include <windows.h>  // DWORD, GetTickCount
struct PendingMapComp {
    uint32_t    mapId          = 0;     // 0 = nothing pending
    std::string mapName;                  // resolved from MapId
    std::string character;                // active character at fire time
    DWORD       firedAt        = 0;     // GetTickCount() snapshot for 5-s TTL
    bool        markedComplete = false; // popup has applied the auto-mark
};
extern PendingMapComp g_pendingMapComp;
extern std::mutex     g_pendingMapCompMtx;

// Toast lifetime in milliseconds. After this elapses, the popup self-destroys
// and the auto-mark stays in place (user didn't click Revert).
constexpr DWORD kMapCompToastTtlMs = 5000;
