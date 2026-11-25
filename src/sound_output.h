#pragma once
// O que a logica de freio (BrakeSounds) precisa do audio. AudioEngine implementa com XAudio2;
// os testes usam uma implementacao falsa que so registra as chamadas.

namespace aa {

class ISoundOutput {
public:
    virtual ~ISoundOutput() = default;

    // Freio: loop continuo que recomeca em loop_start_s.
    virtual void brakeStart() = 0;
    virtual void brakeSetVolume(float volume) = 0;
    virtual void brakeStop() = 0;

    // Saida de ar: toca uma vez; o corte cedo e feito por quem chama (volume/stop).
    virtual void airPlay(float volume) = 0;
    virtual void airSetVolume(float volume) = 0;
    virtual void airStop() = 0;
    virtual float airSeconds() const = 0;

    // Recalcula o ganho dos sons. Os dois sons precisam estar parados.
    virtual void setBoost(float gainDb) = 0;
};

} // namespace aa
