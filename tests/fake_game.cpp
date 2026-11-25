// "Jogo falso": carrega a DLL do plugin pelo SDK de telemetria, simula pedal/velocidade e
// conversa com o plugin como o app faria (command.txt / status.json). Roda sem o ETS2.
// Uso: fake_game <caminho da airbrake_audio.dll> --selftest
#include "json.h"
#include "scs_sdk_min.h"
#include "shared/ipc.h"

#include <windows.h>

#include <cstdio>
#include <map>
#include <string>

namespace {

int g_failures = 0;

void check(bool cond, const char *what, int line) {
    if (!cond) {
        ++g_failures;
        std::printf("FALHOU (linha %d): %s\n", line, what);
    }
}
#define CHECK(c) check((c), #c, __LINE__)

// ---- o "jogo": guarda o que o plugin registrou
std::map<scs_event_t, scs_telemetry_event_callback_t> g_events;
struct ChannelReg {
    scs_telemetry_channel_callback_t callback;
    scs_context_t context;
};
std::map<std::string, ChannelReg> g_channels;
std::string g_log;

void gameLog(const scs_log_type_t, const scs_string_t message) {
    g_log += message;
    g_log += '\n';
}
scs_result_t registerEvent(const scs_event_t e, const scs_telemetry_event_callback_t cb, const scs_context_t) {
    g_events[e] = cb;
    return SCS_RESULT_ok;
}
scs_result_t unregisterEvent(const scs_event_t) { return SCS_RESULT_ok; }
scs_result_t registerChannel(const scs_string_t name, const scs_u32_t, const scs_value_type_t,
                             const scs_u32_t, const scs_telemetry_channel_callback_t cb, const scs_context_t ctx) {
    g_channels[name] = {cb, ctx};
    return SCS_RESULT_ok;
}
scs_result_t unregisterChannel(const scs_string_t, const scs_u32_t, const scs_value_type_t) { return SCS_RESULT_ok; }

void setChannel(const char *name, float value) {
    const ChannelReg &c = g_channels[name];
    scs_value_t v{};
    v.type = SCS_VALUE_TYPE_float;
    v.value_float = value;
    c.callback(name, SCS_U32_NIL, &v, c.context);
}

void fire(scs_event_t e) {
    if (g_events.count(e)) g_events[e](e, nullptr, nullptr);
}

void frames(int n) {
    for (int i = 0; i < n; ++i) {
        fire(SCS_TELEMETRY_EVENT_frame_end);
        Sleep(16);
    }
}

// ---- conversa com o plugin
bool readStatus(aa::json::Value &out) {
    std::string text;
    return aa::ipc::readAll(aa::ipc::statusPath(), text) && aa::json::parse(text, out);
}

long long g_seq = 0;
void sendCommand(const std::string &action) {
    std::string text;
    aa::ipc::readAll(aa::ipc::commandPath(), text);
    text += std::to_string(++g_seq) + " " + action + "\n";
    aa::ipc::writeAtomic(aa::ipc::commandPath(), text);
}

// Espera o plugin responder o comando (ele publica a cada 50 ms quando ha comando).
bool waitReply(long long seq, aa::json::Value &status) {
    for (int i = 0; i < 100; ++i) {
        Sleep(50);
        frames(1); // o jogo consome as configuracoes novas no frame_end
        if (readStatus(status) && status["last_command"]["seq"].asInt(0) >= seq) return true;
    }
    return false;
}

const aa::json::Value *findField(const aa::json::Value &status, const std::string &id) {
    static aa::json::Value holder;
    const aa::json::Value &fields = status["fields"];
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i]["id"].asString() == id) {
            holder = fields[i];
            return &holder;
        }
    }
    return nullptr;
}

std::wstring makeTempDir() {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    const std::wstring dir = std::wstring(tmp) + L"aa_selftest_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

void selftest(HMODULE dll) {
    using InitFn = scs_result_t (*)(scs_u32_t, const scs_telemetry_init_params_v100_t *);
    using ShutdownFn = void (*)();
    auto init = reinterpret_cast<InitFn>(GetProcAddress(dll, "scs_telemetry_init"));
    auto shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(dll, "scs_telemetry_shutdown"));
    CHECK(init && shutdown);
    if (!init || !shutdown) return;

    scs_telemetry_init_params_v100_t params{};
    params.common.game_name = "Fake";
    params.common.game_id = "fake";
    params.common.log = gameLog;
    params.register_for_event = registerEvent;
    params.unregister_from_event = unregisterEvent;
    params.register_for_channel = registerChannel;
    params.unregister_from_channel = unregisterChannel;

    CHECK(init(SCS_MAKE_VERSION(9, 9), &params) == SCS_RESULT_unsupported); // versao nao suportada
    CHECK(init(SCS_TELEMETRY_VERSION_1_01, &params) == SCS_RESULT_ok);
    CHECK(g_events.count(SCS_TELEMETRY_EVENT_frame_end) == 1);
    CHECK(g_channels.count("truck.input.brake") == 1 && g_channels.count("truck.speed") == 1);

    // O plugin carrega mesmo sem placa de som (CI): so mostra o motivo na tela.
    fire(SCS_TELEMETRY_EVENT_started);
    setChannel("truck.speed", 20.0f);
    setChannel("truck.input.brake", 0.8f);
    frames(5);
    setChannel("truck.input.brake", 0.0f);
    frames(5);

    aa::json::Value status;
    bool published = false;
    for (int i = 0; i < 60 && !published; ++i) {
        Sleep(50);
        published = readStatus(status);
    }
    CHECK(published);
    CHECK(status["protocol"].asInt(0) == aa::ipc::kProtocolVersion);
    CHECK(!status["plugin_version"].asString().empty());
    CHECK(status["timestamp_ms"].asNumber(0) > 0);
    CHECK(status["fields"].size() > 10);

    // definir -> o plugin confirma e o valor volta no status
    sendCommand("definir brake.volume_max 0.5");
    CHECK(waitReply(g_seq, status));
    CHECK(status["last_command"]["ok"].asBool(false));
    const aa::json::Value *f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) > 0.49 && f->operator[]("value").asNumber(0) < 0.51);
    CHECK(status["unsaved"].asBool(false));

    // valor fora da faixa e limitado
    sendCommand("definir brake.volume_max 7");
    CHECK(waitReply(g_seq, status));
    f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) <= 1.0);

    // erros: ajuste desconhecido, valor invalido, acao desconhecida
    sendCommand("definir nao.existe 1");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));
    sendCommand("definir brake.volume_max abc");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));
    sendCommand("formatar");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));

    // padrao
    sendCommand("padrao");
    CHECK(waitReply(g_seq, status) && status["last_command"]["ok"].asBool(false));
    f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) > 0.99);

    // jogo pausa e volta
    fire(SCS_TELEMETRY_EVENT_paused);
    fire(SCS_TELEMETRY_EVENT_started);
    frames(3);

    shutdown();
    CHECK(g_log.find("[AirbrakeAudio]") != std::string::npos);
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || std::string(argv[2]) != "--selftest") {
        std::printf("Uso: fake_game <airbrake_audio.dll> --selftest\n");
        return 2;
    }
    const std::wstring dir = makeTempDir();
    SetEnvironmentVariableW(aa::ipc::kFolderOverrideEnv, dir.c_str()); // nao toca em Documentos

    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) {
        std::printf("Nao consegui carregar %s (erro %lu)\n", argv[1], GetLastError());
        return 1;
    }
    selftest(dll);
    FreeLibrary(dll);

    std::printf("%s\n", g_failures == 0 ? "plugin_selftest: OK" : "plugin_selftest: FALHOU");
    if (g_failures) std::printf("--- log do jogo ---\n%s", g_log.c_str());
    return g_failures == 0 ? 0 : 1;
}
