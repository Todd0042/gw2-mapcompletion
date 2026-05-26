// =============================================================================
//  GW2 Map Completion Tracker - Nexus Addon
// =============================================================================

#include "Shared.h"
#include "CompletionTracker.h"
#include "MapData.h"
#include "UI.h"
#include "map_png.h"
#include "Nexus.h"

#include <imgui.h>
#include <windows.h>
#include <string>

AddonAPI_t* g_api = nullptr;

// ---------------------------------------------------------------------------
//  Forward declarations
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef();

void AddonLoad               (AddonAPI_t* aApi);
void AddonUnload             ();
void OnRender                ();
void OnRenderOptions         ();
void OnKeybind               (const char* aIdentifier, bool aIsRelease);
void OnMumbleIdentityUpdated (void* aEventArgs);
void OnTotalXPSourceEvent    (void* aEventArgs);
// ---------------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------------
static AddonDefinition_t  s_addonDef{};
static CompletionTracker  s_tracker;
static UI                 s_ui;
static HMODULE            s_hSelf           = nullptr;
static bool               s_initialized     = false;

// ---------------------------------------------------------------------------
//  DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        s_hSelf = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
//  GetAddonDef
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef()
{
    s_addonDef.Signature   = -19823;
    s_addonDef.APIVersion  = NEXUS_API_VERSION;
    s_addonDef.Name        = "Map Completion Tracker";
    s_addonDef.Version     = { 1, 1, 0, 0 };
    s_addonDef.Author      = "Todd0042";
    s_addonDef.Description = "Tracks GW2 map completion per character.";
    s_addonDef.Load        = AddonLoad;
    s_addonDef.Unload      = AddonUnload;
    s_addonDef.Flags       = AF_None;
    s_addonDef.Provider    = UP_GitHub;
    s_addonDef.UpdateLink  = "https://github.com/Todd0042/gw2-mapcompletion";
    return &s_addonDef;
}

// ---------------------------------------------------------------------------
//  AddonLoad
// ---------------------------------------------------------------------------
void AddonLoad(AddonAPI_t* aApi)
{
    APIDefs = aApi;
    g_api   = aApi;
    APIDefs->Log(LOGL_INFO, "MapCompletionTracker", "AddonLoad: start");

    (void)GetAllMaps();
    (void)GetMapsById();
    (void)GetMapsByName();

    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void* (*)(size_t, void*)) APIDefs->ImguiMalloc,
        (void  (*)(void*, void*))  APIDefs->ImguiFree
    );

    // Get raw MumbleLink for map ID display in UI
    MumbleLinkData = (MumbleLink*)APIDefs->DataLink_Get("DL_MUMBLE_LINK");

    APIDefs->GUI_Register(RT_Render,        OnRender);
    APIDefs->GUI_Register(RT_OptionsRender, OnRenderOptions);

    APIDefs->Events_Subscribe("EV_MUMBLE_IDENTITY_UPDATED", OnMumbleIdentityUpdated);
    APIDefs->Events_Subscribe("EV_XP:SourceEvent",           OnTotalXPSourceEvent);

    APIDefs->InputBinds_RegisterWithString(
        "KB_MAPCOMPLETION_TOGGLE",
        OnKeybind,
        "CTRL+M"
    );

    // Load icon from embedded PNG array using synchronous GetOrCreate.
    // This returns immediately with the texture or nullptr if it failed.
    APIDefs->Textures_GetOrCreateFromMemory("ICON_MAPCOMPLETION",       (void*)map_png, (uint64_t)map_png_len);
    APIDefs->Textures_GetOrCreateFromMemory("ICON_MAPCOMPLETION_HOVER",  (void*)map_png, (uint64_t)map_png_len);

    APIDefs->QuickAccess_Add(
        "QA_MAPCOMPLETION",         // shortcut identifier
        "ICON_MAPCOMPLETION",       // normal texture identifier
        "ICON_MAPCOMPLETION_HOVER", // hover texture identifier
        "KB_MAPCOMPLETION_TOGGLE",  // keybind to invoke on click
        "Map Completion Tracker"    // tooltip text
    );

    APIDefs->Log(LOGL_INFO, "MapCompletionTracker", "AddonLoad: complete");
}

// ---------------------------------------------------------------------------
//  AddonUnload
// ---------------------------------------------------------------------------
void AddonUnload()
{
    APIDefs->Events_Unsubscribe("EV_MUMBLE_IDENTITY_UPDATED", OnMumbleIdentityUpdated);
    APIDefs->Events_Unsubscribe("EV_XP:SourceEvent",           OnTotalXPSourceEvent);
    APIDefs->GUI_Deregister(OnRender);
    APIDefs->GUI_Deregister(OnRenderOptions);
    APIDefs->InputBinds_Deregister("KB_MAPCOMPLETION_TOGGLE");
    APIDefs->QuickAccess_Remove("QA_MAPCOMPLETION");

    APIDefs        = nullptr;
    MumbleLinkData = nullptr;
    s_initialized  = false;
    g_currentCharName[0] = '\0';
}

// ---------------------------------------------------------------------------
//  OnMumbleIdentityUpdated
//  Nexus fires this whenever the identity changes (character swap, map change).
//  aEventArgs is a MumbleIdentity* with the parsed fields.
// ---------------------------------------------------------------------------
void OnMumbleIdentityUpdated(void* aEventArgs)
{
    if (!aEventArgs) return;

    MumbleIdentity* identity = static_cast<MumbleIdentity*>(aEventArgs);

    if (identity->Name[0] == '\0') return;

    std::string name = identity->Name;

    // MumbleLink sometimes returns a file path on the character select screen.
    // Real character names can only contain letters and spaces, so reject
    // anything containing ':' or '\'.
    if (name.find(':') != std::string::npos || name.find('\\') != std::string::npos)
    {
        APIDefs->Log(LOGL_DEBUG, "MapCompletionTracker",
                     "Ignoring non-character MumbleLink name (file path).");
        return;
    }

    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf),
             "MumbleIdentityUpdated: name='%s' mapId=%u profession=%u",
             name.c_str(), identity->MapID, identity->Profession);
    APIDefs->Log(LOGL_DEBUG, "MapCompletionTracker", logBuf);

    strncpy(g_currentCharName, name.c_str(), sizeof(g_currentCharName) - 1);
    g_currentCharName[sizeof(g_currentCharName) - 1] = '\0';
    g_currentMapId = identity->MapID;

    if (s_initialized)
    {
        s_tracker.SetActiveCharacter(name);
        s_tracker.RegisterCharacter(name);
    }

    char infoLog[128];
    snprintf(infoLog, sizeof(infoLog),
             "MapCompletionTracker: active character -> '%s'.", name.c_str());
    APIDefs->Log(LOGL_INFO, "MapCompletionTracker", infoLog);
}

// ---------------------------------------------------------------------------
//  OnTotalXPSourceEvent
//  Subscribed to "EV_XP:SourceEvent" from the gw2-events-xp DLL (formerly
//  EV_TOTAL:XPSourceEvent from total-events). Fires whenever GW2 grants
//  XP from a known internal source. We only care about SourceCode == 13
//  (map completion). Layout matches gw2_events_xp.h::GW2XP_SourceEvent.
//
//  This event fires the moment the game grants map-completion XP — i.e. when
//  the player accepts the reward chest. It is more reliable than the popup
//  scanner (which only fires at level 80 with the visible 8001-XP popup
//  string) because the SourceCode is GW2's internal enum value, not a heuristic.
//
//  The event payload does NOT carry a map id, so we use the most-recent
//  Mumble map id (g_currentMapId, updated on every identity event).
// ---------------------------------------------------------------------------
static constexpr uint32_t TOTAL_XPSRC_MAP_COMPLETION = 13;  // mirrors gw2_events_xp.h::GW2XP_SRC_MAP_COMPLETION
void OnTotalXPSourceEvent(void* aEventArgs)
{
    if (!aEventArgs) return;
    // Layout: { uint32_t SourceCode; uint32_t PreXP; uint64_t PayloadAddr; }
    struct Payload { uint32_t SourceCode; uint32_t PreXP; uint64_t PayloadAddr; };
    const Payload* p = static_cast<const Payload*>(aEventArgs);

    if (p->SourceCode != TOTAL_XPSRC_MAP_COMPLETION) return;

    uint32_t mapId = g_currentMapId;
    if (mapId == 0) {
        if (APIDefs) APIDefs->Log(LOGL_WARNING, "MapCompletionTracker",
            "OnTotalXPSourceEvent: map completion fired but Mumble map id is 0");
        return;
    }

    // Resolve map id → MapInfo
    const auto& byId = GetMapsById();
    auto it = byId.find(mapId);
    if (it == byId.end()) {
        if (APIDefs) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "OnTotalXPSourceEvent: unknown map id %u (XP source event)", mapId);
            APIDefs->Log(LOGL_WARNING, "MapCompletionTracker", buf);
        }
        return;
    }
    const std::string& mapName = it->second->name;

    std::string charNow = g_currentCharName;
    if (charNow.empty()) charNow = "(unknown)";

    {
        std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
        if (g_pendingMapComp.mapId != 0 && g_pendingMapComp.mapId != mapId) return;
        g_pendingMapComp.mapId     = mapId;
        g_pendingMapComp.mapName   = mapName;
        g_pendingMapComp.character = charNow;
    }

    if (APIDefs) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "Auto-detected map completion (via XPSourceEvent): map='%s' (id=%u) character='%s' preXP=%u",
                 mapName.c_str(), mapId, charNow.c_str(), p->PreXP);
        APIDefs->Log(LOGL_INFO, "MapCompletionTracker", buf);
    }
}

// ---------------------------------------------------------------------------
//  OnRender
// ---------------------------------------------------------------------------
void OnRender()
{
    if (!s_initialized)
    {
        const char* addonDirCStr = APIDefs->Paths_GetAddonDirectory("MapCompletionTracker");
        std::string addonDir     = addonDirCStr ? addonDirCStr : "";

        if (!addonDir.empty())
            CreateDirectoryA(addonDir.c_str(), nullptr);

        s_tracker.Init(addonDir);
        s_ui.Init(&s_tracker, addonDir.empty() ? "completion.json" : addonDir + "\\completion.json");
        s_initialized = true;

        // If MumbleLink wasn't available at AddonLoad, retry now
        if (!MumbleLinkData)
            MumbleLinkData = (MumbleLink*)APIDefs->DataLink_Get("DL_MUMBLE_LINK");

        // If we already have a character name from an earlier event, apply it
        if (g_currentCharName[0] != '\0')
            s_tracker.SetActiveCharacter(g_currentCharName);

        APIDefs->Log(LOGL_INFO, "MapCompletionTracker", "Initialised successfully.");
    }

    // Always render the auto-detected completion popup, even when the main
    // window is closed — it's a standalone bottom-right confirmation.
    s_ui.RenderPendingMapCompletionPopup();

    s_ui.Render();
}

void OnRenderOptions()
{
    if (!s_initialized) return;
    s_ui.RenderOptions();
}

// ---------------------------------------------------------------------------
//  Keybind handler
// ---------------------------------------------------------------------------
void OnKeybind(const char* aIdentifier, bool aIsRelease)
{
    if (aIsRelease) return;
    if (strcmp(aIdentifier, "KB_MAPCOMPLETION_TOGGLE") == 0)
        s_ui.ToggleVisible();
}
