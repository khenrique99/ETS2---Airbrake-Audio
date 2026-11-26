#include "model.h"

#include "shared/ipc.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace aa::app {

const FieldView *AppState::field(const std::string &id) const {
    for (const FieldView &f : fields)
        if (f.id == id) return &f;
    return nullptr;
}

FieldView *AppState::field(const std::string &id) {
    return const_cast<FieldView *>(static_cast<const AppState *>(this)->field(id));
}

bool heartbeatFresh(unsigned long long ts, unsigned long long nowMs) {
    if (ts == 0) return false;
    if (ts > nowMs) return ts - nowMs <= 2000; // relogio do plugin um pouco adiantado
    return nowMs - ts <= ipc::kStaleMs;
}

bool stateFromJson(const json::Value &v, unsigned long long nowMs, AppState &s) {
    if (v.type != json::Value::Type::Object) return false;
    s = AppState{};
    s.fileFound = true;
    s.connected = heartbeatFresh(static_cast<unsigned long long>(v["timestamp_ms"].asNumber(0)), nowMs);
    s.paused = v["paused"].asBool(false);
    s.soundsLoaded = v["sounds_loaded"].asBool(true);
    s.soundError = ipc::widen(v["sound_error"].asString());
    s.pluginVersion = ipc::widen(v["plugin_version"].asString());
    s.configPath = ipc::widen(v["config_path"].asString());
    s.appHotkey = ipc::widen(v["app_hotkey"].asString());
    s.unsaved = v["unsaved"].asBool(false);

    const json::Value &m = v["metrics"];
    s.pedal = m["pedal"].asNumber(0);
    s.speedKmh = m["speed_kmh"].asNumber(0);
    s.heldS = m["held_s"].asNumber(0);
    s.sound = m["sound"].asString();

    s.lastCommandSeq = v["last_command"]["seq"].asInt(0);
    s.lastCommandOk = v["last_command"]["ok"].asBool(true);
    s.lastCommandResult = ipc::widen(v["last_command"]["result"].asString());

    const json::Value &fields = v["fields"];
    for (size_t i = 0; i < fields.size(); ++i) {
        FieldView f;
        f.id = fields[i]["id"].asString();
        f.value = fields[i]["value"].asNumber(0);
        f.min = fields[i]["min"].asNumber(0);
        f.max = fields[i]["max"].asNumber(1);
        f.def = fields[i]["default"].asNumber(f.value);
        if (!f.id.empty() && f.max > f.min) s.fields.push_back(f);
    }
    return true;
}

// ------------------------------------------------------------------ plugin

bool PluginSource::poll(AppState &state) {
    static const std::wstring path = ipc::statusPath();
    std::string text;
    if (path.empty() || !ipc::readAll(path, text)) {
        const bool changed = state.fileFound || state.connected;
        state = AppState{};
        lastText_.clear();
        return changed;
    }
    const unsigned long long now = ipc::nowUnixMs();
    json::Value v;
    if (text == lastText_) {
        // Mesmo conteudo: so o batimento pode ter vencido.
        if (!json::parse(text, v)) return false;
        const bool connected = heartbeatFresh(static_cast<unsigned long long>(v["timestamp_ms"].asNumber(0)), now);
        if (connected == lastConnected_) return false;
        lastConnected_ = connected;
        state.connected = connected;
        return true;
    }
    if (!json::parse(text, v)) return false; // arquivo pela metade: tenta de novo
    AppState s;
    if (!stateFromJson(v, now, s)) return false;
    lastText_ = text;
    lastConnected_ = s.connected;
    state = std::move(s);
    return true;
}

long long PluginSource::send(const std::string &action) {
    static const std::wstring path = ipc::commandPath();
    if (path.empty()) return 0;
    long long seq = static_cast<long long>(ipc::nowUnixMs());
    if (seq <= lastSeq_) seq = lastSeq_ + 1;
    lastSeq_ = seq;

    // Se o plugin ainda nao pegou o anterior, acrescenta.
    std::string text;
    ipc::readAll(path, text);
    if (!text.empty() && text.back() != '\n') text += '\n';
    text += std::to_string(seq) + " " + action + "\n";
    return ipc::writeAtomic(path, text) ? seq : 0;
}

// ------------------------------------------------------------------ demonstracao

namespace {

// Mesmos ids, faixas e padroes do plugin (config.cpp).
const FieldView kDemoFields[] = {
    {"output.boost_db", 6, 0, 18, 6},
    {"brake.volume_min", 0.35, 0, 1, 0.35},
    {"brake.volume_max", 1, 0, 1, 1},
    {"brake.high_from", 0.65, 0.05, 1, 0.65},
    {"brake.min_speed_kmh", 3, 0, 200, 3},
    {"brake.fade_in_ms", 60, 0, 5000, 60},
    {"brake.fade_out_ms", 200, 0, 5000, 200},
    {"air_release.volume", 0.9, 0, 1, 0.9},
    {"air_release.full_brake_time_s", 6, 0.5, 120, 6},
    {"air_release.short_volume", 0.6, 0, 1, 0.6},
    {"air_release.short_length_s", 0.45, 0.05, 10, 0.45},
    {"air_release.audible_length_s", 1.1, 0.1, 10, 1.1},
    {"air_release.cut_fade_ms", 120, 5, 2000, 120},
    {"air_release.min_brake_time_ms", 400, 0, 10000, 400},
    {"air_release.cooldown_ms", 1500, 0, 60000, 1500},
    {"air_release.min_high_time_s", 2, 0, 60, 2},
    {"trigger.threshold_on", 0.05, 0.01, 0.9, 0.05},
    {"trigger.threshold_off", 0.02, 0, 0.9, 0.02},
};

} // namespace

DemoSource::DemoSource() : startedAt_(GetTickCount64()) {
    sim_.fileFound = true;
    sim_.connected = true;
    sim_.pluginVersion = L"demo";
    sim_.appHotkey = L"CTRL+PLUS";
    sim_.fields.assign(std::begin(kDemoFields), std::end(kDemoFields));
    saved_ = sim_.fields;
}

long long DemoSource::send(const std::string &action) {
    apply(action, ++seq_);
    return seq_;
}

void DemoSource::apply(const std::string &action, long long seq) {
    std::istringstream in(action);
    std::string verb, id, value;
    in >> verb >> id >> value;
    bool ok = true;
    std::wstring msg;
    if (verb == "definir") {
        FieldView *f = sim_.field(id);
        const double v = std::strtod(value.c_str(), nullptr);
        if (f) {
            f->value = std::min(f->max, std::max(f->min, v));
            sim_.unsaved = true;
            msg = ipc::widen(id + " = " + value);
        } else {
            ok = false;
            msg = ipc::widen("Ajuste desconhecido: " + id);
        }
    } else if (verb == "padrao") {
        for (FieldView &f : sim_.fields) f.value = f.def;
        sim_.unsaved = true;
        msg = L"Valores padrao aplicados. Salve para manter.";
    } else if (verb == "salvar") {
        saved_ = sim_.fields;
        sim_.unsaved = false;
        msg = L"Ajustes salvos (demonstração: nada foi gravado).";
    } else {
        msg = ipc::widen(verb);
    }
    sim_.lastCommandSeq = seq;
    sim_.lastCommandOk = ok;
    sim_.lastCommandResult = msg;
}

bool DemoSource::poll(AppState &state) {
    // Ciclo de 6 s: 2,5 s freando forte, solta, anda.
    const double t = static_cast<double>(GetTickCount64() - startedAt_) / 1000.0;
    const double phase = std::fmod(t, 6.0);
    const bool braking = phase < 2.5;
    sim_.pedal = braking ? 0.55 + 0.35 * std::sin(phase * 2.0) : 0.0;
    sim_.speedKmh = braking ? std::max(0.0, 62.0 - phase * 12.0) : 32.0 + (phase - 2.5) * 8.0;
    sim_.heldS = braking ? phase : 0.0;
    const FieldView *highFrom = sim_.field("brake.high_from");
    const bool high = braking && highFrom && sim_.pedal >= highFrom->value;
    sim_.sound = braking ? (high ? "freio_alto" : "freio") : (phase < 3.6 ? "ar" : "nenhum");
    state = sim_;
    return true;
}

} // namespace aa::app
