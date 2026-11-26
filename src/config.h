#pragma once
// airbrake_audio.ini (ao lado da DLL). Valor ausente ou invalido -> padrao; tudo limitado a
// faixas seguras. Cada ajuste numerico e descrito uma vez em floatFields(): a mesma tabela
// le o .ini, vai para o status.json (a tela monta os controles com ela) e grava de volta.
#include <cstddef>
#include <string>

namespace aa {

struct Config {
    // [output]
    float boostDb = 6.0f; // ganho aplicado ao carregar os sons (com limitador suave)

    // [brake]
    std::wstring brakeFile = L"brake_loop.wav";        // "temperatura alta": pedal a partir de brakeHighFrom
    std::wstring brakeLowFile = L"brake_loop_low.wav"; // "temperatura baixa": pedal abaixo de brakeHighFrom
    float loopStartS = 2.0f;
    float loopStartLowS = 2.0f;
    float brakeHighFrom = 0.65f; // pedal (0..1) a partir do qual toca o loop alto
    float fadeInMs = 60.0f;
    float fadeOutMs = 200.0f;
    float volMin = 0.35f; // volume com o pedal leve
    float volMax = 1.0f;  // volume com o pedal no fundo
    float minSpeedKmh = 3.0f;

    // [air_release]
    std::wstring airFile = L"air_release.wav";
    float airVolume = 0.9f;
    float airFullS = 6.0f;     // freada com essa duracao toca o som inteiro, volume cheio
    float airShortVol = 0.6f;  // multiplicador de volume na freada curta
    float airShortLenS = 0.45f;
    float airAudibleS = 1.1f;  // ate onde o corte pode chegar antes de liberar o som inteiro
    float airCutFadeMs = 120.0f;
    float airMinHoldMs = 400.0f;
    float airCooldownMs = 1500.0f;
    float airMinHighS = 2.0f;  // so toca se a freada ficou isso no loop alto (0 = sempre)

    // [trigger]
    float thresholdOn = 0.05f;
    float thresholdOff = 0.02f;

    // [ui]
    bool appEnabled = true;           // atalho que abre o AirbrakeAudio.exe
    std::string hotkey = "CTRL+PLUS"; // formato de shared/hotkey_combo.h
};

struct FloatField {
    const char *section;
    const char *key;
    float Config::*member;
    float low;
    float high;
    bool live = true; // false: so vale ao reiniciar o jogo (editar no .ini)

    std::string id() const { return std::string(section) + "." + key; } // "brake.volume_max"
};

const FloatField *floatFields(size_t &count);
const FloatField *findField(const std::string &id);

// Mantem a histerese valida e as faixas coerentes. Usado depois de toda mudanca.
void sanitize(Config &c);

// Conversao com ponto decimal, independente do locale do jogo.
std::string formatNumber(float v);
bool parseNumber(const std::string &text, float &out);

// Texto do .ini <-> Config (sem arquivo: testavel).
Config configFromIni(const std::string &iniText);
std::string applyConfigToIni(const std::string &iniText, const Config &c); // preserva comentarios

Config loadConfig(const std::wstring &iniPath);
bool saveConfig(const std::wstring &iniPath, const Config &c);

// Diretorio (com barra no fim) onde a DLL esta: bin\win_x64\plugins\.
std::wstring pluginDirectory();

} // namespace aa
