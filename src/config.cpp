#include "config.h"

#include "shared/hotkey_combo.h"
#include "shared/ini_edit.h"
#include "shared/ipc.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>
#include <vector>

namespace aa {

namespace {

const FloatField kFields[] = {
    {"output", "boost_db", &Config::boostDb, 0.0f, 18.0f},
    {"brake", "loop_start_s", &Config::loopStartS, 0.0f, 600.0f, false},
    {"brake", "fade_in_ms", &Config::fadeInMs, 0.0f, 5000.0f},
    {"brake", "fade_out_ms", &Config::fadeOutMs, 0.0f, 5000.0f},
    {"brake", "volume_min", &Config::volMin, 0.0f, 1.0f},
    {"brake", "volume_max", &Config::volMax, 0.0f, 1.0f},
    {"brake", "min_speed_kmh", &Config::minSpeedKmh, 0.0f, 200.0f},
    {"air_release", "volume", &Config::airVolume, 0.0f, 1.0f},
    {"air_release", "full_brake_time_s", &Config::airFullS, 0.5f, 120.0f},
    {"air_release", "short_volume", &Config::airShortVol, 0.0f, 1.0f},
    {"air_release", "short_length_s", &Config::airShortLenS, 0.05f, 10.0f},
    {"air_release", "audible_length_s", &Config::airAudibleS, 0.1f, 10.0f},
    {"air_release", "cut_fade_ms", &Config::airCutFadeMs, 5.0f, 2000.0f},
    {"air_release", "min_brake_time_ms", &Config::airMinHoldMs, 0.0f, 10000.0f},
    {"air_release", "cooldown_ms", &Config::airCooldownMs, 0.0f, 60000.0f},
    {"trigger", "threshold_on", &Config::thresholdOn, 0.01f, 0.9f},
    {"trigger", "threshold_off", &Config::thresholdOff, 0.0f, 0.9f},
};

std::string lower(std::string s) {
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool isOff(const std::string &v) {
    const std::string l = lower(v);
    return l == "off" || l == "0" || l == "false" || l == "no";
}

} // namespace

const FloatField *floatFields(size_t &count) {
    count = sizeof(kFields) / sizeof(kFields[0]);
    return kFields;
}

const FloatField *findField(const std::string &id) {
    for (const FloatField &f : kFields)
        if (f.id() == id) return &f;
    return nullptr;
}

void sanitize(Config &c) {
    for (const FloatField &f : kFields) {
        float &v = c.*(f.member);
        v = std::min(f.high, std::max(f.low, v));
    }
    if (c.thresholdOff >= c.thresholdOn) c.thresholdOff = c.thresholdOn * 0.5f;
    if (c.volMax < c.volMin) std::swap(c.volMin, c.volMax);
    if (c.airShortLenS > c.airAudibleS) c.airShortLenS = c.airAudibleS;
}

std::string formatNumber(float v) {
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << v;
    return text.str();
}

bool parseNumber(const std::string &text, float &out) {
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    float v = 0.0f;
    if (!(in >> v)) return false;
    char extra = 0;
    if (in >> extra) return false; // "1.5abc"
    out = v;
    return true;
}

Config configFromIni(const std::string &iniText) {
    Config c;
    for (const FloatField &f : kFields) parseNumber(ini::get(iniText, f.section, f.key), c.*(f.member));

    const std::string brake = ini::get(iniText, "brake", "file");
    if (!brake.empty()) c.brakeFile = ipc::widen(brake);
    const std::string air = ini::get(iniText, "air_release", "file");
    if (!air.empty()) c.airFile = ipc::widen(air);

    std::string app = ini::get(iniText, "ui", "app");
    if (app.empty()) app = ini::get(iniText, "ui", "web_ui"); // nome da 1.x
    c.appEnabled = app.empty() || !isOff(app);

    hotkey::Combo combo;
    const std::string key = ini::get(iniText, "ui", "hotkey");
    if (hotkey::parse(key, combo)) c.hotkey = key;

    sanitize(c);
    return c;
}

std::string applyConfigToIni(const std::string &iniText, const Config &c) {
    std::vector<ini::Value> values;
    for (const FloatField &f : kFields) values.push_back({f.section, f.key, formatNumber(c.*(f.member))});
    return ini::apply(iniText, values);
}

Config loadConfig(const std::wstring &iniPath) {
    std::string text;
    ipc::readAll(iniPath, text);
    return configFromIni(text);
}

bool saveConfig(const std::wstring &iniPath, const Config &c) {
    std::string text;
    ipc::readAll(iniPath, text);
    return ipc::writeAtomic(iniPath, applyConfigToIni(text, c));
}

std::wstring pluginDirectory() {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&pluginDirectory), &module);
    wchar_t path[MAX_PATH * 2] = {};
    GetModuleFileNameW(module, path, static_cast<DWORD>(sizeof(path) / sizeof(path[0]) - 1));
    const std::wstring full = path;
    const size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
}

} // namespace aa
