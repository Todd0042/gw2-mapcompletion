#pragma once

#include "CompletionTracker.h"
#include "MapData.h"
#include "Shared.h"
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>

class UI
{
public:
    void Init(CompletionTracker* tracker, const std::string& savePath)
    {
        m_tracker  = tracker;
        m_savePath = savePath;
    }

    void ToggleVisible() { m_visible = !m_visible; }
    bool IsVisible()     { return m_visible; }

    // ================================================================
    //  Main window
    // ================================================================
    void Render()
    {
        if (!m_visible) return;

        const std::string& activeChar = m_tracker->GetActiveCharacter();
        std::vector<std::string> chars = m_tracker->GetCharacterNames();

        bool activeIsReal = !activeChar.empty() && activeChar != "__unknown__";
        if (!m_viewedCharacterSet && activeIsReal)
        {
            m_viewedCharacter    = activeChar;
            m_viewedCharacterSet = true;
        }

        std::string viewChar = m_viewedCharacter;
        if (viewChar.empty() || viewChar == "__unknown__")
        {
            if (activeIsReal)        viewChar = activeChar;
            else if (!chars.empty()) viewChar = chars[0];
        }

        const auto& allMaps = GetAllMaps();
        const auto& tabs    = GetTabOrder();

        // --- Aggregate stats ------------------------------------------------
        size_t totalAll = allMaps.size();
        size_t doneAll  = 0;
        for (const auto& m : allMaps)
            if (m_tracker->IsCompleteFor(viewChar, m.id)) doneAll++;
        float progressAll = totalAll > 0 ? (float)doneAll / (float)totalAll : 0.0f;

        std::unordered_map<std::string, std::pair<size_t,size_t>> tabStats;
        std::unordered_map<std::string, std::pair<size_t,size_t>> regionStats;
        for (const auto& m : allMaps)
        {
            auto& ts = tabStats[m.tab];
            ts.second++;
            if (m_tracker->IsCompleteFor(viewChar, m.id)) ts.first++;
            std::string key = m.tab + "\x1f" + m.region;
            auto& rs = regionStats[key];
            rs.second++;
            if (m_tracker->IsCompleteFor(viewChar, m.id)) rs.first++;
        }
        size_t regions100 = 0, totalRegions = regionStats.size();
        for (auto& kv : regionStats)
            if (kv.second.second > 0 && kv.second.first == kv.second.second) regions100++;
        size_t exps100 = 0, totalExps = tabStats.size();
        for (auto& kv : tabStats)
            if (kv.second.second > 0 && kv.second.first == kv.second.second) exps100++;

        ImGui::SetNextWindowSize(ImVec2(640, 800), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(440, 500), ImVec2(1280, 1600));

        // --- Window-scoped style overrides ----------------------------------
        ImGui::PushStyleColor(ImGuiCol_WindowBg,         ColBgDeep());          // 1
        ImGui::PushStyleColor(ImGuiCol_TitleBg,          ImVec4(0.10f,0.11f,0.13f,1)); // 2
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive,    ImVec4(0.13f,0.15f,0.18f,1)); // 3
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.08f,0.09f,0.11f,1)); // 4
        ImGui::PushStyleColor(ImGuiCol_Separator,        ColSep());             // 5
        ImGui::PushStyleColor(ImGuiCol_Tab,              ImVec4(0.12f,0.14f,0.17f,1)); // 6
        ImGui::PushStyleColor(ImGuiCol_TabActive,        ImVec4(0.20f,0.23f,0.27f,1)); // 7
        ImGui::PushStyleColor(ImGuiCol_TabHovered,       ImVec4(0.26f,0.30f,0.35f,1)); // 8
        ImGui::PushStyleColor(ImGuiCol_Border,           ColSep());             // 9
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4(0.13f,0.15f,0.18f,1)); // 10
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   ImVec4(0.17f,0.20f,0.24f,1)); // 11
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    ImVec4(0.20f,0.23f,0.27f,1)); // 12
        ImGui::PushStyleColor(ImGuiCol_Button,           ImVec4(0.18f,0.21f,0.25f,1)); // 13
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,    ImVec4(0.26f,0.30f,0.35f,1)); // 14
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,     ImVec4(0.30f,0.34f,0.40f,1)); // 15
        ImGui::PushStyleColor(ImGuiCol_Header,           ImVec4(0.18f,0.21f,0.25f,1)); // 16
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,    ImVec4(0.22f,0.25f,0.29f,1)); // 17
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,     ImVec4(0.26f,0.30f,0.35f,1)); // 18
        ImGui::PushStyleColor(ImGuiCol_CheckMark,        ColAccent());          // 19
        ImGui::PushStyleColor(ImGuiCol_Text,             ImVec4(0.92f,0.93f,0.94f,1)); // 20
        const int kStyleColorCount = 20;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));       // 1
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);                 // 2
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(8, 6));         // 3
        const int kStyleVarCount = 3;

        if (!ImGui::Begin("Map Completion Tracker", &m_visible, ImGuiWindowFlags_None))
        {
            ImGui::End();
            ImGui::PopStyleVar  (kStyleVarCount);
            ImGui::PopStyleColor(kStyleColorCount);
            return;
        }

        bool isViewingCurrent = activeIsReal && (viewChar == activeChar);

        // --- Hero header with map.png backdrop ------------------------------
        DrawHero(viewChar, isViewingCurrent,
                 doneAll, totalAll,
                 regions100, totalRegions,
                 exps100, totalExps);

        ImGui::Spacing();

        // --- Resolve active expansion (clickable strip below selects it) ----
        if (!tabs.empty())
        {
            bool valid = false;
            for (const auto& t : tabs) if (t == m_activeTab) { valid = true; break; }
            if (!valid) m_activeTab = tabs.front();
        }

        // --- Expansion strip (clickable cards — replaces the tab bar) -------
        DrawExpansionStrip(tabs, tabStats);

        ImGui::Spacing();

        // --- Controls (character / search / filter / sort / copy) -----------
        DrawControlsRow(viewChar, activeChar, activeIsReal, chars,
                        doneAll, totalAll, progressAll);

        // --- Current map banner ---------------------------------------------
        DrawCurrentMapBanner(viewChar);

        ImGui::Spacing();

        bool canEdit = !viewChar.empty() && viewChar != "__unknown__";
        if (!canEdit)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.62f, 0.22f, 1));
            ImGui::TextUnformatted("Log in to a character to track progress.");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        std::string searchLower = m_searchBuf;
        for (auto& c : searchLower) c = (char)tolower((unsigned char)c);

        // --- Active expansion content ---------------------------------------
        if (!m_activeTab.empty())
        {
            auto tsit = tabStats.find(m_activeTab);
            size_t tabDone  = tsit != tabStats.end() ? tsit->second.first  : 0;
            size_t tabTotal = tsit != tabStats.end() ? tsit->second.second : 0;
            float tabFrac = tabTotal > 0 ? (float)tabDone / (float)tabTotal : 0.0f;

            // Active-expansion title bar (replaces the old tab label)
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 hp = ImGui::GetCursorScreenPos();
            float hw = ImGui::GetContentRegionAvail().x;
            const float titleH = 22.0f;
            ImVec4 accent = ExpansionColor(m_activeTab);
            dl->AddRectFilled(ImVec2(hp.x, hp.y + (titleH - 14) * 0.5f),
                              ImVec2(hp.x + 3, hp.y + (titleH + 14) * 0.5f),
                              ToU32(accent), 1.5f);
            dl->AddText(ImVec2(hp.x + 10, hp.y + (titleH - ImGui::GetTextLineHeight()) * 0.5f),
                        IM_COL32(232, 232, 232, 255), m_activeTab.c_str());
            char cnt[48];
            snprintf(cnt, sizeof(cnt), "%zu / %zu", tabDone, tabTotal);
            ImVec2 cts = ImGui::CalcTextSize(cnt);
            dl->AddText(ImVec2(hp.x + hw - cts.x,
                               hp.y + (titleH - cts.y) * 0.5f),
                        ToU32(ColTextDim()), cnt);
            ImGui::Dummy(ImVec2(hw, titleH));

            DrawSegmentedProgress(tabFrac, 10.0f, accent);
            ImGui::Spacing();

            ImGui::BeginChild("##mapslist", ImVec2(0, 0), false,
                              ImGuiWindowFlags_None);

            std::vector<std::string> regions;
            {
                std::unordered_set<std::string> seen;
                for (const auto& m : allMaps)
                    if (m.tab == m_activeTab && seen.insert(m.region).second)
                        regions.push_back(m.region);
            }
            bool multiRegion = regions.size() > 1;

            for (const auto& region : regions)
            {
                std::vector<const MapInfo*> rowMaps;
                for (const auto& m : allMaps)
                    if (m.tab == m_activeTab && m.region == region
                        && PassesFilter(viewChar, m, searchLower))
                        rowMaps.push_back(&m);
                if (rowMaps.empty()) continue;

                SortMapList(viewChar, rowMaps);

                std::string rkey = m_activeTab + "\x1f" + region;
                auto rs = regionStats.find(rkey);
                size_t rd = rs != regionStats.end() ? rs->second.first  : 0;
                size_t rt = rs != regionStats.end() ? rs->second.second : 0;

                if (multiRegion)
                {
                    DrawRegionCard(region, rd, rt,
                                   viewChar, canEdit, rowMaps);
                }
                else
                {
                    for (const auto* m : rowMaps)
                    {
                        bool done = m_tracker->IsCompleteFor(viewChar, m->id);
                        DrawMapRow(viewChar, *m, done, canEdit);
                    }
                }
            }

            ImGui::EndChild();
        }

        ImGui::End();
        ImGui::PopStyleVar  (kStyleVarCount);
        ImGui::PopStyleColor(kStyleColorCount);
    }

    // ================================================================
    //  Options panel
    // ================================================================
    void RenderOptions()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ColAccent());
        ImGui::TextUnformatted("Map Completion Tracker");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Characters are tracked automatically via MumbleLink.");
        ImGui::TextDisabled("Log in to each character at least once to add it to the list.");
        ImGui::Spacing();
        ImGui::TextDisabled("Keybind: set via Nexus keybind settings (KB_MAPCOMPLETION_TOGGLE)");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped(
            "Auto-detect map completion: install the char-events addon "
            "(https://github.com/Todd0042/char-events). When loaded, the "
            "tracker auto-marks the active character's completion the moment "
            "GW2 receives the chest from the server and shows a 5-second "
            "revert toast. Without char-events, this tracker still works "
            "as a manual checklist.");
    }

    // ================================================================
    //  Auto-detected completion toast (bottom-right celebration)
    //  Custom-painted backdrop with gold border, gradient, fade-in/out.
    //  Big-font map name via Nexus FontBig when available.
    // ================================================================
    void RenderPendingMapCompletionPopup()
    {
        uint32_t    mapId;
        std::string mapName, character;
        DWORD       firedAt;
        bool        alreadyMarked;
        {
            std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
            mapId         = g_pendingMapComp.mapId;
            mapName       = g_pendingMapComp.mapName;
            character     = g_pendingMapComp.character;
            firedAt       = g_pendingMapComp.firedAt;
            alreadyMarked = g_pendingMapComp.markedComplete;
        }
        if (mapId == 0) return;

        std::string applyTo = character;
        if (applyTo.empty() || applyTo == "(unknown)" || applyTo == "__unknown__")
        {
            if (g_currentCharName[0]) applyTo = g_currentCharName;
        }

        DWORD elapsed = GetTickCount() - firedAt;
        if (elapsed >= kMapCompToastTtlMs)
        {
            std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
            g_pendingMapComp = {};
            return;
        }

        if (!alreadyMarked && m_tracker
            && !applyTo.empty() && applyTo != "(unknown)" && applyTo != "__unknown__")
        {
            m_tracker->MarkCompleteFor(applyTo, mapId);
            std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
            g_pendingMapComp.markedComplete = true;
        }

        // Alpha envelope: smooth fade-in, hold, fade-out.
        const DWORD fadeInMs  = 260;
        const DWORD fadeOutMs = 900;
        float alpha = 1.0f;
        if (elapsed < fadeInMs)
            alpha = (float)elapsed / (float)fadeInMs;
        else if (elapsed > kMapCompToastTtlMs - fadeOutMs)
            alpha = (float)(kMapCompToastTtlMs - elapsed) / (float)fadeOutMs;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        int alphaB = (int)(alpha * 255.0f);

        const ImGuiIO& io = ImGui::GetIO();
        const float w = 460.0f, h = 178.0f;
        const float margin = 24.0f, rightOffset = 75.0f;
        ImVec2 pos(io.DisplaySize.x - w - margin - rightOffset,
                   io.DisplaySize.y - h - margin);
        ImGui::SetNextWindowPos (pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f); // we paint our own background

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        bool open = true;
        if (!ImGui::Begin("##MapCompletedToast", &open, flags))
        {
            ImGui::End();
            ImGui::PopStyleVar(2);
            return;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 tl = ImGui::GetWindowPos();
        ImVec2 br(tl.x + w, tl.y + h);

        // Layered backdrop: dark card → warm gold highlight at top → border.
        dl->AddRectFilled(tl, br, IM_COL32(10, 12, 16, (int)(245 * alpha)), 9.0f);
        dl->AddRectFilledMultiColor(
            ImVec2(tl.x, tl.y),
            ImVec2(br.x, tl.y + h * 0.55f),
            IM_COL32(70, 50, 18, (int)(90 * alpha)),
            IM_COL32(70, 50, 18, (int)(90 * alpha)),
            IM_COL32(10, 12, 16, 0),
            IM_COL32(10, 12, 16, 0));
        dl->AddRect(tl, br, IM_COL32(245, 200, 90, (int)(225 * alpha)),
                    9.0f, ImDrawCornerFlags_All, 2.0f);
        dl->AddRectFilled(ImVec2(tl.x + 8, tl.y + 3),
                          ImVec2(br.x - 8, tl.y + 6),
                          IM_COL32(245, 200, 90, alphaB), 1.5f);

        const float padX = 20.0f, padY = 16.0f;

        // Header tag
        dl->AddText(ImVec2(tl.x + padX, tl.y + padY),
                    IM_COL32(247, 207, 92, alphaB),
                    "MAP COMPLETED!");

        // Map name (large font from Nexus when available).
        NexusLinkData_t* link = APIDefs ?
            (NexusLinkData_t*)APIDefs->DataLink_Get("DL_NEXUS_LINK") : nullptr;
        ImFont* big = link ? (ImFont*)link->FontBig : nullptr;
        ImVec2 namePos(tl.x + padX, tl.y + padY + 20);
        if (big)
        {
            dl->AddText(big, big->FontSize, namePos,
                        IM_COL32(247, 247, 247, alphaB), mapName.c_str());
        }
        else
        {
            // Fallback: use default font scaled up via two-pass with shadow.
            dl->AddText(ImVec2(namePos.x + 1, namePos.y + 1),
                        IM_COL32(0, 0, 0, alphaB), mapName.c_str());
            dl->AddText(namePos,
                        IM_COL32(247, 247, 247, alphaB), mapName.c_str());
        }

        // Character + map id
        float infoY = tl.y + padY + 64;
        char info[160];
        snprintf(info, sizeof(info), "Character: %s     Map ID: %u",
                 applyTo.empty() ? "(unknown)" : applyTo.c_str(), mapId);
        dl->AddText(ImVec2(tl.x + padX, infoY),
                    IM_COL32(170, 174, 180, alphaB), info);

        // Countdown
        DWORD remainingMs = (elapsed < kMapCompToastTtlMs)
                          ? (kMapCompToastTtlMs - elapsed) : 0;
        char count[96];
        snprintf(count, sizeof(count),
                 "Auto-marked complete. Dismissing in %us.",
                 (unsigned)((remainingMs + 999) / 1000));
        dl->AddText(ImVec2(tl.x + padX, infoY + 18),
                    IM_COL32(130, 134, 140, alphaB), count);

        // Buttons row (Revert / Dismiss)
        const float btnY = tl.y + h - 36;
        ImGui::SetCursorScreenPos(ImVec2(tl.x + padX, btnY));

        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(64, 30, 30, alphaB));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(96, 42, 42, alphaB));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(120, 56, 56, alphaB));
        ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(240, 220, 220, alphaB));
        bool clickedRevert = ImGui::Button("Revert", ImVec2(96, 26));
        ImGui::PopStyleColor(4);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(28, 32, 38, alphaB));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(46, 52, 60, alphaB));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(60, 68, 78, alphaB));
        ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(220, 222, 226, alphaB));
        bool clickedDismiss = ImGui::Button("Dismiss", ImVec2(96, 26));
        ImGui::PopStyleColor(4);

        ImGui::End();
        ImGui::PopStyleVar(2);

        if (clickedRevert)
        {
            if (!applyTo.empty() && applyTo != "(unknown)"
                && applyTo != "__unknown__" && m_tracker)
            {
                m_tracker->MarkIncompleteFor(applyTo, mapId);
            }
            std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
            g_pendingMapComp = {};
        }
        else if (clickedDismiss)
        {
            std::lock_guard<std::mutex> lk(g_pendingMapCompMtx);
            g_pendingMapComp = {};
        }
    }

private:
    // ----- Palette -----------------------------------------------------------
    static ImVec4 ColAccent()    { return ImVec4(0.96f, 0.81f, 0.36f, 1); }
    static ImVec4 ColAccentDim() { return ImVec4(0.72f, 0.58f, 0.25f, 1); }
    static ImVec4 ColBgDeep()    { return ImVec4(0.06f, 0.07f, 0.09f, 0.97f); }
    static ImVec4 ColCard()      { return ImVec4(0.11f, 0.13f, 0.16f, 0.92f); }
    static ImVec4 ColCardHead()  { return ImVec4(0.16f, 0.19f, 0.23f, 1.00f); }
    static ImVec4 ColDone()      { return ImVec4(0.45f, 0.85f, 0.40f, 1); }
    static ImVec4 ColDoneDim()   { return ImVec4(0.20f, 0.55f, 0.25f, 1); }
    static ImVec4 ColTextDim()   { return ImVec4(0.72f, 0.74f, 0.77f, 1); }
    static ImVec4 ColTextMute()  { return ImVec4(0.50f, 0.52f, 0.55f, 1); }
    static ImVec4 ColSep()       { return ImVec4(0.20f, 0.22f, 0.25f, 1); }
    static ImVec4 ColPill()      { return ImVec4(0.18f, 0.20f, 0.23f, 1); }

    static ImU32 ToU32 (ImVec4 v)             { return ImGui::ColorConvertFloat4ToU32(v); }
    static ImU32 ToU32A(ImVec4 v, float a)    { v.w *= a; return ToU32(v); }

    static ImVec4 ExpansionColor(const std::string& tab)
    {
        if (tab == TAB_CORE)     return ImVec4(0.45f, 0.65f, 0.95f, 1);
        if (tab == TAB_LW12)     return ImVec4(0.85f, 0.78f, 0.55f, 1);
        if (tab == TAB_HOT)      return ImVec4(0.55f, 0.88f, 0.45f, 1);
        if (tab == TAB_POF)      return ImVec4(0.95f, 0.65f, 0.30f, 1);
        if (tab == TAB_ICEBROOD) return ImVec4(0.55f, 0.85f, 0.95f, 1);
        if (tab == TAB_EOD)      return ImVec4(0.40f, 0.90f, 0.70f, 1);
        if (tab == TAB_SOTO)     return ImVec4(0.55f, 0.55f, 0.95f, 1);
        if (tab == TAB_JANTHIR)  return ImVec4(0.75f, 0.50f, 0.95f, 1);
        if (tab == TAB_VOE)      return ImVec4(0.95f, 0.50f, 0.75f, 1);
        return ColAccent();
    }

    static const char* TabShortLabel(const std::string& t)
    {
        if (t == TAB_CORE)     return "Core";
        if (t == TAB_LW12)     return "LW1-2";
        if (t == TAB_HOT)      return "HoT";
        if (t == TAB_POF)      return "PoF";
        if (t == TAB_ICEBROOD) return "IBS";
        if (t == TAB_EOD)      return "EoD";
        if (t == TAB_SOTO)     return "SotO";
        if (t == TAB_JANTHIR)  return "JW";
        if (t == TAB_VOE)      return "VoE";
        return t.c_str();
    }

    ImTextureID FetchMapTexture()
    {
        if (!APIDefs) return nullptr;
        Texture_t* t = APIDefs->Textures_Get("ICON_MAPCOMPLETION");
        return t ? (ImTextureID)t->Resource : nullptr;
    }

    // ----- Hero --------------------------------------------------------------
    void DrawHero(const std::string& viewChar, bool isCurrent,
                  size_t doneAll, size_t totalAll,
                  size_t regions100, size_t totalRegions,
                  size_t exps100, size_t totalExps)
    {
        const float heroH = 162.0f;
        ImDrawList* dl    = ImGui::GetWindowDrawList();
        ImVec2 origin     = ImGui::GetCursorScreenPos();
        float w           = ImGui::GetContentRegionAvail().x;
        ImVec2 br(origin.x + w, origin.y + heroH);

        // Base
        dl->AddRectFilled(origin, br, IM_COL32(14, 16, 20, 255), 6.0f);

        // Map.png backdrop (faded)
        ImTextureID tex = FetchMapTexture();
        if (tex)
        {
            dl->AddImageRounded(tex, origin, br,
                                ImVec2(0, 0), ImVec2(1, 1),
                                IM_COL32(255, 255, 255, 58),
                                6.0f, ImDrawCornerFlags_All);
        }

        // Dark overlay top→bottom for legibility
        dl->AddRectFilledMultiColor(
            origin, br,
            IM_COL32(10, 12, 16, 170),
            IM_COL32(10, 12, 16, 170),
            IM_COL32(2,  4,  6,  240),
            IM_COL32(2,  4,  6,  240));

        // Gold accents
        dl->AddRectFilled(ImVec2(origin.x, origin.y + 14),
                          ImVec2(origin.x + 4, origin.y + 58),
                          ToU32(ColAccent()), 2.0f);
        dl->AddLine(ImVec2(origin.x + 10, br.y - 1),
                    ImVec2(br.x - 10,     br.y - 1),
                    ToU32A(ColAccent(), 0.55f), 1.0f);

        const float padX = 20.0f, padY = 14.0f;

        // Title tag
        dl->AddText(ImVec2(origin.x + padX, origin.y + padY),
                    ToU32(ColAccent()), "MAP COMPLETION");

        // Character line (large font via NexusLink FontBig when available)
        std::string charLine = (viewChar.empty() || viewChar == "__unknown__")
            ? std::string("(no character)")
            : viewChar + (isCurrent ? "   [current]" : "");
        NexusLinkData_t* link = APIDefs ?
            (NexusLinkData_t*)APIDefs->DataLink_Get("DL_NEXUS_LINK") : nullptr;
        ImFont* big = link ? (ImFont*)link->FontBig : nullptr;
        ImVec2 charPos(origin.x + padX, origin.y + padY + 18);
        if (big)
            dl->AddText(big, big->FontSize, charPos,
                        IM_COL32(247, 247, 247, 255), charLine.c_str());
        else
            dl->AddText(charPos, IM_COL32(247, 247, 247, 255), charLine.c_str());

        // Progress bar
        const float barH = 22.0f;
        ImVec2 pbP(origin.x + padX, br.y - padY - barH - 22);
        float pbW = w - padX * 2;
        ImVec2 pbP1(pbP.x + pbW, pbP.y + barH);
        float rad = barH * 0.5f;
        dl->AddRectFilled(pbP, pbP1, IM_COL32(18, 20, 26, 240), rad);
        float frac = totalAll > 0 ? (float)doneAll / (float)totalAll : 0.0f;
        if (frac > 0.0f)
        {
            float fillW = pbW * (frac > 1.0f ? 1.0f : frac);
            if (fillW < rad * 2.0f) fillW = rad * 2.0f;
            ImVec2 q1(pbP.x + fillW, pbP.y + barH);
            dl->AddRectFilledMultiColor(
                pbP, q1,
                ToU32(ColAccent()), ToU32(ColDone()),
                ToU32(ColDone()),    ToU32(ColAccent()));
        }
        dl->AddRect(pbP, pbP1, ToU32A(ColAccent(), 0.6f), rad,
                    ImDrawCornerFlags_All, 1.0f);

        char pbLabel[80];
        snprintf(pbLabel, sizeof(pbLabel),
                 "%zu / %zu     %.1f %%", doneAll, totalAll, frac * 100.0f);
        ImVec2 ts = ImGui::CalcTextSize(pbLabel);
        ImVec2 tp(pbP.x + (pbW - ts.x) * 0.5f,
                  pbP.y + (barH - ts.y) * 0.5f);
        dl->AddText(ImVec2(tp.x + 1, tp.y + 1),
                    IM_COL32(0, 0, 0, 220), pbLabel);
        dl->AddText(tp, IM_COL32(252, 252, 252, 250), pbLabel);

        // Stats row beneath bar
        float statY = pbP1.y + 8;
        char stat1[64], stat2[64];
        snprintf(stat1, sizeof(stat1),
                 "%zu / %zu regions complete", regions100, totalRegions);
        snprintf(stat2, sizeof(stat2),
                 "%zu / %zu expansions complete", exps100, totalExps);
        ImVec2 s1p(origin.x + padX, statY);
        dl->AddCircleFilled(ImVec2(s1p.x + 4, s1p.y + 7),
                            3.5f, ToU32(ColDone()));
        dl->AddText(ImVec2(s1p.x + 14, s1p.y),
                    IM_COL32(184, 188, 192, 255), stat1);
        float s1w = 14 + ImGui::CalcTextSize(stat1).x + 26;
        ImVec2 s2p(s1p.x + s1w, statY);
        dl->AddCircleFilled(ImVec2(s2p.x + 4, s2p.y + 7),
                            3.5f, ToU32(ColAccent()));
        dl->AddText(ImVec2(s2p.x + 14, s2p.y),
                    IM_COL32(184, 188, 192, 255), stat2);

        ImGui::Dummy(ImVec2(w, heroH));
    }

    // ----- Expansion strip ---------------------------------------------------
    //  Clickable tile per expansion — these replace the tab bar. Click to
    //  switch the active expansion (stored in m_activeTab). Label and count
    //  are stacked vertically so the layout works at any window width.
    void DrawExpansionStrip(const std::vector<std::string>& tabs,
                            const std::unordered_map<std::string,
                                std::pair<size_t,size_t>>& tabStats)
    {
        if (tabs.empty()) return;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float tileH = 62.0f;
        float w = ImGui::GetContentRegionAvail().x;
        int n = (int)tabs.size();
        const float gap = 5.0f;
        float tileW = (w - gap * (n - 1)) / (float)n;
        if (tileW < 50.0f) tileW = 50.0f;
        ImVec2 origin = ImGui::GetCursorScreenPos();
        const float padX = 7.0f;

        for (int i = 0; i < n; i++)
        {
            const std::string& tab = tabs[i];
            auto it = tabStats.find(tab);
            size_t d = it != tabStats.end() ? it->second.first  : 0;
            size_t t = it != tabStats.end() ? it->second.second : 0;
            float frac = t > 0 ? (float)d / (float)t : 0.0f;
            bool fullyDone = (t > 0 && d == t);
            bool isActive  = (tab == m_activeTab);

            ImVec2 tl(origin.x + i * (tileW + gap), origin.y);
            ImVec2 tbr(tl.x + tileW, tl.y + tileH);
            ImVec4 accent = ExpansionColor(tab);

            // Click target — full tile area.
            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(tl);
            bool clicked = ImGui::InvisibleButton("##exptile", ImVec2(tileW, tileH));
            bool hovered = ImGui::IsItemHovered();
            if (clicked) m_activeTab = tab;
            if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::PopID();

            // Tile background — brighter when active or hovered.
            ImVec4 bg = isActive ? ImVec4(0.18f, 0.21f, 0.26f, 1.00f)
                       : hovered ? ImVec4(0.14f, 0.16f, 0.20f, 0.98f)
                                  : ImVec4(0.11f, 0.13f, 0.16f, 0.95f);
            dl->AddRectFilled(tl, tbr, ToU32(bg), 4.0f);

            // Top colored stripe (taller when active).
            float stripeH = isActive ? 4.0f : 3.0f;
            dl->AddRectFilled(tl, ImVec2(tbr.x, tl.y + stripeH),
                              ToU32(accent), 4.0f);

            // Border: thick accent when active, thin completion ring otherwise.
            if (isActive)
                dl->AddRect(tl, tbr, ToU32(accent), 4.0f,
                            ImDrawCornerFlags_All, 2.0f);
            else if (fullyDone)
                dl->AddRect(tl, tbr, ToU32A(accent, 0.9f), 4.0f,
                            ImDrawCornerFlags_All, 1.5f);
            else
                dl->AddRect(tl, tbr, ToU32(ColSep()), 4.0f);

            const char* lbl = TabShortLabel(tab);
            char cnt[32];
            snprintf(cnt, sizeof(cnt), "%zu/%zu", d, t);

            const float innerLeft  = tl.x + padX;
            const float innerRight = tbr.x - padX;
            const float innerW     = innerRight - innerLeft;

            ImVec2 ls = ImGui::CalcTextSize(lbl);
            float lx = innerLeft + (innerW - ls.x) * 0.5f;
            if (lx < innerLeft) lx = innerLeft;
            dl->AddText(ImVec2(lx, tl.y + 10),
                        isActive ? IM_COL32(252, 252, 252, 255)
                                 : IM_COL32(228, 228, 228, 255),
                        lbl);

            ImVec2 cts = ImGui::CalcTextSize(cnt);
            float cx = innerLeft + (innerW - cts.x) * 0.5f;
            if (cx < innerLeft) cx = innerLeft;
            dl->AddText(ImVec2(cx, tl.y + 28),
                        fullyDone ? ToU32(ColDone())
                                  : IM_COL32(178, 182, 188, 255),
                        cnt);

            const float mbH = 4.0f;
            float mbY = tbr.y - 9 - mbH;
            ImVec2 mbP (innerLeft,  mbY);
            ImVec2 mbP1(innerRight, mbY + mbH);
            float mbW = mbP1.x - mbP.x;
            dl->AddRectFilled(mbP, mbP1, IM_COL32(28, 32, 38, 255), mbH * 0.5f);
            if (frac > 0.0f)
            {
                float fillW = mbW * (frac > 1.0f ? 1.0f : frac);
                if (fillW < mbH) fillW = mbH;
                dl->AddRectFilled(mbP, ImVec2(mbP.x + fillW, mbP1.y),
                                  ToU32(accent), mbH * 0.5f);
            }
        }

        // The per-tile InvisibleButtons disrupted the cursor; restore it to
        // the row directly below the strip so the next widget lays out cleanly.
        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + tileH));
    }

    // ----- Controls (char/search/filter/sort/copy) ---------------------------
    void DrawControlsRow(const std::string& viewChar,
                         const std::string& activeChar,
                         bool activeIsReal,
                         const std::vector<std::string>& chars,
                         size_t doneAll, size_t totalAll, float progressAll)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ColTextDim());
        ImGui::TextUnformatted("Character");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 10);

        ImGui::SetNextItemWidth(220);
        std::string previewLabel;
        if (viewChar.empty() || viewChar == "__unknown__")
            previewLabel = "Not in game";
        else
            previewLabel = viewChar + (viewChar == activeChar ? "  [current]" : "");

        if (ImGui::BeginCombo("##charselect", previewLabel.c_str()))
        {
            if (activeIsReal)
            {
                std::string label = activeChar + "  [current]";
                bool sel = (viewChar == activeChar);
                if (ImGui::Selectable(label.c_str(), sel))
                {
                    m_viewedCharacter    = activeChar;
                    m_viewedCharacterSet = true;
                }
                if (sel) ImGui::SetItemDefaultFocus();
                if (!chars.empty()) ImGui::Separator();
            }
            for (const auto& c : chars)
            {
                if (c == activeChar) continue;
                bool sel = (viewChar == c);
                if (ImGui::Selectable(c.c_str(), sel))
                {
                    m_viewedCharacter    = c;
                    m_viewedCharacterSet = true;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            if (!activeIsReal && chars.empty())
                ImGui::TextDisabled("Log in to a character to start tracking.");
            ImGui::EndCombo();
        }

        if (activeIsReal && !viewChar.empty() && viewChar != activeChar)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Go to current"))
            {
                m_viewedCharacter    = activeChar;
                m_viewedCharacterSet = true;
            }
        }

        // Right-aligned Copy progress button
        ImGui::SameLine();
        const float copyBtnW = 120.0f;
        float curX  = ImGui::GetCursorPosX();
        float rightX = ImGui::GetWindowContentRegionMax().x - copyBtnW;
        if (rightX > curX) ImGui::SetCursorPosX(rightX);
        if (ImGui::Button("Copy progress", ImVec2(copyBtnW, 0)))
        {
            char buf[256];
            const char* who = (viewChar.empty() || viewChar == "__unknown__")
                              ? "(no character)" : viewChar.c_str();
            snprintf(buf, sizeof(buf),
                     "GW2 Map Completion %s: %zu / %zu (%.1f%%)",
                     who, doneAll, totalAll, progressAll * 100.0f);
            ImGui::SetClipboardText(buf);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Copy progress summary to clipboard");

        // Second row
        ImGui::Spacing();
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##search", "Search maps...",
                                 m_searchBuf, sizeof(m_searchBuf));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        static const char* filterItems[] = { "All", "Completed", "Incomplete" };
        ImGui::Combo("##filter", &m_filterMode, filterItems, 3);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        static const char* sortItems[] = { "Default", "Name", "Map ID", "Status" };
        ImGui::Combo("##sort", &m_sortMode, sortItems, 4);
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) m_searchBuf[0] = '\0';
    }

    // ----- Current map banner ------------------------------------------------
    void DrawCurrentMapBanner(const std::string& viewChar)
    {
        if (!MumbleLinkData || MumbleLinkData->uiTick == 0) return;
        uint32_t mapId = 0;
        if (MumbleLinkData->context_len >= 32)
        {
            mapId = (uint32_t)MumbleLinkData->context[28]
                  | ((uint32_t)MumbleLinkData->context[29] << 8)
                  | ((uint32_t)MumbleLinkData->context[30] << 16)
                  | ((uint32_t)MumbleLinkData->context[31] << 24);
        }
        if (mapId == 0) return;

        const auto& byId = GetMapsById();
        auto it = byId.find(mapId);
        if (it == byId.end()) return;

        bool done = m_tracker->IsCompleteFor(viewChar, it->second->id);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w  = ImGui::GetContentRegionAvail().x;
        const float h = 28.0f;
        ImVec2 br(p.x + w, p.y + h);

        ImVec4 accent = done ? ColDone() : ColAccent();
        dl->AddRectFilled(p, br, ToU32A(accent, 0.12f), 4.0f);
        dl->AddRectFilled(p, ImVec2(p.x + 3, br.y), ToU32(accent), 2.0f);
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "Currently on: %s   %s",
                 it->second->name.c_str(),
                 done ? "[COMPLETE]" : "[incomplete]");
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(p.x + 12, p.y + (h - ts.y) * 0.5f),
                    ToU32(accent), buf);
        ImGui::Dummy(ImVec2(w, h));
    }

    // ----- Per-tab segmented progress ---------------------------------------
    void DrawSegmentedProgress(float fraction, float h, ImVec4 fill)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w  = ImGui::GetContentRegionAvail().x;
        ImVec2 br(p.x + w, p.y + h);
        float r = h * 0.5f;
        dl->AddRectFilled(p, br, IM_COL32(24, 28, 34, 255), r);
        if (fraction > 0.0f)
        {
            float fw = w * (fraction > 1.0f ? 1.0f : fraction);
            if (fw < r * 2.0f) fw = r * 2.0f;
            ImVec4 dim(fill.x * 0.65f, fill.y * 0.65f, fill.z * 0.65f, 1);
            dl->AddRectFilledMultiColor(
                p, ImVec2(p.x + fw, br.y),
                ToU32(dim), ToU32(fill), ToU32(fill), ToU32(dim));
        }
        ImGui::Dummy(ImVec2(w, h));
    }

    // ----- Sorting -----------------------------------------------------------
    void SortMapList(const std::string& viewChar,
                     std::vector<const MapInfo*>& rowMaps)
    {
        if (m_sortMode == 0) return;
        if (m_sortMode == 1)
        {
            std::sort(rowMaps.begin(), rowMaps.end(),
                [](const MapInfo* a, const MapInfo* b){ return a->name < b->name; });
        }
        else if (m_sortMode == 2)
        {
            std::sort(rowMaps.begin(), rowMaps.end(),
                [](const MapInfo* a, const MapInfo* b){ return a->id < b->id; });
        }
        else if (m_sortMode == 3)
        {
            std::sort(rowMaps.begin(), rowMaps.end(),
                [this, &viewChar](const MapInfo* a, const MapInfo* b){
                    bool da = m_tracker->IsCompleteFor(viewChar, a->id);
                    bool db = m_tracker->IsCompleteFor(viewChar, b->id);
                    if (da != db) return da && !db;  // completed first
                    return a->name < b->name;
                });
        }
    }

    // ----- Region card -------------------------------------------------------
    void DrawRegionCard(const std::string& region, size_t done, size_t total,
                        const std::string& viewChar, bool canEdit,
                        const std::vector<const MapInfo*>& rowMaps)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w  = ImGui::GetContentRegionAvail().x;
        const float headerH = 26.0f;
        ImVec2 hbr(p.x + w, p.y + headerH);

        dl->AddRectFilled(p, hbr, ToU32(ColCardHead()), 4.0f);
        dl->AddLine(ImVec2(p.x + 6, hbr.y - 1),
                    ImVec2(hbr.x - 6, hbr.y - 1),
                    ToU32(ColSep()), 1.0f);

        ImVec2 ns = ImGui::CalcTextSize(region.c_str());
        dl->AddText(ImVec2(p.x + 10, p.y + (headerH - ns.y) * 0.5f),
                    ToU32(ColAccent()), region.c_str());

        char rcnt[48];
        snprintf(rcnt, sizeof(rcnt), "%zu / %zu", done, total);
        ImVec2 rs = ImGui::CalcTextSize(rcnt);
        bool regionDone = (total > 0 && done == total);
        dl->AddText(ImVec2(hbr.x - 10 - rs.x,
                           p.y + (headerH - rs.y) * 0.5f),
                    regionDone ? ToU32(ColDone()) : IM_COL32(178, 182, 188, 255),
                    rcnt);

        ImGui::Dummy(ImVec2(w, headerH));

        for (const auto* m : rowMaps)
        {
            bool d = m_tracker->IsCompleteFor(viewChar, m->id);
            DrawMapRow(viewChar, *m, d, canEdit);
        }
        ImGui::Spacing();
    }

    // ----- Single map row ----------------------------------------------------
    void DrawMapRow(const std::string& viewChar, const MapInfo& m,
                    bool done, bool canEdit)
    {
        ImGui::PushID((int)m.id);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w  = ImGui::GetContentRegionAvail().x;
        const float rowH = 26.0f;

        bool clicked = ImGui::InvisibleButton("##row", ImVec2(w, rowH));
        bool hovered = ImGui::IsItemHovered();
        if (clicked && canEdit)
        {
            if (done) m_tracker->MarkIncompleteFor(viewChar, m.id);
            else      m_tracker->MarkCompleteFor  (viewChar, m.id);
        }

        ImVec2 br(p.x + w, p.y + rowH);

        if (hovered && canEdit)
            dl->AddRectFilled(p, br, IM_COL32(255, 255, 255, 14), 3.0f);
        if (done)
            dl->AddRectFilled(p, ImVec2(p.x + 3, br.y), ToU32(ColDone()), 2.0f);

        // Status dot
        float cy = p.y + rowH * 0.5f;
        float cx = p.x + 18.0f;
        if (done)
        {
            dl->AddCircleFilled(ImVec2(cx, cy), 5.5f, ToU32(ColDone()));
            dl->AddCircle(ImVec2(cx, cy), 7.5f,
                          ToU32A(ColDone(), 0.45f), 12, 1.0f);
            ImVec2 c1(cx - 2.4f, cy + 0.2f);
            ImVec2 c2(cx - 0.6f, cy + 2.0f);
            ImVec2 c3(cx + 2.6f, cy - 1.8f);
            dl->AddLine(c1, c2, IM_COL32(8, 28, 8, 255), 1.6f);
            dl->AddLine(c2, c3, IM_COL32(8, 28, 8, 255), 1.6f);
        }
        else
        {
            dl->AddCircle(ImVec2(cx, cy), 6.0f,
                          ToU32(ColTextMute()), 14, 1.5f);
        }

        // Name
        ImU32 nameCol = done
            ? IM_COL32(168, 226, 168, 255)
            : (canEdit ? IM_COL32(232, 232, 232, 255)
                       : IM_COL32(160, 162, 165, 255));
        ImVec2 ns = ImGui::CalcTextSize(m.name.c_str());
        dl->AddText(ImVec2(p.x + 32, cy - ns.y * 0.5f),
                    nameCol, m.name.c_str());

        // ID pill (right)
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%u", m.id);
        ImVec2 ids = ImGui::CalcTextSize(idBuf);
        const float pillPadX = 6.0f, pillPadY = 2.0f;
        ImVec2 pillBr(br.x - 8.0f,
                      cy + ids.y * 0.5f + pillPadY);
        ImVec2 pillTl(pillBr.x - ids.x - pillPadX * 2.0f,
                      cy - ids.y * 0.5f - pillPadY);
        dl->AddRectFilled(pillTl, pillBr, IM_COL32(28, 32, 38, 220), 3.0f);
        dl->AddText(ImVec2(pillTl.x + pillPadX, pillTl.y + pillPadY),
                    IM_COL32(150, 152, 156, 255), idBuf);

        ImGui::PopID();
    }

    // ----- Filtering ---------------------------------------------------------
    bool PassesFilter(const std::string& viewChar, const MapInfo& m,
                      const std::string& searchLower) const
    {
        bool done = m_tracker->IsCompleteFor(viewChar, m.id);
        if (m_filterMode == 1 && !done) return false;
        if (m_filterMode == 2 &&  done) return false;
        if (!searchLower.empty())
        {
            std::string nameLower = m.name;
            for (auto& c : nameLower) c = (char)tolower((unsigned char)c);
            if (nameLower.find(searchLower) == std::string::npos) return false;
        }
        return true;
    }

    // ----- State -------------------------------------------------------------
    CompletionTracker* m_tracker            = nullptr;
    bool               m_visible            = false;
    char               m_searchBuf[128]     = {};
    int                m_filterMode         = 0;
    int                m_sortMode           = 0;
    std::string        m_viewedCharacter;
    std::string        m_savePath;
    std::string        m_activeTab;
    bool               m_viewedCharacterSet = false;
};
