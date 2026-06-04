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
void OnRewardMapCompletion   (void* aEventArgs);
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
    s_addonDef.Version     = { 1, 4, 0, 0 };
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

    APIDefs->Events_Subscribe("EV_MUMBLE_IDENTITY_UPDATED",   OnMumbleIdentityUpdated);
    // Auto-completion via the char-events broadcast was removed (ANet/Nexus ban broadcasting
    // internal data). Public build: manual checklist + GW2 API only. The memory-scanning
    // build (private 'memory-scan' branch) self-detects completion via an embedded hook.

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
    APIDefs->Events_Unsubscribe("EV_MUMBLE_IDENTITY_UPDATED",   OnMumbleIdentityUpdated);
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
//  OnRewardMapCompletion
//  Subscribed to "EV_REWARD:MapCompletion" from the char-events DLL. Fires
//  the moment GW2 receives a CHAR_REWARD_MAP_COMPLETION (rewardType=1) message
//  from the server — BEFORE the player accepts the popup chest. Level-
//  independent and standalone (no ArcDPS required, no XP-delta heuristic).
//
//  Payload layout (mirrors char_events.h::GW2Reward_Any):
//      { uint32_t RewardType; uint32_t MapId; }
//
//  Behavior: this handler sets g_pendingMapComp with the map id + character.
//  The UI thread (RenderPendingMapCompletionPopup) reads it, auto-marks the
//  map complete for the active character on first render, and shows a
//  5-second toast with a Revert button.
// ---------------------------------------------------------------------------
void OnRewardMapCompletion(void* aEventArgs)
{
    if (!aEventArgs) return;
    // Matches char_events.h::GW2Reward_Any exactly: RewardType FIRST,
    // MapId SECOND. Don't reorder — the layout is wire-compatible.
    struct Payload { uint32_t RewardType; uint32_t MapId; };
    const Payload* p = static_cast<const Payload*>(aEventArgs);

    // Defensive: char-events publishes the named event only for type 1,
    // but the payload still carries the type code for symmetry with EV_REWARD:Any.
    if (p->RewardType != 1) return;

    uint32_t mapId = p->MapId;
    if (mapId == 0) {
        if (APIDefs) APIDefs->Log(LOGL_WARNING, "MapCompletionTracker",
            "OnRewardMapCompletion: payload MapId is 0");
        return;
    }

    // Resolve map id → MapInfo for the toast text. Unknown id is logged but
    // we still mark complete (the tracker doesn't require a known name).
    const auto& byId = GetMapsById();
    auto it = byId.find(mapId);
    std::string mapName;
    if (it != byId.end()) {
        mapName = it->second->name;
    } else {
        mapName = "(unknown map)";
        if (APIDefs) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "OnRewardMapCompletion: unknown map id %u", mapId);
            APIDefs->Log(LOGL_WARNING, "MapCompletionTracker", buf);
        }
    }

    std::string charNow = g_currentCharName;
    if (charNow.empty()) charNow = "(unknown)";

    {
        std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
        g_pendingMapComp.mapId          = mapId;
        g_pendingMapComp.mapName        = mapName;
        g_pendingMapComp.character      = charNow;
        g_pendingMapComp.firedAt        = GetTickCount();
        g_pendingMapComp.markedComplete = false;
    }

    if (APIDefs) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "Auto-detected map completion: map='%s' (id=%u) character='%s'",
                 mapName.c_str(), mapId, charNow.c_str());
        APIDefs->Log(LOGL_INFO, "MapCompletionTracker", buf);
    }
}

// ---------------------------------------------------------------------------
//  Category-change disclaimer (INTERIM PUBLIC BUILD ONLY)
//  Nexus requires notice before an addon changes category. On SWITCH_DATE this
//  addon moves to the "memory reading" category (auto-detects map completion by
//  reading game memory). Shown once per session until acknowledged. Remove this
//  block when the memory-scanning build is merged public on/after SWITCH_DATE.
// ---------------------------------------------------------------------------
static const char* SWITCH_DATE = "2026-07-07";
static void RenderCategoryDisclaimer()
{
    static bool s_ack    = false;   // user confirmed + closed it this session
    static bool s_opened = false;   // popup opened once
    static bool s_read   = false;   // read-confirmation checkbox
    if (s_ack) return;

    const char* kPopupId = "Map Completion Tracker \xE2\x80\x94 Important Notice###MapCompNotice";
    if (!s_opened) { ImGui::OpenPopup(kPopupId); s_opened = true; }

    // Center it; modal so the user can't ignore it — they must read + close manually.
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped(
            "On %s, Map Completion Tracker will change to the \"memory reading\" addon "
            "category: it will read map-completion status directly from the game's memory "
            "to auto-detect completion.\n\n"
            "This build does NOT read game memory \xE2\x80\x94 it uses the manual checklist and "
            "the official Guild Wars 2 API only.\n\n"
            "If you are not comfortable with an addon reading game memory, please uninstall "
            "Map Completion Tracker before %s.",
            SWITCH_DATE, SWITCH_DATE);
        ImGui::Separator();
        ImGui::Checkbox("I have read and understand this notice.", &s_read);
        ImGui::Spacing();
        if (s_read) {
            if (ImGui::Button("Close")) { s_ack = true; ImGui::CloseCurrentPopup(); }
        } else {
            ImGui::TextDisabled("Tick the box above to close this notice.");
        }
        ImGui::EndPopup();
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

    // Interim category-change notice (public build only).
    RenderCategoryDisclaimer();

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
