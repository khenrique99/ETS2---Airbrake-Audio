// Airbrake Audio: plugin de telemetria para Euro Truck Simulator 2 / American Truck Simulator.
//
// SO LEITURA. Le o pedal de freio e a velocidade pelo SCS Telemetry SDK oficial e toca som.
// Nao muda temperatura do freio, forca de frenagem nem fisica, e nunca toca na memoria do
// jogo: convive com outros plugins de freio/telemetria.
//
// Aqui ficam so as entradas do SDK e a ligacao entre os componentes:
//   BrakeSounds (logica)  ->  AudioEngine (XAudio2)
//   AppBridge (thread)    <->  AirbrakeAudio.exe (status.json / command.txt)
#include "app_bridge.h"
#include "audio_engine.h"
#include "brake_sounds.h"
#include "config.h"
#include "log.h"
#include "scs_sdk_min.h"
#include "version.h"

#include <objbase.h>
#include <windows.h>

#include <cmath>
#include <memory>
#include <string>

namespace aa {

namespace {

struct Plugin {
    Config config;
    AudioEngine audio;
    BrakeSounds sounds{audio, config};
    Metrics metrics;
    std::unique_ptr<AppBridge> bridge;
    bool comInitialized = false;
    bool paused = true;

    // Valores dos canais (escritos pelo jogo antes do frame_end).
    float pedal = 0.0f;   // 0..1
    float speedMs = 0.0f; // m/s
};

// Alocado no heap de proposito: um objeto estatico juntaria as threads no descarregamento da
// DLL, o que pode travar sob o loader lock se o jogo nao chamar shutdown.
Plugin *g_plugin = nullptr;

double nowSeconds() {
    static const LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
}

void publishMetrics(Plugin &p, double now, float speedKmh) {
    p.metrics.pedal = p.pedal;
    p.metrics.speedKmh = speedKmh;
    p.metrics.heldS = p.sounds.heldSeconds(now);
    p.metrics.sound = static_cast<int>(p.sounds.state());
}

void onFrameEnd(Plugin &p) {
    Config updated;
    if (p.bridge && p.bridge->takePending(updated)) p.sounds.applyConfig(updated);
    if (p.paused) return;

    const double now = nowSeconds();
    const float speedKmh = std::fabs(p.speedMs) * 3.6f;
    p.sounds.update(p.pedal, speedKmh, now);
    publishMetrics(p, now, speedKmh);
}

void onChannel(const scs_string_t, const scs_u32_t, const scs_value_t *value, const scs_context_t context) {
    auto *destination = static_cast<float *>(context);
    if (destination) *destination = (value && value->type == SCS_VALUE_TYPE_float) ? value->value_float : 0.0f;
}

void onEvent(const scs_event_t event, const void *, const scs_context_t) {
    if (!g_plugin) return;
    Plugin &p = *g_plugin;
    switch (event) {
    case SCS_TELEMETRY_EVENT_frame_end: onFrameEnd(p); break;
    case SCS_TELEMETRY_EVENT_paused:
        p.paused = true;
        p.metrics.paused = true;
        p.sounds.stopAll();
        break;
    case SCS_TELEMETRY_EVENT_started:
        p.paused = false;
        p.metrics.paused = false;
        p.sounds.resume(nowSeconds());
        break;
    default: break;
    }
}

bool registerTelemetry(const scs_telemetry_init_params_v100_t &params, Plugin &p) {
    const scs_u32_t flags = SCS_TELEMETRY_CHANNEL_FLAG_each_frame | SCS_TELEMETRY_CHANNEL_FLAG_no_value;
    return params.register_for_event(SCS_TELEMETRY_EVENT_frame_end, onEvent, nullptr) == SCS_RESULT_ok &&
           params.register_for_event(SCS_TELEMETRY_EVENT_paused, onEvent, nullptr) == SCS_RESULT_ok &&
           params.register_for_event(SCS_TELEMETRY_EVENT_started, onEvent, nullptr) == SCS_RESULT_ok &&
           params.register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_input_brake, SCS_U32_NIL, SCS_VALUE_TYPE_float,
                                       flags, onChannel, &p.pedal) == SCS_RESULT_ok &&
           params.register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_speed, SCS_U32_NIL, SCS_VALUE_TYPE_float, flags,
                                       onChannel, &p.speedMs) == SCS_RESULT_ok;
}

void destroyPlugin() {
    if (!g_plugin) return;
    Plugin *p = g_plugin;
    g_plugin = nullptr;
    if (p->bridge) p->bridge->stop();
    p->sounds.stopAll();
    p->audio.shutdown();
    const bool com = p->comInitialized;
    delete p;
    if (com) CoUninitialize();
}

} // namespace

} // namespace aa

extern "C" {

scs_result_t scs_telemetry_init(const scs_u32_t version,
                                                      const scs_telemetry_init_params_v100_t *params) {
    using namespace aa;
    // O jogo tenta de novo com uma versao mais antiga quando respondemos "unsupported".
    if (version != SCS_TELEMETRY_VERSION_1_00 && version != SCS_TELEMETRY_VERSION_1_01) return SCS_RESULT_unsupported;
    if (!params) return SCS_RESULT_invalid_parameter;
    if (g_plugin) return SCS_RESULT_generic_error;
    log::init(params->common.log);

    auto *p = new Plugin;
    g_plugin = p;
    const std::wstring directory = pluginDirectory();
    const std::wstring iniPath = directory + L"airbrake_audio.ini";
    p->config = loadConfig(iniPath);
    p->sounds.applyConfig(p->config);

    // Se o jogo ja iniciou o COM em outro modo, isto falha sem problema.
    p->comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    // Sem som (arquivo faltando, sem dispositivo) o plugin continua: a tela mostra o motivo.
    std::string soundError;
    const bool soundsLoaded = p->audio.init(p->config, directory + L"airbrake_audio\\", soundError);
    if (!soundsLoaded) log::error("Sem som: %s", soundError.c_str());

    if (!registerTelemetry(*params, *p)) {
        log::error("Falha ao registrar eventos/canais da telemetria.");
        destroyPlugin();
        return SCS_RESULT_generic_error;
    }

    p->bridge = std::make_unique<AppBridge>(p->config, iniPath, p->metrics, soundsLoaded, soundError);
    p->bridge->start();

    // So silencia quando o jogo avisar que pausou (comportamento validado no jogo desde a 0.1).
    p->paused = false;
    p->metrics.paused = false;
    log::info("v" AIRBRAKE_AUDIO_VERSION " pronto (ganho %.1f dB, freio=%ls, ar=%ls, atalho %s)", p->config.boostDb,
              p->config.brakeFile.c_str(), p->config.airFile.c_str(),
              p->config.appEnabled ? p->config.hotkey.c_str() : "desligado");
    return SCS_RESULT_ok;
}

void scs_telemetry_shutdown(void) {
    aa::destroyPlugin();
    aa::log::shutdown();
}

} // extern "C"
