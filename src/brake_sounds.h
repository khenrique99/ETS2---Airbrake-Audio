#pragma once
// Logica dos sons: liga o loop do freio quando o pedal passa de threshold_on, segue o pedal com
// fades e, ao soltar, toca a saida de ar proporcional ao tempo de freada. Ate brake.high_from
// toca o loop "temperatura baixa"; acima dele, o "temperatura alta" (com cruzamento pelos fades).
// A saida de ar so toca se a freada ficou pelo menos air_release.min_high_time_s no loop alto.
// Sem Windows nem SDK: recebe o tempo de fora e fala com ISoundOutput, entao roda nos testes.
#include "config.h"
#include "sound_output.h"

namespace aa {

enum class SoundState { None = 0, BrakeLow = 1, Air = 2, BrakeHigh = 3 };

class BrakeSounds {
public:
    // Para voltar ao loop baixo o pedal precisa cair um pouco abaixo de high_from: um pedal
    // analogico tremendo em cima do limite nao fica trocando de som.
    static constexpr float kHighHysteresis = 0.03f;

    BrakeSounds(ISoundOutput &out, const Config &config) : out_(out), config_(config) {}

    // Uma vez por frame. pedal 0..1, speedKmh >= 0, now em segundos (monotonico).
    void update(float pedal, float speedKmh, double now);

    // Jogo pausou: silencia tudo e esquece a freada em andamento.
    void stopAll();
    // Jogo voltou: evita um dt gigante no primeiro frame.
    void resume(double now) { lastTick_ = now; }

    // Troca os ajustes. Mudar o ganho recalcula os sons, entao silencia antes.
    void applyConfig(const Config &config);
    const Config &config() const { return config_; }

    // Para a tela.
    float heldSeconds(double now) const { return braking_ ? static_cast<float>(now - pressedAt_) : 0.0f; }
    SoundState state() const;
    BrakeLevel level() const { return level_; }

private:
    struct Voice {
        bool running = false;
        float volume = 0.0f;
    };

    void updateTrigger(float pedal, double now);
    void updateLevel(float pedal);
    void startVoice(BrakeLevel level);
    void updateBrakeVolumes(float pedal, float speedKmh, float dt);
    void updateVoice(BrakeLevel level, float target, float dt);
    void updateAirRelease(double now);
    void playAirRelease(double heldMs, double now);

    Voice &voice(BrakeLevel level) { return voices_[static_cast<int>(level)]; }
    const Voice &voice(BrakeLevel level) const { return voices_[static_cast<int>(level)]; }

    ISoundOutput &out_;
    Config config_;

    double lastTick_ = 0.0;
    bool started_ = false;

    bool braking_ = false;
    BrakeLevel level_ = BrakeLevel::Low; // loop que segue o pedal; o outro (se tocando) sai em fade
    Voice voices_[kBrakeLevels];
    double pressedAt_ = 0.0;
    float highHeldS_ = 0.0f; // tempo no loop alto nesta freada (decide se toca a saida de ar)
    double lastAirAt_ = -1e9;

    bool airPlaying_ = false;
    bool airIsCut_ = false;   // fecha antes do fim
    double airStartedAt_ = 0.0;
    double airCutAt_ = 0.0;   // segundos apos o inicio em que o corte termina
    double airEndAt_ = 0.0;   // segundos apos o inicio em que o som inteiro termina
    float airVolume_ = 0.0f;
};

} // namespace aa
