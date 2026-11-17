// Reads airbrake_audio.ini (next to the DLL). Missing or invalid values fall back
// to defaults, and everything is clamped to safe ranges.
#pragma once
#include <windows.h>
#include <algorithm>
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
};

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

} // namespace cfgdetail

inline Config loadConfig(const std::wstring& iniPath) {
    using namespace cfgdetail;
    Config c;
    c.boostDb       = clamp(readNumber(iniPath, L"output", L"boost_db", c.boostDb), 0.0f, 18.0f);

    c.brakeFile     = readString(iniPath, L"brake", L"file", c.brakeFile);
    c.loopStartS    = clamp(readNumber(iniPath, L"brake", L"loop_start_s", c.loopStartS), 0.0f, 600.0f);
    c.fadeInMs      = clamp(readNumber(iniPath, L"brake", L"fade_in_ms", c.fadeInMs), 0.0f, 5000.0f);
    c.fadeOutMs     = clamp(readNumber(iniPath, L"brake", L"fade_out_ms", c.fadeOutMs), 0.0f, 5000.0f);
    c.volMin        = clamp(readNumber(iniPath, L"brake", L"volume_min", c.volMin), 0.0f, 1.0f);
    c.volMax        = clamp(readNumber(iniPath, L"brake", L"volume_max", c.volMax), 0.0f, 1.0f);
    c.minSpeedKmh   = clamp(readNumber(iniPath, L"brake", L"min_speed_kmh", c.minSpeedKmh), 0.0f, 200.0f);

    c.airFile       = readString(iniPath, L"air_release", L"file", c.airFile);
    c.airVolume     = clamp(readNumber(iniPath, L"air_release", L"volume", c.airVolume), 0.0f, 1.0f);
    c.airFullS      = clamp(readNumber(iniPath, L"air_release", L"full_brake_time_s", c.airFullS), 0.5f, 120.0f);
    c.airShortVol   = clamp(readNumber(iniPath, L"air_release", L"short_volume", c.airShortVol), 0.0f, 1.0f);
    c.airShortLenS  = clamp(readNumber(iniPath, L"air_release", L"short_length_s", c.airShortLenS), 0.05f, 10.0f);
    c.airAudibleS   = clamp(readNumber(iniPath, L"air_release", L"audible_length_s", c.airAudibleS), 0.1f, 10.0f);
    c.airCutFadeMs  = clamp(readNumber(iniPath, L"air_release", L"cut_fade_ms", c.airCutFadeMs), 5.0f, 2000.0f);
    c.airMinHoldMs  = clamp(readNumber(iniPath, L"air_release", L"min_brake_time_ms", c.airMinHoldMs), 0.0f, 10000.0f);
    c.airCooldownMs = clamp(readNumber(iniPath, L"air_release", L"cooldown_ms", c.airCooldownMs), 0.0f, 60000.0f);

    c.thresholdOn   = clamp(readNumber(iniPath, L"trigger", L"threshold_on", c.thresholdOn), 0.01f, 0.9f);
    c.thresholdOff  = clamp(readNumber(iniPath, L"trigger", L"threshold_off", c.thresholdOff), 0.0f, 0.9f);

    // Keep the hysteresis valid and the ranges consistent.
    if (c.thresholdOff >= c.thresholdOn) c.thresholdOff = c.thresholdOn * 0.5f;
    if (c.volMax < c.volMin) std::swap(c.volMin, c.volMax);
    if (c.airShortLenS > c.airAudibleS) c.airShortLenS = c.airAudibleS;
    return c;
}
