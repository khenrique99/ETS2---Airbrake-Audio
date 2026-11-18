// airbrake_audio: telemetry plugin for Euro Truck Simulator 2 / American Truck Simulator.
//
// READ-ONLY. It reads the brake pedal and the speed through the official SCS
// Telemetry SDK and plays sound. It does not change brake temperature, braking
// force or physics, and it never touches game memory, so it coexists with
// other brake-heating / telemetry plugins.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "web.h"   // before windows.h: pulls in winsock2.h first
#include <windows.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include "scs_sdk_min.h"
#include "config.h"
#include "audio.h"
#include "version.h"

namespace {

Config    g_config;
Engine    g_audio;
// Heap-allocated on purpose: a static would join its threads while the DLL unloads,
// which can deadlock under the loader lock if the game never calls shutdown.
WebUi*    g_web = nullptr;
Metrics   g_metrics;
scs_log_t g_gameLog = nullptr;
bool      g_comInitialized = false;
bool      g_ready = false;

// Values read from the game.
float g_pedal = 0.0f;       // 0..1
float g_speedMs = 0.0f;     // m/s

// Brake state.
bool   g_paused = false;
bool   g_braking = false;
bool   g_brakeVoiceRunning = false;
float  g_brakeVolume = 0.0f;
double g_pressedAt = 0.0;
double g_lastAirAt = -1e9;
double g_lastTickAt = 0.0;

// Air release currently playing.
bool   g_airPlaying = false;
bool   g_airIsCut = false;      // true when it must be closed early
double g_airStartedAt = 0.0;
double g_airCutAt = 0.0;        // seconds after start where the early cut ends
double g_airEndAt = 0.0;        // seconds after start where the whole sound ends
float  g_airVolume = 0.0f;

double nowSeconds() {
    static const LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return double(counter.QuadPart) / double(frequency.QuadPart);
}

void logMessage(scs_log_type_t type, const char* format, ...) {
    if (!g_gameLog) return;
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    g_gameLog(type, buffer);
}

// Directory (with trailing slash) of this DLL.
std::wstring moduleDirectory() {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&moduleDirectory), &module);
    wchar_t path[MAX_PATH * 2] = {};
    GetModuleFileNameW(module, path, DWORD(sizeof(path) / sizeof(path[0]) - 1));
    const std::wstring full = path;
    const size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
}

void stopAllSounds() {
    g_audio.brakeStop();
    g_audio.airStop();
    g_braking = false;
    g_brakeVoiceRunning = false;
    g_brakeVolume = 0.0f;
    g_airPlaying = false;
}

// Longer braking -> whole sound at full volume. Shorter braking -> quieter and cut earlier.
void playAirRelease(double heldMs, double now) {
    const double fullness = std::min(1.0, heldMs / 1000.0 / double(g_config.airFullS));
    const double length = double(g_audio.airSeconds());
    const double audible = std::min(length, double(g_config.airAudibleS));
    const double shortest = double(g_config.airShortLenS);

    g_airVolume = g_config.airVolume * (g_config.airShortVol + (1.0f - g_config.airShortVol) * float(fullness));
    g_airIsCut = fullness < 1.0;
    g_airCutAt = shortest + (audible - shortest) * fullness;
    g_airEndAt = length + 0.05;
    g_airStartedAt = now;
    g_airPlaying = true;
    g_audio.airPlay(g_airVolume);
}

void updateTrigger(double now) {
    if (!g_braking && g_pedal >= g_config.thresholdOn) {
        g_braking = true;
        g_pressedAt = now;
        if (!g_brakeVoiceRunning) {
            g_audio.brakeStart();
            g_brakeVoiceRunning = true;
            g_brakeVolume = 0.0f;
        }
    } else if (g_braking && g_pedal <= g_config.thresholdOff) {
        g_braking = false;
        const double heldMs = (now - g_pressedAt) * 1000.0;
        const bool heldLongEnough = heldMs >= double(g_config.airMinHoldMs);
        const bool cooledDown = (now - g_lastAirAt) * 1000.0 >= double(g_config.airCooldownMs);
        if (heldLongEnough && cooledDown) {
            playAirRelease(heldMs, now);
            g_lastAirAt = now;
        }
    }
}

// Brake volume follows the pedal; fades smooth every change.
void updateBrakeVolume(float dt) {
    if (!g_brakeVoiceRunning) return;

    const float speedKmh = std::fabs(g_speedMs) * 3.6f;
    float target = 0.0f;
    if (g_braking && speedKmh >= g_config.minSpeedKmh) {
        target = g_config.volMin + (g_config.volMax - g_config.volMin) * std::min(1.0f, g_pedal);
    }

    if (g_brakeVolume < target) {
        g_brakeVolume = g_config.fadeInMs > 0.0f
                            ? std::min(target, g_brakeVolume + dt / (g_config.fadeInMs / 1000.0f))
                            : target;
    } else {
        g_brakeVolume = g_config.fadeOutMs > 0.0f
                            ? std::max(target, g_brakeVolume - dt / (g_config.fadeOutMs / 1000.0f))
                            : target;
    }

    if (!g_braking && g_brakeVolume <= 0.0005f) {
        g_audio.brakeStop();
        g_brakeVoiceRunning = false;
        g_brakeVolume = 0.0f;
    } else {
        g_audio.brakeSetVolume(g_brakeVolume);
    }
}

// A cut air release closes with an exponential fade (-80 dB) instead of stopping dead.
void updateAirRelease(double now) {
    if (!g_airPlaying) return;
    const double elapsed = now - g_airStartedAt;

    if (g_airIsCut) {
        const double fade = double(g_config.airCutFadeMs) / 1000.0;
        const double fadeStart = g_airCutAt - fade;
        if (elapsed >= g_airCutAt) {
            g_audio.airStop();
            g_airPlaying = false;
        } else if (elapsed >= fadeStart) {
            g_audio.airSetVolume(g_airVolume * float(std::pow(1e-4, (elapsed - fadeStart) / fade)));
        }
    } else if (elapsed >= g_airEndAt) {
        g_audio.airStop();
        g_airPlaying = false;
    }
}

// Applies settings changed from the settings page. Changing the boost re-bakes the
// sounds, so it silences them first (the voices read those buffers).
void applyPendingConfig() {
    Config updated;
    if (!g_web && g_web->takePending(updated)) return;
    const bool boostChanged = updated.boostDb != g_config.boostDb;
    g_config = updated;
    if (boostChanged) {
        stopAllSounds();
        g_audio.setBoost(g_config.boostDb);
    }
}

void publishMetrics(double now) {
    g_metrics.pedal = g_pedal;
    g_metrics.speedKmh = std::fabs(g_speedMs) * 3.6f;
    g_metrics.heldS = g_braking ? float(now - g_pressedAt) : 0.0f;
    g_metrics.state = g_airPlaying ? 2 : (g_brakeVoiceRunning && g_brakeVolume > 0.01f ? 1 : 0);
}

void onFrameEnd() {
    if (!g_ready) return;
    applyPendingConfig();
    if (g_paused) return;
    const double now = nowSeconds();
    const float dt = float(std::min(0.1, std::max(0.0, now - g_lastTickAt)));
    g_lastTickAt = now;

    updateTrigger(now);
    updateBrakeVolume(dt);
    updateAirRelease(now);
    publishMetrics(now);
}

void onChannel(const scs_string_t, const scs_u32_t, const scs_value_t* value, const scs_context_t context) {
    float* destination = static_cast<float*>(context);
    if (!destination) return;
    *destination = (value && value->type == SCS_VALUE_TYPE_float) ? value->value_float : 0.0f;
}

void onEvent(const scs_event_t event, const void*, const scs_context_t) {
    switch (event) {
    case SCS_TELEMETRY_EVENT_frame_end:
        onFrameEnd();
        break;
    case SCS_TELEMETRY_EVENT_paused:
        g_paused = true;
        if (g_ready) stopAllSounds();
        break;
    case SCS_TELEMETRY_EVENT_started:
        g_paused = false;
        g_lastTickAt = nowSeconds();
        break;
    default:
        break;
    }
}

} // namespace

extern "C" {

__declspec(dllexport) scs_result_t scs_telemetry_init(const scs_u32_t version,
                                                      const scs_telemetry_init_params_v100_t* params) {
    // The game retries with an older version when we answer "unsupported".
    if (version != SCS_TELEMETRY_VERSION_1_00 && version != SCS_TELEMETRY_VERSION_1_01) {
        return SCS_RESULT_unsupported;
    }
    if (!params) return SCS_RESULT_invalid_parameter;
    g_gameLog = params->common.log;

    const std::wstring directory = moduleDirectory();
    const std::wstring iniPath = directory + L"airbrake_audio.ini";
    g_config = loadConfig(iniPath);

    // If the game already initialized COM in another mode this fails harmlessly.
    g_comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    std::string error;
    if (!g_audio.init(g_config, directory + L"airbrake_audio\\", error)) {
        logMessage(SCS_LOG_TYPE_error, "[airbrake_audio] %s", error.c_str());
        if (g_comInitialized) { CoUninitialize(); g_comInitialized = false; }
        return SCS_RESULT_generic_error;
    }

    const scs_u32_t channelFlags = SCS_TELEMETRY_CHANNEL_FLAG_each_frame | SCS_TELEMETRY_CHANNEL_FLAG_no_value;
    bool registered =
        params->register_for_event(SCS_TELEMETRY_EVENT_frame_end, onEvent, nullptr) == SCS_RESULT_ok &&
        params->register_for_event(SCS_TELEMETRY_EVENT_paused, onEvent, nullptr) == SCS_RESULT_ok &&
        params->register_for_event(SCS_TELEMETRY_EVENT_started, onEvent, nullptr) == SCS_RESULT_ok &&
        params->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_input_brake, SCS_U32_NIL,
                                     SCS_VALUE_TYPE_float, channelFlags, onChannel, &g_pedal) == SCS_RESULT_ok &&
        params->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_speed, SCS_U32_NIL,
                                     SCS_VALUE_TYPE_float, channelFlags, onChannel, &g_speedMs) == SCS_RESULT_ok;
    if (!registered) {
        logMessage(SCS_LOG_TYPE_error, "[airbrake_audio] failed to register events/channels");
        g_audio.shutdown();
        if (g_comInitialized) { CoUninitialize(); g_comInitialized = false; }
        return SCS_RESULT_generic_error;
    }

    g_web = new WebUi;
    g_web->start(g_config, iniPath, &g_metrics);   // hotkey + settings page (no-op when web_ui=off)

    g_paused = false;   // only silence when the game says it paused
    g_lastTickAt = nowSeconds();
    g_ready = true;
    logMessage(SCS_LOG_TYPE_message, "[airbrake_audio] v" AIRBRAKE_AUDIO_VERSION " ready (boost %.1f dB, brake=%ls, air=%ls)",
               g_config.boostDb, g_config.brakeFile.c_str(), g_config.airFile.c_str());
    return SCS_RESULT_ok;
}

__declspec(dllexport) void scs_telemetry_shutdown(void) {
    if (g_web) {
        g_web->stop();
        delete g_web;
        g_web = nullptr;
    }
    if (g_ready) {
        stopAllSounds();
        g_ready = false;
    }
    g_audio.shutdown();
    if (g_comInitialized) {
        CoUninitialize();
        g_comInitialized = false;
    }
}

} // extern "C"
