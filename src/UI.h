#pragma once

#include "CompletionTracker.h"
#include "MapData.h"
#include "Shared.h"
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>

// ImGui 1.80 doesn't have BeginDisabled/EndDisabled (added in 1.87).
static void ImGui_BeginDisabled()
{
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.4f);
}
static void ImGui_EndDisabled()
{
    ImGui::PopStyleVar();
}

class UI
{
public:
    void Init(CompletionTracker* tracker)
    {
        m_tracker = tracker;
    }

    void ToggleVisible() { m_visible = !m_visible; }
    bool IsVisible()     { return m_visible; }

    // -----------------------------------------------------------------
    //  Main window
    // -----------------------------------------------------------------
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

        // Compute overall progress — count only maps actually in the tracked list
        size_t totalAll = allMaps.size();
        size_t doneAll  = 0;
        for (const auto& m : allMaps)
            if (m_tracker->IsCompleteFor(viewChar, m.id)) doneAll++;
        float  progressAll = totalAll > 0 ? (float)doneAll / (float)totalAll : 0.0f;

        ImGui::SetNextWindowSize(ImVec2(560, 720), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(360, 380), ImVec2(960, 1400));

        if (!ImGui::Begin("Map Completion Tracker", &m_visible, ImGuiWindowFlags_None))
        {
            ImGui::End();
            return;
        }

        // ---- Character selector ----
        ImGui::TextUnformatted("Character:");
        ImGui::SameLine();
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
                ImGui::TextDisabled("Log in or add an API key in Options.");
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

        ImGui::Spacing();

        // ---- Overall progress bar ----
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.22f, 0.72f, 0.22f, 1.0f));
        char overallLabel[64];
        snprintf(overallLabel, sizeof(overallLabel),
                 "Overall: %zu / %zu  (%.1f%%)", doneAll, totalAll, progressAll * 100.0f);
        ImGui::ProgressBar(progressAll, ImVec2(-1, 18), overallLabel);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // ---- Current map info ----
        if (MumbleLinkData && MumbleLinkData->uiTick > 0)
        {
            uint32_t mapId = 0;
            if (MumbleLinkData->context_len >= 12)
                mapId = (uint32_t)MumbleLinkData->context[8]
                      | ((uint32_t)MumbleLinkData->context[9]  << 8)
                      | ((uint32_t)MumbleLinkData->context[10] << 16)
                      | ((uint32_t)MumbleLinkData->context[11] << 24);
            if (mapId != 0)
            {
                const auto& byId = GetMapsById();
                auto it = byId.find(mapId);
                if (it != byId.end())
                {
                    bool curDone = m_tracker->IsCompleteFor(viewChar, it->second->id);
                    ImGui::TextColored(
                        curDone ? ImVec4(0.3f,0.9f,0.3f,1) : ImVec4(1,0.85f,0.2f,1),
                        "Current map: %s [%s]",
                        it->second->name.c_str(),
                        curDone ? "COMPLETE" : "incomplete");
                }
                else
                    ImGui::TextDisabled("Current map ID %u (not tracked)", mapId);
            }
            else
                ImGui::TextDisabled("Not in a tracked map");
        }
        else
            ImGui::TextDisabled("Not in game");

        ImGui::Separator();

        // ---- Search + filter ----
        ImGui::SetNextItemWidth(180);
        ImGui::InputText("##search", m_searchBuf, sizeof(m_searchBuf));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        static const char* filterItems[] = { "All", "Completed", "Incomplete" };
        ImGui::Combo("##filter", &m_filterMode, filterItems, 3);
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear"))
            m_searchBuf[0] = '\0';

        ImGui::Spacing();

        bool canEdit = !viewChar.empty() && viewChar != "__unknown__";
        if (!canEdit)
        {
            ImGui::TextColored(ImVec4(1,0.6f,0.2f,1),
                "Log in to a character or add an API key in Options to track progress.");
            ImGui::Spacing();
        }

        std::string searchLower = m_searchBuf;
        for (auto& c : searchLower) c = (char)tolower((unsigned char)c);

        // ---- Tab bar ----
        if (ImGui::BeginTabBar("##expansiontabs"))
        {
            for (const auto& tabName : tabs)
            {
                // Collect maps for this tab that pass the filter
                std::vector<const MapInfo*> tabMaps;
                for (const auto& m : allMaps)
                    if (m.tab == tabName && PassesFilter(viewChar, m, searchLower))
                        tabMaps.push_back(&m);

                // Compute tab progress
                size_t tabTotal = 0, tabDone = 0;
                for (const auto& m : allMaps)
                    if (m.tab == tabName)
                    {
                        tabTotal++;
                        if (m_tracker->IsCompleteFor(viewChar, m.id)) tabDone++;
                    }

                // Build tab label with compact progress
                char tabLabel[64];
                snprintf(tabLabel, sizeof(tabLabel), "%s##tab", tabName.c_str());

                if (ImGui::BeginTabItem(tabLabel))
                {
                    // Per-tab progress bar
                    float tabProgress = tabTotal > 0 ? (float)tabDone / (float)tabTotal : 0.0f;
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.22f, 0.72f, 0.22f, 1.0f));
                    char tabProgressLabel[64];
                    snprintf(tabProgressLabel, sizeof(tabProgressLabel),
                             "%zu / %zu  (%.1f%%)", tabDone, tabTotal, tabProgress * 100.0f);
                    ImGui::ProgressBar(tabProgress, ImVec2(-1, 14), tabProgressLabel);
                    ImGui::PopStyleColor();
                    ImGui::Spacing();

                    ImGui::BeginChild("##mapslist", ImVec2(0, 0), false, ImGuiWindowFlags_None);

                    // Group by region within the tab
                    std::vector<std::string> regions;
                    {
                        std::unordered_set<std::string> seen;
                        for (const auto& m : allMaps)
                            if (m.tab == tabName && seen.insert(m.region).second)
                                regions.push_back(m.region);
                    }

                    for (const auto& region : regions)
                    {
                        // Check if any maps in this region pass the filter
                        bool anyVisible = false;
                        for (const auto& m : allMaps)
                            if (m.tab == tabName && m.region == region
                                && PassesFilter(viewChar, m, searchLower))
                            { anyVisible = true; break; }
                        if (!anyVisible) continue;

                        // Only show region header if there's more than one region in this tab
                        bool multiRegion = regions.size() > 1;
                        bool expanded = true;
                        if (multiRegion)
                            expanded = ImGui::CollapsingHeader(region.c_str(),
                                           ImGuiTreeNodeFlags_DefaultOpen);

                        if (expanded)
                        {
                            if (multiRegion) ImGui::Indent(8.0f);
                            for (const auto& m : allMaps)
                            {
                                if (m.tab != tabName || m.region != region) continue;
                                if (!PassesFilter(viewChar, m, searchLower)) continue;
                                bool done = m_tracker->IsCompleteFor(viewChar, m.id);
                                DrawMapRow(viewChar, m, done, canEdit);
                            }
                            if (multiRegion) ImGui::Unindent(8.0f);
                        }
                    }

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // -----------------------------------------------------------------
    //  Options panel
    // -----------------------------------------------------------------
    void RenderOptions()
    {
        ImGui::TextUnformatted("Map Completion Tracker Options");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Characters are tracked automatically via MumbleLink.");
        ImGui::TextDisabled("Log in to each character at least once to add it to the list.");
        ImGui::Spacing();
        ImGui::TextDisabled("Keybind: set via Nexus keybind settings (KB_MAPCOMPLETION_TOGGLE)");
    }

    bool ShouldShowAlert()   const { return false; }
    bool ShouldShowChatLog() const { return false; }

private:
    void DrawMapRow(const std::string& viewChar, const MapInfo& m, bool done, bool canEdit)
    {
        ImGui::PushID(m.id);
        if (!canEdit) ImGui_BeginDisabled();
        bool check = done;
        if (ImGui::Checkbox("##chk", &check))
        {
            if (check) m_tracker->MarkCompleteFor(viewChar, m.id);
            else       m_tracker->MarkIncompleteFor(viewChar, m.id);
        }
        if (!canEdit) ImGui_EndDisabled();
        ImGui::SameLine();
        if (done)
            ImGui::TextColored(ImVec4(0.3f,0.9f,0.3f,1), "%s", m.name.c_str());
        else
            ImGui::TextUnformatted(m.name.c_str());
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "(%u)", m.id);
        float textW = ImGui::CalcTextSize(idBuf).x;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - textW + ImGui::GetCursorPosX() - 6.0f);
        ImGui::TextDisabled("%s", idBuf);
        ImGui::PopID();
    }

    bool PassesFilter(const std::string& viewChar, const MapInfo& m, const std::string& searchLower) const
    {
        bool done = m_tracker->IsCompleteFor(viewChar, m.id);
        if (m_filterMode == 1 && !done) return false;
        if (m_filterMode == 2 && done)  return false;
        if (!searchLower.empty())
        {
            std::string nameLower = m.name;
            for (auto& c : nameLower) c = (char)tolower((unsigned char)c);
            if (nameLower.find(searchLower) == std::string::npos) return false;
        }
        return true;
    }

    CompletionTracker* m_tracker            = nullptr;
    bool               m_visible            = false;
    char               m_searchBuf[128]     = {};
    int                m_filterMode         = 0;
    std::string        m_viewedCharacter;
    bool               m_viewedCharacterSet = false;
};
