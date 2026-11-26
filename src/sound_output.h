#pragma once
// O que a logica de freio (BrakeSounds) precisa do audio. AudioEngine implementa com XAudio2;
// os testes usam uma implementacao falsa que so registra as chamadas.

namespace aa {

// Qual dos dois loops do freio: Low ("temperatura baixa", pedal leve) ou High ("temperatura
// alta", pedal acima de brake.high_from). Os dois tem voz propria para poderem se cruzar.
enum class BrakeLevel { Low = 0, High = 1 };
constexpr int kBrakeLevels = 2;

class ISoundOutput {
public:
    virtual ~ISoundOutput() = default;

    // Freio: loop continuo que recomeca no loop_start do nivel.
    virtual void brakeStart(BrakeLevel level) = 0;
    virtual void brakeSetVolume(BrakeLevel level, float volume) = 0;
    virtual void brakeStop(BrakeLevel level) = 0;

    // Saida de ar: toca uma vez; o corte cedo e feito por quem chama (volume/stop).
    virtual void airPlay(float volume) = 0;
    virtual void airSetVolume(float volume) = 0;
    virtual void airStop() = 0;
    virtual float airSeconds() const = 0;

    // Recalcula o ganho dos sons. Todos os sons precisam estar parados.
    virtual void setBoost(float gainDb) = 0;
};

} // namespace aa
