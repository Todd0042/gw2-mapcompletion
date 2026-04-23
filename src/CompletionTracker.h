#pragma once

#include <windows.h>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static constexpr const char* UNKNOWN_CHARACTER = "__unknown__";

class CompletionTracker
{
public:
    CompletionTracker() = default;

    void Init(const std::string& dataDir)
    {
        m_savePath = dataDir.empty() ? "completion.json" : dataDir + "\\completion.json";
        Load();
    }

    void SetActiveCharacter(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = name.empty() ? UNKNOWN_CHARACTER : name;
        if (key == m_activeCharacter) return;
        m_activeCharacter = key;
        // Ensure entry exists in completion data
        if (m_data.find(key) == m_data.end())
            m_data[key] = {};
    }

    // Called when the API fetch returns — saves the full character list
    void RegisterCharacters(const std::vector<std::string>& names)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool changed = false;
        for (const auto& name : names)
        {
            if (name.empty() || name == UNKNOWN_CHARACTER) continue;
            if (m_knownCharacters.insert(name).second)
                changed = true;
            // Also ensure a completion entry exists
            if (m_data.find(name) == m_data.end())
                m_data[name] = {};
        }
        if (changed) Save();
    }

    // Called for a single character (e.g. from MumbleLink)
    void RegisterCharacter(const std::string& name)
    {
        if (name.empty() || name == UNKNOWN_CHARACTER) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        bool changed = m_knownCharacters.insert(name).second;
        if (m_data.find(name) == m_data.end())
            m_data[name] = {};
        if (changed) Save();
    }

    const std::string& GetActiveCharacter() const { return m_activeCharacter; }

    // Returns all known characters — both from API fetch and from MumbleLink
    std::vector<std::string> GetCharacterNames() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> names(m_knownCharacters.begin(), m_knownCharacters.end());
        std::sort(names.begin(), names.end());
        return names;
    }

    void MarkComplete(uint32_t mapId)   { MarkCompleteFor(m_activeCharacter, mapId); }
    void MarkIncomplete(uint32_t mapId) { MarkIncompleteFor(m_activeCharacter, mapId); }
    bool IsComplete(uint32_t mapId) const { return IsCompleteFor(m_activeCharacter, mapId); }
    size_t CompletedCount() const         { return CompletedCountFor(m_activeCharacter); }

    void MarkCompleteFor(const std::string& character, uint32_t mapId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = character.empty() ? UNKNOWN_CHARACTER : character;
        if (m_data[key].insert(mapId).second) Save();
    }

    void MarkIncompleteFor(const std::string& character, uint32_t mapId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = character.empty() ? UNKNOWN_CHARACTER : character;
        if (m_data[key].erase(mapId)) Save();
    }

    bool IsCompleteFor(const std::string& character, uint32_t mapId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = character.empty() ? UNKNOWN_CHARACTER : character;
        auto it = m_data.find(key);
        if (it == m_data.end()) return false;
        return it->second.count(mapId) > 0;
    }

    size_t CompletedCountFor(const std::string& character) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = character.empty() ? UNKNOWN_CHARACTER : character;
        auto it = m_data.find(key);
        if (it == m_data.end()) return 0;
        return it->second.size();
    }

    // API key persistence
    void SaveApiKey(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_savedApiKey = key;
        Save();
    }

    std::string GetSavedApiKey() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_savedApiKey;
    }

private:
    void Load()
    {
        std::ifstream f(m_savePath);
        if (!f.is_open()) return;
        try
        {
            json j = json::parse(f);

            // Load known character list
            if (j.contains("known_characters") && j["known_characters"].is_array())
                for (const auto& name : j["known_characters"])
                    if (name.is_string())
                        m_knownCharacters.insert(name.get<std::string>());

            // Load completion data
            if (j.contains("characters") && j["characters"].is_object())
            {
                for (auto& [charName, ids] : j["characters"].items())
                {
                    if (!ids.is_array()) continue;
                    // Any character with completion data is also a known character
                    if (charName != UNKNOWN_CHARACTER)
                        m_knownCharacters.insert(charName);
                    for (auto& id : ids)
                        m_data[charName].insert(id.get<uint32_t>());
                }
            }
            else if (j.contains("completed") && j["completed"].is_array())
            {
                for (auto& id : j["completed"])
                    m_data[UNKNOWN_CHARACTER].insert(id.get<uint32_t>());
            }

            // Load API key
            if (j.contains("api_key") && j["api_key"].is_string())
                m_savedApiKey = j["api_key"].get<std::string>();
        }
        catch (...) {}
    }

    void Save()
    {
        try
        {
            json j;

            // Save full known character list
            j["known_characters"] = json::array();
            for (const auto& name : m_knownCharacters)
                j["known_characters"].push_back(name);

            // Save completion data
            j["characters"] = json::object();
            for (const auto& [charName, ids] : m_data)
            {
                j["characters"][charName] = json::array();
                for (auto id : ids)
                    j["characters"][charName].push_back(id);
            }

            if (!m_savedApiKey.empty())
                j["api_key"] = m_savedApiKey;

            std::ofstream f(m_savePath);
            f << j.dump(2);
        }
        catch (...) {}
    }

    mutable std::mutex                                             m_mutex;
    std::unordered_map<std::string, std::unordered_set<uint32_t>> m_data;
    std::unordered_set<std::string>                                m_knownCharacters;
    std::string                                                    m_activeCharacter = UNKNOWN_CHARACTER;
    std::string                                                    m_savePath;
    std::string                                                    m_savedApiKey;
};
