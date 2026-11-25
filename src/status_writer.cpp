#include "status_writer.h"

#include "shared/ipc.h"
#include "version.h"

#include <sstream>

namespace aa {

namespace {

std::string esc(const std::string &s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"': o += "\\\""; break;
        case '\\': o += "\\\\"; break;
        case '\n': o += "\\n"; break;
        case '\t': o += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) >= 0x20) o += c;
        }
    }
    return o;
}

const char *b(bool v) { return v ? "true" : "false"; }

const char *soundName(SoundState s) {
    switch (s) {
    case SoundState::Brake: return "freio";
    case SoundState::Air: return "ar";
    default: return "nenhum";
    }
}

} // namespace

std::string statusJson(const StatusInfo &info, unsigned long long timestampMs) {
    std::ostringstream j;
    j << "{\n";
    j << "  \"protocol\": " << ipc::kProtocolVersion << ",\n";
    j << "  \"plugin_version\": \"" AIRBRAKE_AUDIO_VERSION "\",\n";
    j << "  \"timestamp_ms\": " << timestampMs << ",\n";
    j << "  \"paused\": " << b(info.paused) << ",\n";
    j << "  \"sounds_loaded\": " << b(info.soundsLoaded) << ",\n";
    j << "  \"sound_error\": \"" << esc(info.soundError) << "\",\n";
    j << "  \"config_path\": \"" << esc(info.configPath) << "\",\n";
    j << "  \"app_hotkey\": \"" << esc(info.config ? info.config->hotkey : std::string()) << "\",\n";
    j << "  \"unsaved\": " << b(info.unsaved) << ",\n";
    j << "  \"metrics\": {\"pedal\": " << formatNumber(info.pedal) << ", \"speed_kmh\": " << formatNumber(info.speedKmh)
      << ", \"held_s\": " << formatNumber(info.heldS) << ", \"sound\": \"" << soundName(info.sound) << "\"},\n";
    j << "  \"last_command\": {\"seq\": " << info.lastCommandSeq << ", \"ok\": " << b(info.lastCommandOk)
      << ", \"result\": \"" << esc(info.lastCommandResult) << "\"},\n";
    j << "  \"fields\": [\n";
    size_t count = 0;
    const FloatField *fields = floatFields(count);
    const Config defaults;
    bool first = true;
    for (size_t i = 0; i < count; ++i) {
        const FloatField &f = fields[i];
        if (!f.live) continue; // a tela so mostra o que vale na hora
        if (!first) j << ",\n";
        first = false;
        const float value = info.config ? info.config->*(f.member) : defaults.*(f.member);
        j << "    {\"id\": \"" << f.id() << "\", \"value\": " << formatNumber(value)
          << ", \"min\": " << formatNumber(f.low) << ", \"max\": " << formatNumber(f.high)
          << ", \"default\": " << formatNumber(defaults.*(f.member)) << "}";
    }
    j << "\n  ]\n}\n";
    return j.str();
}

void writeStatus(const StatusInfo &info) {
    static const std::wstring path = ipc::statusPath();
    if (!path.empty()) ipc::writeAtomic(path, statusJson(info, ipc::nowUnixMs()));
}

} // namespace aa
