// Reads airbrake_audio.ini (next to the DLL). Missing or invalid values fall back
// to defaults, and everything is clamped to safe ranges.
//
// Every numeric option is described once in kFloatFields. The same table drives
// loading the .ini, the settings page (web.h) and saving back to the .ini.
#pragma once
#include <windows.h>
#include <algorithm>
#include <cstddef>
#include <locale>
#include <sstream>
#include <string>

struct Config {
    // [output]
    float boostDb = 6.0f;               // extra gain applied at load time (soft-limited)

    // [brake]
    std::wstring brakeFile = L"brake_loop.wav";
    float loopStartS   = 2.0f;
    float fadeInMs     = 60.0f;
    float fadeOutMs    = 200.0f;
    float volMin       = 0.35f;         // volume with the pedal barely pressed
    float volMax       = 1.0f;          // volume with the pedal fully pressed
    float minSpeedKmh  = 3.0f;          // below this speed the brake sound is muted

    // [air_release]
    std::wstring airFile = L"air_release.wav";
    float airVolume     = 0.9f;
    float airFullS      = 6.0f;         // braking this long plays the whole sound at full volume
    float airShortVol   = 0.6f;         // volume multiplier after a short brake application
    float airShortLenS  = 0.45f;        // how long the release plays after a short brake application
    float airAudibleS   = 1.1f;         // how far the early cut can reach before the full sound is used
    float airCutFadeMs  = 120.0f;       // closing fade when the sound is cut early
    float airMinHoldMs  = 400.0f;       // shorter brake applications play no release sound
    float airCooldownMs = 1500.0f;

    // [trigger]
    float thresholdOn  = 0.05f;
    float thresholdOff = 0.02f;

    // [ui]
    bool        webUi   = true;         // settings page + hotkey
    std::string hotkey  = "Minus";      // KeyboardEvent.code of the key that opens the page
    float       webPort = 8770.0f;      // first port to try (127.0.0.1 only)
};

struct FloatField {
    const wchar_t* section;
    const wchar_t* key;
    float Config::* member;
    float low;
    float high;
};

inline const FloatField* floatFields(size_t& count) {
    static const FloatField fields[] = {
        {L"output",      L"boost_db",          &Config::boostDb,       0.0f, 18.0f},
        {L"brake",       L"loop_start_s",      &Config::loopStartS,    0.0f, 600.0f},
        {L"brake",       L"fade_in_ms",        &Config::fadeInMs,      0.0f, 5000.0f},
        {L"brake",       L"fade_out_ms",       &Config::fadeOutMs,     0.0f, 5000.0f},
        {L"brake",       L"volume_min",        &Config::volMin,        0.0f, 1.0f},
        {L"brake",       L"volume_max",        &Config::volMax,        0.0f, 1.0f},
        {L"brake",       L"min_speed_kmh",     &Config::minSpeedKmh,   0.0f, 200.0f},
        {L"air_release", L"volume",            &Config::airVolume,     0.0f, 1.0f},
        {L"air_release", L"full_brake_time_s", &Config::airFullS,      0.5f, 120.0f},
        {L"air_release", L"short_volume",      &Config::airShortVol,   0.0f, 1.0f},
        {L"air_release", L"short_length_s",    &Config::airShortLenS,  0.05f, 10.0f},
        {L"air_release", L"audible_length_s",  &Config::airAudibleS,   0.1f, 10.0f},
        {L"air_release", L"cut_fade_ms",       &Config::airCutFadeMs,  5.0f, 2000.0f},
        {L"air_release", L"min_brake_time_ms", &Config::airMinHoldMs,  0.0f, 10000.0f},
        {L"air_release", L"cooldown_ms",       &Config::airCooldownMs, 0.0f, 60000.0f},
        {L"trigger",     L"threshold_on",      &Config::thresholdOn,   0.01f, 0.9f},
        {L"trigger",     L"threshold_off",     &Config::thresholdOff,  0.0f, 0.9f},
    };
    count = sizeof(fields) / sizeof(fields[0]);
    return fields;
}

namespace cfgdetail {

inline std::wstring trim(std::wstring s) {
    const wchar_t* whitespace = L" \t\r\n";
    const size_t first = s.find_first_not_of(whitespace);
    if (first == std::wstring::npos) return L"";
    const size_t last = s.find_last_not_of(whitespace);
    return s.substr(first, last - first + 1);
}

// GetPrivateProfileString keeps trailing comments, so strip them here.
inline std::wstring readString(const std::wstring& path, const wchar_t* section, const wchar_t* key,
                               const std::wstring& fallback) {
    wchar_t buffer[512] = {};
    GetPrivateProfileStringW(section, key, fallback.c_str(), buffer, 512, path.c_str());
    std::wstring value = buffer;
    const size_t comment = value.find(L';');
    if (comment != std::wstring::npos) value.erase(comment);
    value = trim(value);
    return value.empty() ? fallback : value;
}

// Always uses a decimal point, regardless of the game's locale.
inline float readNumber(const std::wstring& path, const wchar_t* section, const wchar_t* key, float fallback) {
    const std::wstring text = readString(path, section, key, L"");
    if (text.empty()) return fallback;
    std::wistringstream stream(text);
    stream.imbue(std::locale::classic());
    float value = 0.0f;
    return (stream >> value) ? value : fallback;
}

inline float clamp(float value, float low, float high) { return std::min(high, std::max(low, value)); }

inline std::string toAscii(const std::wstring& text) {
    std::string result;
    for (wchar_t c : text) result += (c < 128 ? char(c) : '?');
    return result;
}

} // namespace cfgdetail

// Keeps the hysteresis valid and the ranges consistent. Used after every change.
inline void sanitizeConfig(Config& c) {
    size_t count = 0;
    const FloatField* fields = floatFields(count);
    for (size_t i = 0; i < count; ++i) {
        float& value = c.*(fields[i].member);
        value = cfgdetail::clamp(value, fields[i].low, fields[i].high);
    }
    c.webPort = cfgdetail::clamp(c.webPort, 1024.0f, 65500.0f);
    if (c.thresholdOff >= c.thresholdOn) c.thresholdOff = c.thresholdOn * 0.5f;
    if (c.volMax < c.volMin) std::swap(c.volMin, c.volMax);
    if (c.airShortLenS > c.airAudibleS) c.airShortLenS = c.airAudibleS;
}

inline Config loadConfig(const std::wstring& iniPath) {
    using namespace cfgdetail;
    Config c;

    size_t count = 0;
    const FloatField* fields = floatFields(count);
    for (size_t i = 0; i < count; ++i) {
        float& value = c.*(fields[i].member);
        value = readNumber(iniPath, fields[i].section, fields[i].key, value);
    }

    c.brakeFile = readString(iniPath, L"brake", L"file", c.brakeFile);
    c.airFile   = readString(iniPath, L"air_release", L"file", c.airFile);

    const std::wstring webUi = readString(iniPath, L"ui", L"web_ui", L"on");
    c.webUi = !(webUi == L"off" || webUi == L"0" || webUi == L"false" || webUi == L"no");
    const std::string hotkey = toAscii(readString(iniPath, L"ui", L"hotkey", L"Minus"));
    if (!hotkey.empty()) c.hotkey = hotkey;
    c.webPort = readNumber(iniPath, L"ui", L"port", c.webPort);

    sanitizeConfig(c);
    return c;
}

// Writes the tunable values back to the .ini. WritePrivateProfileString leaves
// the rest of the file (comments, order) as it is.
inline bool saveConfig(const std::wstring& iniPath, const Config& c) {
    size_t count = 0;
    const FloatField* fields = floatFields(count);
    bool ok = true;
    for (size_t i = 0; i < count; ++i) {
        std::wostringstream text;
        text.imbue(std::locale::classic());
        text << (c.*(fields[i].member));
        ok = WritePrivateProfileStringW(fields[i].section, fields[i].key, text.str().c_str(), iniPath.c_str()) && ok;
    }
    const std::wstring hotkey(c.hotkey.begin(), c.hotkey.end());
    ok = WritePrivateProfileStringW(L"ui", L"hotkey", hotkey.c_str(), iniPath.c_str()) && ok;
    return ok;
}
