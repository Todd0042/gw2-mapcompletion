#pragma once

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Async fetcher for per-character map completion from the GW2 API.
//
// Tries /v2/characters/{name}/maps first (may return an array of map objects
// or an object with a "maps" array, each with "done" / sub-count fields).
// Falls back to /v2/characters/{name} (full character data) and looks for
// a top-level "maps" array if the primary endpoint returns nothing useful.
//
// The callback is invoked from the background thread with the character name
// and the set of map IDs the API considers complete for that character.
// CompletionTracker is mutex-protected so calling it from the callback is safe.
class MapCompletionAPI
{
public:
    enum class State { Idle, Fetching, Done, Error };

    using Callback = std::function<void(const std::string& charName,
                                        std::unordered_set<uint32_t> completedIds)>;

    void Fetch(const std::string& charName, const std::string& apiKey, Callback cb)
    {
        if (m_state.load() == State::Fetching) return;
        m_state    = State::Fetching;
        m_charName = charName;
        m_apiKey   = apiKey;
        m_callback = std::move(cb);
        std::thread([this]() { DoFetch(); }).detach();
    }

    State GetState() const { return m_state.load(); }
    void  Reset()          { m_state = State::Idle; }

private:
    // -------------------------------------------------------------------------
    //  Helpers
    // -------------------------------------------------------------------------
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

    static std::wstring ToWide(const std::string& s)
    {
        if (s.empty()) return L"";
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
        return w;
    }

    static std::string HttpGet(const std::wstring& host,
                               const std::wstring& path,
                               const std::wstring& headers)
    {
        std::string result;

        HINTERNET hSession = WinHttpOpen(
            L"GW2MapCompletionTracker/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return result;

        HINTERNET hConnect = WinHttpConnect(
            hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"GET", path.c_str(),
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        BOOL sent = WinHttpSendRequest(
            hRequest, headers.c_str(), (DWORD)-1L,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!sent || !WinHttpReceiveResponse(hRequest, nullptr))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0)
        {
            std::string chunk(avail, '\0');
            DWORD read = 0;
            WinHttpReadData(hRequest, &chunk[0], avail, &read);
            result.append(chunk, 0, read);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Returns true if a map JSON object should be counted as complete.
    // Handles three shapes:
    //   1. { "done": true }
    //   2. { "hearts": { "done": N, "total": N }, "waypoints": ..., ... }
    //   3. Plain integer in an array (caller handles that case separately)
    static bool IsMapDone(const json& m)
    {
        if (m.contains("done") && m["done"].is_boolean())
            return m["done"].get<bool>();

        // All present sub-components must be fully completed
        static const char* kFields[] = {
            "hearts", "waypoints", "poi", "tasks", "skill_challenges", "sectors"
        };
        bool hasAny = false;
        for (const char* f : kFields)
        {
            if (!m.contains(f)) continue;
            const auto& c = m[f];
            if (!c.contains("done") || !c.contains("total")) continue;
            hasAny = true;
            if (c["done"].get<int>() < c["total"].get<int>()) return false;
        }
        return hasAny;
    }

    // Parse whatever the /map_completion (or similar) endpoint returned.
    // Handles:
    //   [15, 17, 18]                          — array of completed map IDs
    //   [{"id":15,"done":true}, ...]           — array of map objects
    //   {"maps":[{"id":15,"done":true}, ...]}  — object wrapping maps array
    static bool ParseCompletionBody(const std::string& body,
                                    std::unordered_set<uint32_t>& out)
    {
        try
        {
            json j = json::parse(body);
            // Both "text" and "error" keys indicate an API error response
            if (j.contains("text") || j.contains("error")) return false;

            if (j.is_array())
            {
                bool found = false;
                for (const auto& item : j)
                {
                    if (item.is_number_integer())
                    {
                        out.insert(item.get<uint32_t>());
                        found = true;
                    }
                    else if (item.is_object() && item.contains("id"))
                    {
                        if (IsMapDone(item))
                            out.insert(item["id"].get<uint32_t>());
                        found = true;
                    }
                }
                return found;
            }

            if (j.is_object() && j.contains("maps") && j["maps"].is_array())
            {
                for (const auto& m : j["maps"])
                    if (m.contains("id") && IsMapDone(m))
                        out.insert(m["id"].get<uint32_t>());
                return true;
            }
        }
        catch (...) {}
        return false;
    }

    // -------------------------------------------------------------------------
    //  Fetch thread
    // -------------------------------------------------------------------------
    void DoFetch()
    {
        std::wstring wEnc  = ToWide(UrlEncode(m_charName));
        std::wstring wKey  = ToWide(m_apiKey);
        std::wstring auth  = L"Authorization: Bearer " + wKey + L"\r\n";
        const std::wstring kHost = L"api.guildwars2.com";

        std::unordered_set<uint32_t> completed;

        // Primary: /v2/characters/{name}/maps
        // ParseCompletionBody returns true when the body contained valid
        // structured data (even if zero maps are complete).  Only fall back
        // to the full character endpoint when the primary produced no parsable
        // structure (e.g. 404, error object, unexpected format).
        bool primaryOk = false;
        {
            std::wstring path = L"/v2/characters/" + wEnc + L"/maps";
            std::string  body = HttpGet(kHost, path, auth);
            if (!body.empty())
                primaryOk = ParseCompletionBody(body, completed);
        }

        // Fallback: /v2/characters/{name}  — look for top-level "maps" array
        if (!primaryOk)
        {
            std::wstring path = L"/v2/characters/" + wEnc;
            std::string  body = HttpGet(kHost, path, auth);
            if (!body.empty())
            {
                try
                {
                    json j = json::parse(body);
                    if (j.is_object() && j.contains("maps") && j["maps"].is_array())
                        for (const auto& m : j["maps"])
                            if (m.contains("id") && IsMapDone(m))
                                completed.insert(m["id"].get<uint32_t>());
                }
                catch (...) {}
            }
        }

        if (m_callback)
            m_callback(m_charName, std::move(completed));

        m_state = State::Done;
    }

    std::atomic<State> m_state { State::Idle };
    std::string        m_charName;
    std::string        m_apiKey;
    Callback           m_callback;
};
