#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct MapInfo
{
    uint32_t    id;
    std::string name;
    std::string region;       // sub-region for collapsing headers within a tab
    std::string tab;          // which tab this map belongs to
    std::string completionText;
};

// Tab names — used in UI tab bar
#define TAB_CORE      "Core Tyria + LW1/2"
#define TAB_HOT       "HoT + LW3"
#define TAB_POF       "PoF + LW4"
#define TAB_ICEBROOD  "Icebrood Saga"
#define TAB_EOD       "End of Dragons"
#define TAB_SOTO      "Secrets of the Obscure"
#define TAB_JANTHIR   "Janthir Wilds"
#define TAB_VOE       "Visions of Eternity"

// All map IDs verified against api.guildwars2.com/v2/maps
inline const std::vector<MapInfo>& GetAllMaps()
{
    static std::vector<MapInfo> maps =
    {
        // =========================================================
        // TAB: Core Tyria + Living World Season 1 & 2
        // =========================================================

        // ---- Kryta ----
        {  15,  "Queensdale",          "Kryta",          TAB_CORE, "Queensdale" },
        {  18,  "Divinity's Reach",    "Kryta",          TAB_CORE, "Divinity's Reach" },
        {  23,  "Kessex Hills",        "Kryta",          TAB_CORE, "Kessex Hills" },
        {  24,  "Gendarran Fields",    "Kryta",          TAB_CORE, "Gendarran Fields" },
        {  50,  "Lion's Arch",         "Kryta",          TAB_CORE, "Lion's Arch" },
        {  73,  "Bloodtide Coast",     "Kryta",          TAB_CORE, "Bloodtide Coast" },

        // ---- Shiverpeak Mountains ----
        {  31,  "Snowden Drifts",      "Shiverpeak",     TAB_CORE, "Snowden Drifts" },
        {  27,  "Lornar's Pass",       "Shiverpeak",     TAB_CORE, "Lornar's Pass" },
        {  26,  "Dredgehaunt Cliffs",  "Shiverpeak",     TAB_CORE, "Dredgehaunt Cliffs" },
        {  29,  "Timberline Falls",    "Shiverpeak",     TAB_CORE, "Timberline Falls" },
        {  30,  "Frostgorge Sound",    "Shiverpeak",     TAB_CORE, "Frostgorge Sound" },
        {  28,  "Wayfarer Foothills",  "Shiverpeak",     TAB_CORE, "Wayfarer Foothills" },
        { 326,  "Hoelbrak",            "Shiverpeak",     TAB_CORE, "Hoelbrak" },

        // ---- Ascalon ----
        {  21,  "Fields of Ruin",      "Ascalon",        TAB_CORE, "Fields of Ruin" },
        {  20,  "Blazeridge Steppes",  "Ascalon",        TAB_CORE, "Blazeridge Steppes" },
        {  25,  "Iron Marches",        "Ascalon",        TAB_CORE, "Iron Marches" },
        {  22,  "Fireheart Rise",      "Ascalon",        TAB_CORE, "Fireheart Rise" },
        {  19,  "Plains of Ashford",   "Ascalon",        TAB_CORE, "Plains of Ashford" },
        {  32,  "Diessa Plateau",      "Ascalon",        TAB_CORE, "Diessa Plateau" },
        { 218,  "Black Citadel",       "Ascalon",        TAB_CORE, "Black Citadel" },

        // ---- Maguuma Jungle ----
        {  39,  "Mount Maelstrom",     "Maguuma Jungle", TAB_CORE, "Mount Maelstrom" },
        {  53,  "Sparkfly Fen",        "Maguuma Jungle", TAB_CORE, "Sparkfly Fen" },
        {  54,  "Brisban Wildlands",   "Maguuma Jungle", TAB_CORE, "Brisban Wildlands" },
        {  91,  "The Grove",           "Maguuma Jungle", TAB_CORE, "The Grove" },
        {  34,  "Caledon Forest",      "Maguuma Jungle", TAB_CORE, "Caledon Forest" },
        {  35,  "Metrica Province",    "Maguuma Jungle", TAB_CORE, "Metrica Province" },
        { 139,  "Rata Sum",            "Maguuma Jungle", TAB_CORE, "Rata Sum" },

        // ---- Ruins of Orr ----
        {  51,  "Straits of Devastation", "Ruins of Orr", TAB_CORE, "Straits of Devastation" },
        {  62,  "Cursed Shore",           "Ruins of Orr", TAB_CORE, "Cursed Shore" },
        {  65,  "Malchor's Leap",         "Ruins of Orr", TAB_CORE, "Malchor's Leap" },

        // ---- Living World Season 1 ----
        { 873,  "Southsun Cove",       "LW Season 1",    TAB_CORE, "Southsun Cove" },

        // ---- Living World Season 2 ----
        { 988,  "Dry Top",             "LW Season 2",    TAB_CORE, "Dry Top" },
        { 1015, "The Silverwastes",    "LW Season 2",    TAB_CORE, "The Silverwastes" },

        // =========================================================
        // TAB: Heart of Thorns + Living World Season 3
        // =========================================================

        // ---- Heart of Thorns ----
        { 1052, "Verdant Brink",       "Heart of Thorns", TAB_HOT, "Verdant Brink" },
        { 1043, "Auric Basin",         "Heart of Thorns", TAB_HOT, "Auric Basin" },
        { 1045, "Tangled Depths",      "Heart of Thorns", TAB_HOT, "Tangled Depths" },
        { 1041, "Dragon's Stand",      "Heart of Thorns", TAB_HOT, "Dragon's Stand" },

        // ---- Living World Season 3 ----
        { 1165, "Bloodstone Fen",      "LW Season 3",    TAB_HOT, "Bloodstone Fen" },
        { 1175, "Ember Bay",           "LW Season 3",    TAB_HOT, "Ember Bay" },
        { 1178, "Bitterfrost Frontier","LW Season 3",    TAB_HOT, "Bitterfrost Frontier" },
        { 1185, "Lake Doric",          "LW Season 3",    TAB_HOT, "Lake Doric" },
        { 1195, "Draconis Mons",       "LW Season 3",    TAB_HOT, "Draconis Mons" },
        { 1203, "Siren's Landing",     "LW Season 3",    TAB_HOT, "Siren's Landing" },

        // =========================================================
        // TAB: Path of Fire + Living World Season 4
        // =========================================================

        // ---- Path of Fire ----
        { 1210, "Crystal Oasis",       "Path of Fire",   TAB_POF, "Crystal Oasis" },
        { 1211, "Desert Highlands",    "Path of Fire",   TAB_POF, "Desert Highlands" },
        { 1228, "Elon Riverlands",     "Path of Fire",   TAB_POF, "Elon Riverlands" },
        { 1226, "The Desolation",      "Path of Fire",   TAB_POF, "The Desolation" },
        { 1248, "Domain of Vabbi",     "Path of Fire",   TAB_POF, "Domain of Vabbi" },

        // ---- Living World Season 4 ----
        { 1240, "Domain of Istan",     "LW Season 4",    TAB_POF, "Domain of Istan" },
        { 1265, "Sandswept Isles",     "LW Season 4",    TAB_POF, "Sandswept Isles" },
        { 1288, "Domain of Kourna",    "LW Season 4",    TAB_POF, "Domain of Kourna" },
        { 1301, "Jahai Bluffs",        "LW Season 4",    TAB_POF, "Jahai Bluffs" },
        { 1310, "Thunderhead Peaks",   "LW Season 4",    TAB_POF, "Thunderhead Peaks" },
        { 1317, "Dragonfall",          "LW Season 4",    TAB_POF, "Dragonfall" },

        // =========================================================
        // TAB: Icebrood Saga (LW Season 5)
        // =========================================================
        { 1330, "Grothmar Valley",     "Icebrood Saga",  TAB_ICEBROOD, "Grothmar Valley" },
        { 1300, "Bjora Marches",       "Icebrood Saga",  TAB_ICEBROOD, "Bjora Marches" },
        { 1370, "Drizzlewood Coast",   "Icebrood Saga",  TAB_ICEBROOD, "Drizzlewood Coast" },

        // =========================================================
        // TAB: End of Dragons
        // =========================================================
        { 1442, "Seitung Province",    "Cantha",         TAB_EOD, "Seitung Province" },
        { 1438, "New Kaineng City",    "Cantha",         TAB_EOD, "New Kaineng City" },
        { 1452, "The Echovald Wilds",  "Cantha",         TAB_EOD, "The Echovald Wilds" },
        { 1422, "Dragon's End",        "Cantha",         TAB_EOD, "Dragon's End" },
        { 1490, "Gyala Delve",         "Cantha",         TAB_EOD, "Gyala Delve" },

        // =========================================================
        // TAB: Secrets of the Obscure
        // =========================================================
        { 1510, "Skywatch Archipelago","Realm of Dreams", TAB_SOTO, "Skywatch Archipelago" },
        { 1517, "Amnytas",             "Realm of Dreams", TAB_SOTO, "Amnytas" },
        { 1526, "Inner Nayos",         "Nayos",           TAB_SOTO, "Inner Nayos" },

        // =========================================================
        // TAB: Janthir Wilds
        // =========================================================
        { 1550, "Lowland Shore",       "Janthir",        TAB_JANTHIR, "Lowland Shore" },
        { 1554, "Janthir Syntri",      "Janthir",        TAB_JANTHIR, "Janthir Syntri" },
        { 1575, "Mistburned Barrens",  "Janthir",        TAB_JANTHIR, "Mistburned Barrens" },
        { 1574, "Bava Nisos",          "Janthir",        TAB_JANTHIR, "Bava Nisos" },

        // =========================================================
        // TAB: Visions of Eternity
        // =========================================================
        { 1595, "Shipwreck Strand",    "Castora",        TAB_VOE, "Shipwreck Strand" },
        { 1593, "Starlit Weald",       "Castora",        TAB_VOE, "Starlit Weald" },
    };
    return maps;
}

// Build a name->MapInfo lookup (case-insensitive)
inline const std::unordered_map<std::string, const MapInfo*>& GetMapsByName()
{
    static std::unordered_map<std::string, const MapInfo*> index;
    if (index.empty())
        for (const auto& m : GetAllMaps())
        {
            std::string key = m.name;
            for (auto& c : key) c = (char)tolower((unsigned char)c);
            index[key] = &m;
        }
    return index;
}

// Build a mapId->MapInfo lookup
inline const std::unordered_map<uint32_t, const MapInfo*>& GetMapsById()
{
    static std::unordered_map<uint32_t, const MapInfo*> index;
    if (index.empty())
        for (const auto& m : GetAllMaps())
            index[m.id] = &m;
    return index;
}

// Returns ordered list of unique tab names
inline const std::vector<std::string>& GetTabOrder()
{
    static std::vector<std::string> tabs = {
        TAB_CORE,
        TAB_HOT,
        TAB_POF,
        TAB_ICEBROOD,
        TAB_EOD,
        TAB_SOTO,
        TAB_JANTHIR,
        TAB_VOE,
    };
    return tabs;
}
