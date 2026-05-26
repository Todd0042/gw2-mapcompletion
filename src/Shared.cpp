#include "Shared.h"

AddonAPI_t* APIDefs        = nullptr;
MumbleLink* MumbleLinkData = nullptr;
char        g_currentCharName[20] = {};
uint32_t    g_currentMapId         = 0;

PendingMapComp g_pendingMapComp;
std::mutex     g_pendingMapCompMtx;
