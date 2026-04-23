#pragma once

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GW2API
{
public:
    enum class State { Idle, Fetching, Done, Error };

    void Fetch(const std::string& apiKey)
    {
        if (m_state == State::Fetching) return;
        m_state    = State::Fetching;
        m_errorMsg = "";
        m_apiKey   = apiKey;
        std::thread([this]() { DoFetch(); }).detach();
    }

    State GetState() const { return m_state.load(); }

    std::vector<std::string> GetCharacterNames() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_characters;
    }

    const std::string& GetErrorMessage() const { return m_errorMsg; }

    void Reset() { m_state = State::Idle; m_errorMsg = ""; }

private:
    void DoFetch()
    {
        std::wstring wApiKey(m_apiKey.begin(), m_apiKey.end());
        std::wstring headers = L"Authorization: Bearer " + wApiKey + L"\r\n";

        HINTERNET hSession = WinHttpOpen(
            L"GW2MapCompletionTracker/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        if (!hSession) { Finish({}, "WinHTTP failed to open session."); return; }

        HINTERNET hConnect = WinHttpConnect(
            hSession, L"api.guildwars2.com",
            INTERNET_DEFAULT_HTTPS_PORT, 0
        );
        if (!hConnect) { WinHttpCloseHandle(hSession); Finish({}, "Failed to connect."); return; }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"GET",
            L"/v2/characters?ids=all&schema=2019-02-21T00:00:00.000Z",
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Finish({}, "Failed to create request.");
            return;
        }

        BOOL sent = WinHttpSendRequest(
            hRequest, headers.c_str(), (DWORD)-1L,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0
        );
        if (!sent || !WinHttpReceiveResponse(hRequest, nullptr))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Finish({}, "Request failed. Check your API key.");
            return;
        }

        std::string body;
        DWORD bytesAvail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
        {
            std::string chunk(bytesAvail, '\0');
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, &chunk[0], bytesAvail, &bytesRead);
            body.append(chunk, 0, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        try
        {
            json j = json::parse(body);
            if (j.is_array())
            {
                std::vector<std::string> result;
                for (const auto& c : j)
                    if (c.contains("name") && c["name"].is_string())
                        result.push_back(c["name"].get<std::string>());
                Finish(result, "");
            }
            else if (j.contains("text"))
                Finish({}, "API error: " + j["text"].get<std::string>());
            else
                Finish({}, "Unexpected API response.");
        }
        catch (...)
        {
            Finish({}, "Failed to parse API response.");
        }
    }

    void Finish(const std::vector<std::string>& chars, const std::string& error)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_characters = chars;
            m_errorMsg   = error;
        }
        m_state = error.empty() ? State::Done : State::Error;
    }

    std::atomic<State>       m_state { State::Idle };
    mutable std::mutex       m_mutex;
    std::vector<std::string> m_characters;
    std::string              m_apiKey;
    std::string              m_errorMsg;
};
