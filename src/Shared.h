#pragma once

#include "Nexus.h"

// Global Nexus API pointer – set in AddonLoad, cleared in AddonUnload
extern AddonAPI_t*  APIDefs;

// Raw MumbleLink shared memory — public, accessible via DL_MUMBLE_LINK
extern MumbleLink*  MumbleLinkData;

// Parsed character name from MumbleLink identity JSON
extern char         g_currentCharName[20];
