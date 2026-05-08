// ============================================================================
// EventDebug.cpp — Full Map Completion Debug Module
// Uses Nexus/Mumble for character name + map ID
// Reads API key from completion.json
// Fetches map completion + map metadata via WinHTTP
// Logs everything to debug_mapinfo.txt
// Runs once per map load
// ============================================================================

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp>

#include "Nexus.h"

// Provided by AddonMain.cpp
extern AddonAPI_t* g_api;
extern std::string GetAddonDirectory();

// Forward declaration so AddonMain.cpp can call us
void EventDebug_OnMumbleIdentityUpdated(int mapId);

// JSON alias
using json = nlohmann::json;

// ============================================================================
// Internal state
// ============================================================================
static int g_lastMapId = -1;
static std::mutex g_logMutex;

static std::wstring Utf8ToWstring(const std::string& s)
{
    if (s.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], size_needed);
    return w;
}

// ============================================================================
// Helper: Write a line to file
// ============================================================================
static void WriteLine(std::ofstream& out, const std::string& s)
{
    out << s << "\n";
}

// ============================================================================
// Helper: URL encode (minimal, enough for character names)
// ============================================================================
static std::string UrlEncode(const std::string& s)
{
    std::string out;
    char buf[4];

    for (unsigned char c : s)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out.push_back(c);
        else
        {
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out.append(buf);
        }
    }
    return out;
}

// ============================================================================
// Helper: WinHTTP GET (synchronous)
// ============================================================================
static std::string HttpGet(const std::wstring& host, const std::wstring& path)
{
    std::string result;

    HINTERNET hSession = WinHttpOpen(
        L"MapCompletionTracker/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(
        hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0
    );
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    BOOL sent = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0,
        0, 0
    );

    if (!sent || !WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
    {
        std::string chunk(bytesAvail, '\0');
        DWORD bytesRead = 0;
        WinHttpReadData(hRequest, &chunk[0], bytesAvail, &bytesRead);
        result.append(chunk, 0, bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

// ============================================================================
// Helper: Load API key from completion.json
// ============================================================================
static std::string LoadApiKey()
{
    std::string dir = GetAddonDirectory();
    std::string path = dir + "completion.json";

    std::ifstream f(path);
    if (!f.is_open())
        return "";

    try
    {
        json j;
        f >> j;
        if (j.contains("api_key") && j["api_key"].is_string())
            return j["api_key"].get<std::string>();
    }
    catch (...) {}

    return "";
}

// ============================================================================
// Helper: Log header with timestamp
// ============================================================================
static void WriteHeader(std::ofstream& out, const std::string& title)
{
    std::time_t now = std::time(nullptr);
    char buf[64] = {};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    WriteLine(out, "============================================================");
    WriteLine(out, title);
    WriteLine(out, std::string("Timestamp: ") + buf);
    WriteLine(out, "");
}

// ============================================================================
// Main debug function — called from AddonMain.cpp
// ============================================================================
void EventDebug_OnMumbleIdentityUpdated(int mapId)
{
    if (!g_api)
        return;

    // Avoid spamming — only run when map ID changes
    if (mapId == g_lastMapId)
        return;

    g_lastMapId = mapId;

    // Get Mumble identity pointer
    void* raw = g_api->DataLink_Get(DL_MUMBLE_LINK_IDENTITY);
    if (!raw)
        return;

    MumbleIdentity* id = reinterpret_cast<MumbleIdentity*>(raw);
    if (!id)
        return;

    std::string character = id->Name;
    if (character.empty())
        return;

    // Load API key
    std::string apiKey = LoadApiKey();
    if (apiKey.empty())
        return;

    // Prepare log file
    std::string dir = GetAddonDirectory();
    std::string logPath = dir + "debug_mapinfo.txt";

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::ofstream out(logPath, std::ios::app);
    if (!out.is_open())
        return;

    WriteHeader(out, "Map Completion Debug");

    WriteLine(out, "Character:");
    WriteLine(out, character);
    WriteLine(out, "");

    WriteLine(out, "Map ID:");
    WriteLine(out, std::to_string(mapId));
    WriteLine(out, "");

    // URL encode character name
    std::string encodedName = UrlEncode(character);

    // Build API paths
    std::wstring host = L"api.guildwars2.com";

    std::wstring wEncodedName = Utf8ToWstring(encodedName);
    std::wstring wApiKey      = Utf8ToWstring(apiKey);

    std::wstring pathCompletion =
        L"/v2/characters/" + wEncodedName +
        L"/maps?access_token=" + wApiKey;


    std::wstring pathMap = L"/v2/maps/" + std::to_wstring(mapId);


    // Fetch map completion
    std::string completionJson = HttpGet(host, pathCompletion);
    if (completionJson.empty())
    {
        WriteLine(out, "ERROR: Failed to fetch map completion.");
        WriteLine(out, "");
        return;
    }

    // Fetch map metadata
    std::string mapJson = HttpGet(host, pathMap);
    if (mapJson.empty())
    {
        WriteLine(out, "ERROR: Failed to fetch map metadata.");
        WriteLine(out, "");
        return;
    }

    // Parse and log
    try
    {
        json jc = json::parse(completionJson);
        json jm = json::parse(mapJson);

        WriteLine(out, "Map Name:");
        if (jm.contains("name"))
            WriteLine(out, jm["name"].get<std::string>());
        else
            WriteLine(out, "(unknown)");

        WriteLine(out, "");

        WriteLine(out, "Completion Data:");
        WriteLine(out, jc.dump(4));
        WriteLine(out, "");

    }
    catch (...)
    {
        WriteLine(out, "ERROR: Failed to parse JSON.");
        WriteLine(out, "");
    }
}

// ============================================================================
// Register / Unregister (called from AddonMain.cpp)
// ============================================================================
extern "C" void RegisterEventDebug()
{
    // Nothing needed here — AddonMain.cpp calls us directly
}

extern "C" void UnregisterEventDebug()
{
    // Nothing needed
}
