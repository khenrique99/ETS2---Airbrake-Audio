#pragma once
// Estado mostrado pelo app e as duas fontes possiveis: o plugin (status.json/command.txt)
// ou o modo demonstracao (simulado, sem jogo).
#include "json.h"

#include <string>
#include <vector>

namespace aa::app {

struct FieldView {
    std::string id; // "brake.volume_max"
    double value = 0, min = 0, max = 1, def = 0;
};

struct AppState {
    bool fileFound = false; // existe status.json
    bool connected = false; // batimento recente
    bool paused = false;
    bool soundsLoaded = true;
    std::wstring soundError, pluginVersion, configPath, appHotkey;
    bool unsaved = false;

    double pedal = 0, speedKmh = 0, heldS = 0;
    std::string sound; // "nenhum", "freio" (temperatura baixa), "freio_alto", "ar"

    long long lastCommandSeq = 0;
    bool lastCommandOk = true;
    std::wstring lastCommandResult;

    std::vector<FieldView> fields;

    const FieldView *field(const std::string &id) const;
    FieldView *field(const std::string &id);
};

// Batimento do plugin ainda valido? (tolera relogio adiantado ate 2 s)
bool heartbeatFresh(unsigned long long ts, unsigned long long nowMs);

// Converte status.json em AppState. nowMs = relogio Unix em ms (para o batimento).
bool stateFromJson(const json::Value &v, unsigned long long nowMs, AppState &out);

class Source {
public:
    virtual ~Source() = default;
    // Atualiza o estado; true se algo mudou.
    virtual bool poll(AppState &state) = 0;
    // Envia um comando ("definir brake.volume_max 0.8"); devolve o seq usado (0 = falhou).
    virtual long long send(const std::string &action) = 0;
    virtual bool isDemo() const = 0;
};

class PluginSource final : public Source {
public:
    bool poll(AppState &state) override;
    long long send(const std::string &action) override;
    bool isDemo() const override { return false; }

private:
    std::string lastText_;
    bool lastConnected_ = false;
    long long lastSeq_ = 0;
};

// Simula pedadas no freio e responde aos comandos como o plugin.
class DemoSource final : public Source {
public:
    DemoSource();
    bool poll(AppState &state) override;
    long long send(const std::string &action) override;
    bool isDemo() const override { return true; }

private:
    void apply(const std::string &action, long long seq);

    AppState sim_;
    std::vector<FieldView> saved_;
    long long seq_ = 0;
    unsigned long long startedAt_;
};

} // namespace aa::app
