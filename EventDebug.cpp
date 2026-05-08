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
