// EventDebug.cpp
// Logs all events once, except Rata Sum map completion loot (Inquest Field Kit)

#include "AddonAPI.h"
#include <unordered_set>
#include <string>
#include <format>

static std::unordered_set<std::string> g_SeenEvents;

// Rata Sum map completion reward item ID
static constexpr uint32_t RATA_SUM_ITEM_ID = 70358;

// Helper: print event if unique OR if it's the Rata Sum reward
static void LogEvent(const std::string& name, bool force = false)
{
    if (!force)
    {
        if (g_SeenEvents.contains(name))
            return;
        g_SeenEvents.insert(name);
    }

    api->LogText(std::format("[EventDebug] {}", name).c_str());
}

// -------------------------
// Nexus Core Events
// -------------------------

void OnRender()
{
    LogEvent("Nexus: OnRender");
}

void OnRenderUI()
{
    LogEvent("Nexus: OnRenderUI");
}

void OnRenderWindows()
{
    LogEvent("Nexus: OnRenderWindows");
}

void OnKeyDown(uint32_t key)
{
    LogEvent(std::format("Nexus: OnKeyDown {}", key));
}

void OnKeyUp(uint32_t key)
{
    LogEvent(std::format("Nexus: OnKeyUp {}", key));
}

void OnMouseDown(uint32_t button)
{
    LogEvent(std::format("Nexus: OnMouseDown {}", button));
}

void OnMouseUp(uint32_t button)
{
    LogEvent(std::format("Nexus: OnMouseUp {}", button));
}

void OnMouseMove(int x, int y)
{
    LogEvent("Nexus: OnMouseMove");
}

void OnScroll(int delta)
{
    LogEvent(std::format("Nexus: OnScroll {}", delta));
}

// -------------------------
// Events:Chat
// -------------------------

// Message type enum from GW2-Chat
enum class MessageType : uint32_t {
    Error = 0,
    Guild,
    GuildMotD,
    Local,
    Map,
    Party,
    Squad,
    SquadMessage,
    SquadBroadcast,
    TeamPvP,
    TeamWvW,
    Whisper,
    Emote,
    EmoteCustom
};

const char* GetMessageTypeString(uint32_t type)
{
    switch (type) {
        case 0: return "Error";
        case 1: return "Guild";
        case 2: return "GuildMotD";
        case 3: return "Local";
        case 4: return "Map";
        case 5: return "Party";
        case 6: return "Squad";
        case 7: return "SquadMessage";
        case 8: return "SquadBroadcast";
        case 9: return "TeamPvP";
        case 10: return "TeamWvW";
        case 11: return "Whisper";
        case 12: return "Emote";
        case 13: return "EmoteCustom";
        default: return "Unknown";
    }
}

// GW2-Chat Message struct layout (from Chat.h)
// We need to parse it manually since we don't have the addon's headers
struct GW2ChatMessage
{
    uint64_t DateTime_Low;
    uint64_t DateTime_High;
    uint32_t Type;
    uint32_t Flags;
    // Followed by union data - we'll access via offsets
};

void OnGW2ChatMessage(void* aEventArgs)
{
    if (!aEventArgs)
        return;

    GW2ChatMessage* msg = static_cast<GW2ChatMessage*>(aEventArgs);
    
    // Parse the message - the struct layout is:
    // DateTime (FILETIME): 8 bytes
    // Type: 4 bytes
    // Flags: 4 bytes
    // Union data follows (varies by type)
    
    // For "You receive" messages, they typically come through as system/whisper messages
    // The Content field is at offset 16 + union offset
    // For GenericMessage-based types: Account(16) + CharacterName(ptr) + AccountName(ptr) + Content(ptr)
    
    // Let's log the raw data for debugging
    char logBuf[512];
    snprintf(logBuf, sizeof(logBuf), 
             "[GW2-Chat] Type=%u Flags=%u DateTime=%llu",
             msg->Type, msg->Flags, msg->DateTime_Low);
    api->LogText(logBuf);
    
    // Store in our list
    ChatMessageEvent evt;
    evt.timestamp = msg->DateTime_Low;
    evt.type = GetMessageTypeString(msg->Type);
    evt.flags = msg->Flags;
    
    // Try to extract character name and content
    // This is tricky without the exact struct layout - we'll log what we can
    g_chat_messages.push_back(evt);
    
    // Keep only last 100 events
    if (g_chat_messages.size() > 100)
        g_chat_messages.erase(g_chat_messages.begin());
}

// -------------------------
// Events:Chat
// -------------------------

void OnChatMessage(const ChatMessageInfo* msg)
{
    std::string text = msg->text ? msg->text : "";

    // Detect Rata Sum map completion loot
    if (text.find("Inquest Field Kit") != std::string::npos)
    {
        LogEvent("CHAT: Rata Sum Map Completion Loot (Inquest Field Kit)", true);
        return;
    }

    LogEvent(std::format("CHAT: {}", text));
}

// -------------------------
// Registration API
// -------------------------

void RegisterEventDebug()
{
    api->RegisterRender(OnRender);
    api->RegisterRenderUI(OnRenderUI);
    api->RegisterRenderWindows(OnRenderWindows);

    api->RegisterKeyDown(OnKeyDown);
    api->RegisterKeyUp(OnKeyUp);
    api->RegisterMouseDown(OnMouseDown);
    api->RegisterMouseUp(OnMouseUp);
    api->RegisterMouseMove(OnMouseMove);
    api->RegisterScroll(OnScroll);

    if (api->eventsChat)
        api->eventsChat->Subscribe(OnChatMessage);

    LogEvent("EventDebug Registered");
}

void UnregisterEventDebug()
{
    LogEvent("EventDebug Unregistered");
}

// -------------------------
// Registration API
// -------------------------

void RegisterEventDebug()
{
    api->RegisterRender(OnRender);
    api->RegisterRenderUI(OnRenderUI);
    api->RegisterRenderWindows(OnRenderWindows);

    api->RegisterKeyDown(OnKeyDown);
    api->RegisterKeyUp(OnKeyUp);
    api->RegisterMouseDown(OnMouseDown);
    api->RegisterMouseUp(OnMouseUp);
    api->RegisterMouseMove(OnMouseMove);
    api->RegisterScroll(OnScroll);

    if (api->eventsChat)
        api->eventsChat->Subscribe(OnChatMessage);

    // Subscribe to GW2-Chat addon events
    api->Events_Subscribe("EV_CHAT:Message", OnGW2ChatMessage);
    api->LogText("[EventDebug] Subscribed to EV_CHAT:Message");

    LogEvent("EventDebug Registered");
}

void UnregisterEventDebug()
{
    LogEvent("EventDebug Unregistered");
}

// -------------------------
// ArcDPS Exports
// -------------------------
// ArcDPS loads this directly - separate from Nexus API
static ArcDPS::PluginInfo s_arcdps_exports{};

extern "C" __declspec(dllexport) ArcDPS::PluginInfo* arcdps_exports()
{
    s_arcdps_exports.Size = sizeof(ArcDPS::PluginInfo);
    s_arcdps_exports.Signature = 19823; // Unique signature for this module
    s_arcdps_exports.ImGuiVersion = 18000; // IMGUI_VERSION_NUM
    s_arcdps_exports.Name = "EventDebug";
    s_arcdps_exports.Build = "1.0.0";
    s_arcdps_exports.CombatSquadCallback = (void*)OnCombatEvent;
    // Don't set CombatLocalCallback to avoid double-counting
    return &s_arcdps_exports;
}
    else
    {
        if (ImGui::BeginTable("RewardEvents", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Time");
            ImGui::TableSetupColumn("Reward ID");
            ImGui::TableSetupColumn("Reward Type");
            ImGui::TableSetupColumn("Details");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < g_reward_events.size(); i++)
            {
                const auto& evt = g_reward_events[i];
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", evt.time);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%llu", evt.reward_id);
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", evt.reward_type);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Check arcdps.log for details");
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();
    
    if (ImGui::Button("Clear"))
        g_reward_events.clear();

    ImGui::SameLine();
    ImGui::Text("Events: %zu", g_reward_events.size());

    ImGui::End();
}

void RenderChatDebugWindow()
{
    ImGui::Begin("GW2-Chat Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Subscribed to: EV_CHAT:Message");
    ImGui::Separator();

    if (g_chat_messages.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No chat messages received yet");
    }
    else
    {
        if (ImGui::BeginTable("ChatMessages", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
        {
            ImGui::TableSetupColumn("Time");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Flags");
            ImGui::TableSetupColumn("Character");
            ImGui::TableSetupColumn("Content");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < g_chat_messages.size(); i++)
            {
                const auto& evt = g_chat_messages[i];
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%llu", evt.timestamp);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", evt.type.c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0x%X", evt.flags);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", evt.character.empty() ? "N/A" : evt.character.c_str());
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", evt.content.empty() ? "N/A" : evt.content.c_str());
                
                // Highlight "You receive" messages
                if (evt.content.find("You receive") != std::string::npos)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[REWARD]");
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();
    
    if (ImGui::Button("Clear"))
        g_chat_messages.clear();

    ImGui::SameLine();
    ImGui::Text("Messages: %zu", g_chat_messages.size());

    ImGui::End();
}

// -------------------------
// Registration API
// -------------------------

void RegisterEventDebug()
{
    api->RegisterRender(OnRender);
    api->RegisterRenderUI(OnRenderUI);
    api->RegisterRenderWindows(OnRenderWindows);

    api->RegisterKeyDown(OnKeyDown);
    api->RegisterKeyUp(OnKeyUp);
    api->RegisterMouseDown(OnMouseDown);
    api->RegisterMouseUp(OnMouseUp);
    api->RegisterMouseMove(OnMouseMove);
    api->RegisterScroll(OnScroll);

    if (api->eventsChat)
        api->eventsChat->Subscribe(OnChatMessage);

    // Subscribe to GW2-Chat addon events
    api->Events_Subscribe("EV_CHAT:Message", OnGW2ChatMessage);
    api->LogText("[EventDebug] Subscribed to EV_CHAT:Message");

    LogEvent("EventDebug Registered");
}

void UnregisterEventDebug()
{
    LogEvent("EventDebug Unregistered");
}

// -------------------------
// ArcDPS Exports
// -------------------------
// ArcDPS loads this directly - separate from Nexus API
static ArcDPS::PluginInfo s_arcdps_exports{};

extern "C" __declspec(dllexport) ArcDPS::PluginInfo* arcdps_exports()
{
    s_arcdps_exports.Size = sizeof(ArcDPS::PluginInfo);
    s_arcdps_exports.Signature = 19823; // Unique signature for this module
    s_arcdps_exports.ImGuiVersion = 18000; // IMGUI_VERSION_NUM
    s_arcdps_exports.Name = "EventDebug";
    s_arcdps_exports.Build = "1.0.0";
    s_arcdps_exports.CombatSquadCallback = (void*)OnCombatEvent;
    // Don't set CombatLocalCallback to avoid double-counting
    return &s_arcdps_exports;
}
