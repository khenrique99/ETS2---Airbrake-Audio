// Testes que rodam sem o jogo: WAV, configuracao, logica do freio, protocolo, atalho e modelo do app.
// Uso: ctest -C Release  (ou rodar aa_tests.exe direto)
#include "brake_sounds.h"
#include "command_inbox.h"
#include "config.h"
#include "json.h"
#include "model.h"
#include "settings_commands.h"
#include "shared/hotkey_combo.h"
#include "shared/ini_edit.h"
#include "shared/ipc.h"
#include "status_writer.h"
#include "texts.h"
#include "wav.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool cond, const char *what, int line) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("FALHOU (linha %d): %s\n", line, what);
    }
}
#define CHECK(c) check((c), #c, __LINE__)

bool approx(double a, double b, double eps = 1e-4) { return std::fabs(a - b) <= eps; }

// ---- saida de som falsa: registra as chamadas
struct FakeOutput final : aa::ISoundOutput {
    // Totais (qualquer nivel) e por nivel do freio.
    int brakeStarts = 0, brakeStops = 0, airPlays = 0, airStops = 0, boosts = 0;
    int starts[aa::kBrakeLevels] = {}, stops[aa::kBrakeLevels] = {};
    bool playing[aa::kBrakeLevels] = {};
    float volume[aa::kBrakeLevels] = {};
    float brakeVolume = 0, airVolume = 0, lastBoost = -1; // brakeVolume: ultimo volume definido
    float airLen = 2.0f;

    static int idx(aa::BrakeLevel l) { return static_cast<int>(l); }
    void brakeStart(aa::BrakeLevel l) override {
        ++brakeStarts;
        ++starts[idx(l)];
        playing[idx(l)] = true;
        volume[idx(l)] = 0;
    }
    void brakeSetVolume(aa::BrakeLevel l, float v) override {
        brakeVolume = v;
        volume[idx(l)] = v;
    }
    void brakeStop(aa::BrakeLevel l) override {
        ++brakeStops;
        ++stops[idx(l)];
        playing[idx(l)] = false;
        volume[idx(l)] = 0;
    }
    float vol(aa::BrakeLevel l) const { return playing[idx(l)] ? volume[idx(l)] : 0.0f; }
    void airPlay(float v) override {
        ++airPlays;
        airVolume = v;
    }
    void airSetVolume(float v) override { airVolume = v; }
    void airStop() override { ++airStops; }
    float airSeconds() const override { return airLen; }
    void setBoost(float db) override {
        ++boosts;
        lastBoost = db;
    }
};

// Anda o tempo em passos de 16 ms com o pedal/velocidade dados.
void run(aa::BrakeSounds &s, float pedal, float speed, double &now, double seconds) {
    for (double t = 0; t < seconds; t += 0.016) {
        now += 0.016;
        s.update(pedal, speed, now);
    }
}

void testWav() {
    // WAV valido: mono 16 bits, 8000 Hz, 4 amostras
    std::vector<uint8_t> w = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0,
                              1,   0,   1,   0,   0x40, 0x1F, 0, 0, 0x80, 0x3E, 0, 0, 2, 0, 16, 0,
                              'd', 'a', 't', 'a', 8, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0};
    aa::Wav wav;
    std::string err;
    CHECK(aa::parseWav(w, wav, err));
    CHECK(wav.frames == 4 && wav.format.nChannels == 1 && wav.format.nSamplesPerSec == 8000);
    CHECK(approx(wav.seconds(), 4.0 / 8000.0, 1e-9));

    std::vector<uint8_t> bad = w;
    bad[0] = 'X';
    CHECK(!aa::parseWav(bad, wav, err));                 // nao e RIFF
    CHECK(!aa::parseWav(std::vector<uint8_t>{1, 2, 3}, wav, err)); // pequeno demais
    bad = w;
    bad[34] = 8;                                         // 8 bits
    CHECK(!aa::parseWav(bad, wav, err));
    bad = w;
    bad.resize(44);                                      // fmt sem bloco data util
    CHECK(!aa::parseWav(bad, wav, err));

    // ganho: abaixo do joelho passa direto em 0 dB; com ganho alto nunca passa de 32767
    std::vector<int16_t> a = {1000, -1000, 32767, -32768};
    std::vector<int16_t> same = a;
    aa::applyBoost(same, 0.0f);
    CHECK(same == a);
    aa::applyBoost(a, 18.0f);
    for (int16_t v : a) CHECK(v <= 32767 && v >= -32767);
    CHECK(a[0] > 1000);
}

void testConfig() {
    const aa::Config d;
    CHECK(approx(d.boostDb, 6.0) && d.hotkey == "CTRL+PLUS" && d.appEnabled);

    aa::Config c = aa::configFromIni("");
    CHECK(approx(c.boostDb, 6.0)); // ausente -> padrao

    c = aa::configFromIni("[output]\nboost_db=99\n[brake]\nvolume_min=0.9\nvolume_max=0.2\n[trigger]\n"
                          "threshold_on=0.05\nthreshold_off=0.5\n");
    CHECK(approx(c.boostDb, 18.0));                  // limitado
    CHECK(c.volMin <= c.volMax);                   // trocados
    CHECK(c.thresholdOff < c.thresholdOn);         // histerese valida

    c = aa::configFromIni("[brake]\nfile=meu.wav ; comentario\nfade_in_ms=abc\n[ui]\nweb_ui=off\nhotkey=Minus\n");
    CHECK(c.brakeFile == L"meu.wav" && c.brakeLowFile == L"brake_loop_low.wav");
    CHECK(approx(c.brakeHighFrom, 0.65));
    c = aa::configFromIni("[brake]\nfile_low=baixo.wav\nhigh_from=0.8\n");
    CHECK(c.brakeLowFile == L"baixo.wav" && approx(c.brakeHighFrom, 0.8));
    c = aa::configFromIni("[brake]\nhigh_from=7\n");
    CHECK(approx(c.brakeHighFrom, 1.0)); // limitado
    c = aa::configFromIni("[brake]\nfile=meu.wav ; comentario\nfade_in_ms=abc\n[ui]\nweb_ui=off\nhotkey=Minus\n");
    CHECK(approx(c.fadeInMs, 60.0));                 // lixo -> padrao
    CHECK(!c.appEnabled);                          // nome da 1.x
    CHECK(c.hotkey == "Minus");                    // formato antigo ainda vale

    c = aa::configFromIni("[ui]\nhotkey=banana\n");
    CHECK(c.hotkey == "CTRL+PLUS");                // invalido -> padrao

    // painel: dentro do jogo por padrao; "external" (ou "externo") volta para o AirbrakeAudio.exe
    CHECK(d.inGamePanel && aa::configFromIni("").inGamePanel);
    CHECK(aa::configFromIni("[ui]\nmode=game\n").inGamePanel);
    CHECK(!aa::configFromIni("[ui]\nmode=external\n").inGamePanel);
    CHECK(!aa::configFromIni("[ui]\nmode = Externo\n").inGamePanel);
    CHECK(aa::configFromIni("[ui]\nmode=banana\n").inGamePanel); // lixo -> padrao

    // gravar preserva comentarios e muda so os valores
    const std::string ini = "; topo\n[brake]\n; nota\nvolume_max = 0.5   ; fim\nfile=a.wav\n";
    aa::Config edited;
    edited.volMax = 0.8f;
    const std::string out = aa::applyConfigToIni(ini, edited);
    CHECK(out.find("; topo") != std::string::npos && out.find("; nota") != std::string::npos);
    CHECK(out.find("volume_max = 0.8") != std::string::npos && out.find("; fim") != std::string::npos);
    CHECK(out.find("file=a.wav") != std::string::npos);

    // ida e volta
    aa::Config round = aa::configFromIni(aa::applyConfigToIni("", edited));
    CHECK(approx(round.volMax, 0.8));

    CHECK(aa::findField("brake.volume_max") != nullptr && aa::findField("x.y") == nullptr);
    float v = 0;
    CHECK(aa::parseNumber("1.5", v) && approx(v, 1.5));
    CHECK(!aa::parseNumber("1.5abc", v) && !aa::parseNumber("", v));
}

void testHotkeys() {
    using namespace aa::hotkey;
    Combo c;
    CHECK(parse("CTRL+PLUS", c) && c.ctrl && !c.shift && !c.alt && c.vk == VK_OEM_PLUS);
    CHECK(parse("ctrl + shift + f9", c) && c.ctrl && c.shift && c.vk == VK_F9);
    CHECK(parse("Minus", c) && c.vk == VK_OEM_MINUS);   // .ini da 1.x
    CHECK(parse("KeyP", c) && c.vk == 'P');
    CHECK(!parse("", c) && !parse("CTRL", c) && !parse("A+B", c) && !parse("CTRL+BANANA", c));
    CHECK(makeCombo(true, false, false, VK_OEM_PLUS) == "CTRL+PLUS");
    CHECK(makeCombo(false, false, false, VK_ESCAPE).empty());
    CHECK(pretty("CTRL+PLUS") == L"Ctrl + =");
    CHECK(pretty("ALT+NUMPAD3") == L"Alt + Num 3");
    Combo plain;
    parse("P", plain);
    CHECK(!safeForGame(plain));                          // sozinha dispararia dirigindo
    parse("CTRL+P", plain);
    CHECK(safeForGame(plain));
    parse("F9", plain);
    CHECK(safeForGame(plain));
}

void testBrakeSounds() {
    // Pedal -> loop liga, segue o volume e desliga com fade
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 0.0f, 50.0f, now, 0.2);
        CHECK(out.brakeStarts == 0);
        run(s, 1.0f, 50.0f, now, 0.5);
        CHECK(out.brakeStarts == 1);
        CHECK(approx(out.brakeVolume, cfg.volMax, 0.01));
        CHECK(s.state() == aa::SoundState::BrakeHigh); // pedal no fundo: temperatura alta
        run(s, 0.0f, 50.0f, now, 1.0); // solta
        CHECK(out.brakeStops >= 1);
        CHECK(s.state() == aa::SoundState::Air || s.state() == aa::SoundState::None);
    }
    // Dois loops: ate high_from (65%) toca o de temperatura baixa; acima, o de temperatura alta
    {
        using aa::BrakeLevel;
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 0.4f, 50.0f, now, 0.5);                 // pedal leve
        CHECK(out.starts[0] == 1 && out.starts[1] == 0);
        CHECK(s.state() == aa::SoundState::BrakeLow && s.level() == BrakeLevel::Low);
        CHECK(approx(out.vol(BrakeLevel::Low), cfg.volMin + (cfg.volMax - cfg.volMin) * 0.4f, 0.01));

        run(s, 0.64f, 50.0f, now, 0.5);                // ainda abaixo de 65%
        CHECK(out.starts[1] == 0 && s.level() == BrakeLevel::Low);

        run(s, 0.65f, 50.0f, now, 0.016);              // exatamente 65%: troca
        CHECK(out.starts[1] == 1 && s.level() == BrakeLevel::High);
        CHECK(out.playing[0]);                          // o baixo sai em fade, nao corta seco
        run(s, 0.8f, 50.0f, now, 0.5);
        CHECK(!out.playing[0] && out.stops[0] == 1);   // terminou o fade e parou
        CHECK(approx(out.vol(BrakeLevel::High), cfg.volMin + (cfg.volMax - cfg.volMin) * 0.8f, 0.01));
        CHECK(s.state() == aa::SoundState::BrakeHigh);

        run(s, 0.63f, 50.0f, now, 0.5);                // dentro da histerese: continua alto
        CHECK(s.level() == BrakeLevel::High && out.starts[0] == 1);
        run(s, 0.5f, 50.0f, now, 0.5);                 // desceu de verdade: volta ao baixo, do comeco
        CHECK(s.level() == BrakeLevel::Low && out.starts[0] == 2 && !out.playing[1]);
        CHECK(s.state() == aa::SoundState::BrakeLow);

        // vai e volta rapido: o loop que ainda esta saindo nao reinicia o ataque
        run(s, 0.9f, 50.0f, now, 0.05);
        run(s, 0.3f, 50.0f, now, 0.05);
        CHECK(out.starts[0] == 2 && out.starts[1] == 2);

        run(s, 0.0f, 50.0f, now, 1.0);                 // solta: os dois param
        CHECK(!out.playing[0] && !out.playing[1]);
    }
    // Pisar direto no fundo comeca no loop alto, sem tocar o baixo
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 0.9f, 50.0f, now, 0.3);
        CHECK(out.starts[0] == 0 && out.starts[1] == 1);
    }
    // high_from = 1: so o pedal no fundo toca o alto
    {
        FakeOutput out;
        aa::Config cfg;
        cfg.brakeHighFrom = 1.0f;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 0.95f, 50.0f, now, 0.3);
        CHECK(out.starts[1] == 0);
        run(s, 1.0f, 50.0f, now, 0.3);
        CHECK(out.starts[1] == 1);
    }
    // Abaixo da velocidade minima o freio fica mudo
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 1.0f, 1.0f, now, 1.0);
        CHECK(out.brakeStarts == 1 && out.brakeVolume < 0.01f);
    }
    // Saida de ar: freada curta toca mais baixo que longa; muito curta nao toca; cooldown segura
    // (sem a exigencia do loop alto, testada abaixo)
    {
        FakeOutput out;
        aa::Config cfg;
        cfg.airMinHighS = 0.0f;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 1.0f, 50.0f, now, 0.2);  // abaixo do minimo (400 ms)
        run(s, 0.0f, 50.0f, now, 0.1);
        CHECK(out.airPlays == 0);

        run(s, 1.0f, 50.0f, now, 1.0);  // curta
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 1);
        const float shortVolume = out.airVolume;
        run(s, 0.0f, 50.0f, now, 3.0);  // deixa terminar e o cooldown passar
        CHECK(s.state() == aa::SoundState::None);

        run(s, 1.0f, 50.0f, now, 7.0);  // longa (> full_brake_time_s)
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 2);
        CHECK(out.airVolume > shortVolume);
        CHECK(approx(out.airVolume, cfg.airVolume, 0.01));
        CHECK(s.heldSeconds(now) == 0.0f);

        // freada logo em seguida: cooldown
        run(s, 1.0f, 50.0f, now, 0.6);
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 2);
    }
    // Saida de ar so depois de 2 s acima de 65% (loop alto)
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 0.5f, 50.0f, now, 7.0);   // longa, mas so no loop baixo
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 0);
        run(s, 0.0f, 50.0f, now, 2.0);

        run(s, 0.9f, 50.0f, now, 1.5);   // alto por pouco tempo
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 0);
        run(s, 0.0f, 50.0f, now, 2.0);

        run(s, 0.5f, 50.0f, now, 3.0);   // baixo nao conta...
        run(s, 0.9f, 50.0f, now, 1.0);   // ...so o tempo no alto
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 0);
        run(s, 0.0f, 50.0f, now, 2.0);

        run(s, 0.9f, 50.0f, now, 2.1);   // passou de 2 s no alto
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 1);
        run(s, 0.0f, 50.0f, now, 3.0);

        run(s, 0.9f, 50.0f, now, 1.2);   // somado: 1,2 s + 1,2 s no alto na mesma freada
        run(s, 0.5f, 50.0f, now, 0.5);
        run(s, 0.9f, 50.0f, now, 1.2);
        run(s, 0.0f, 50.0f, now, 0.05);
        CHECK(out.airPlays == 2);
    }
    // Pausa silencia tudo
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        double now = 0;
        run(s, 1.0f, 50.0f, now, 0.5);
        s.stopAll();
        CHECK(out.brakeStops >= 1 && s.state() == aa::SoundState::None);
    }
    // Mudar o ganho silencia e recalcula; mudar outro ajuste nao
    {
        FakeOutput out;
        aa::Config cfg;
        aa::BrakeSounds s(out, cfg);
        aa::Config other = cfg;
        other.volMax = 0.7f;
        s.applyConfig(other);
        CHECK(out.boosts == 0);
        other.boostDb = 12.0f;
        s.applyConfig(other);
        CHECK(out.boosts == 1 && approx(out.lastBoost, 12.0) && out.brakeStops >= 1);
    }
}

void testCommands() {
    aa::Command c;
    CHECK(aa::parseCommandLine("17 definir brake.volume_max 0.8", c) && c.seq == 17 && c.action == "definir" &&
          c.args.size() == 2 && c.args[1] == "0.8");
    CHECK(aa::parseCommandLine("18 padrao", c) && c.args.empty());
    CHECK(aa::parseCommandLine("1732540000000 salvar", c) && c.seq == 1732540000000LL);
    CHECK(!aa::parseCommandLine("", c));
    CHECK(!aa::parseCommandLine("abc salvar", c));
    CHECK(!aa::parseCommandLine("0 salvar", c));
    CHECK(!aa::parseCommandLine("5 Salvar", c));                 // so minusculas
    CHECK(!aa::parseCommandLine("5 rm;rf", c));
    CHECK(!aa::parseCommandLine("5 definir a b c", c));          // argumentos demais
    CHECK(!aa::parseCommandLine("5 definir ../../x 1", c));      // sem '/'

    aa::Config cfg;
    aa::CommandOutcome o = aa::interpretCommand({1, "definir", {"brake.volume_max", "0.5"}}, cfg);
    CHECK(o.ok && o.configChanged && approx(cfg.volMax, 0.5));
    o = aa::interpretCommand({2, "definir", {"brake.volume_max", "9"}}, cfg);
    CHECK(o.ok && approx(cfg.volMax, 1.0));                         // limitado
    o = aa::interpretCommand({3, "definir", {"brake.volume_max", "x"}}, cfg);
    CHECK(!o.ok && !o.configChanged);
    o = aa::interpretCommand({4, "definir", {"nope", "1"}}, cfg);
    CHECK(!o.ok);
    o = aa::interpretCommand({5, "definir", {"brake.volume_max"}}, cfg);
    CHECK(!o.ok);
    cfg.volMax = 0.3f;
    o = aa::interpretCommand({6, "padrao", {}}, cfg);
    CHECK(o.ok && o.configChanged && approx(cfg.volMax, 1.0));
    o = aa::interpretCommand({7, "salvar", {}}, cfg);
    CHECK(o.ok && o.saveRequested && !o.configChanged);
    o = aa::interpretCommand({8, "recarregar_atalho", {}}, cfg);
    CHECK(o.ok && o.reloadHotkey);
    o = aa::interpretCommand({9, "formatar", {}}, cfg);
    CHECK(!o.ok);
}

void testJsonAndModel() {
    using aa::json::Value;
    Value v;
    const std::string doc = R"({"a": 1.5, "b": [true, null], "c": {"d": "x\"y"}})";
    const bool parsed = aa::json::parse(doc, v);
    CHECK(parsed);
    CHECK(approx(v["a"].asNumber(), 1.5) && v["b"].size() == 2 && v["c"]["d"].asString() == "x\"y");
    CHECK(v["nao"]["existe"].isNull());
    CHECK(!aa::json::parse("{\"a\": ", v));

    // status.json gerado pelo plugin -> estado do app
    aa::Config cfg;
    cfg.volMax = 0.7f;
    aa::StatusInfo info;
    info.config = &cfg;
    info.pedal = 0.5f;
    info.speedKmh = 42.0f;
    info.sound = aa::SoundState::BrakeLow;
    info.unsaved = true;
    info.lastCommandSeq = 9;
    info.lastCommandResult = "ok \"aspas\"";
    const unsigned long long now = 1'700'000'000'000ULL;
    const std::string json = aa::statusJson(info, now);
    CHECK(aa::json::parse(json, v));

    aa::app::AppState s;
    CHECK(aa::app::stateFromJson(v, now, s));
    CHECK(s.connected && s.fileFound && s.unsaved);
    CHECK(approx(s.pedal, 0.5) && approx(s.speedKmh, 42.0) && s.sound == "freio");
    CHECK(s.lastCommandSeq == 9 && s.lastCommandResult == L"ok \"aspas\"");
    const aa::app::FieldView *f = s.field("brake.volume_max");
    CHECK(f && approx(f->value, 0.7) && approx(f->def, 1.0) && approx(f->max, 1.0));
    CHECK(s.field("nao.existe") == nullptr);

    // todo ajuste do plugin tem rotulo na tela (e vice-versa)
    size_t count = 0;
    const aa::FloatField *fields = aa::floatFields(count);
    size_t live = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!fields[i].live) continue;
        ++live;
        CHECK(aa::app::findMeta(fields[i].id()) != nullptr);
    }
    size_t metas = 0;
    aa::app::fieldMetas(metas);
    CHECK(metas == live);
    CHECK(s.fields.size() == live);
    aa::Config loopCfg;
    const aa::CommandOutcome o = aa::interpretCommand({1, "definir", {"brake.loop_start_s", "1"}}, loopCfg);
    CHECK(!o.ok && !o.configChanged); // so vale ao reiniciar

    // batimento
    CHECK(aa::app::heartbeatFresh(now, now + 1000));
    CHECK(!aa::app::heartbeatFresh(now, now + 10'000));
    CHECK(aa::app::heartbeatFresh(now + 1500, now));        // relogio adiantado
    CHECK(!aa::app::heartbeatFresh(now + 5000, now));
    CHECK(!aa::app::heartbeatFresh(0, now));

    // o modo demonstracao responde como o plugin
    aa::app::DemoSource demo;
    aa::app::AppState d;
    demo.poll(d);
    CHECK(d.connected && d.fields.size() == live);
    const long long seq = demo.send("definir brake.volume_max 0.4");
    demo.poll(d);
    CHECK(d.lastCommandSeq == seq && d.unsaved && approx(d.field("brake.volume_max")->value, 0.4));
    demo.send("padrao");
    demo.poll(d);
    CHECK(approx(d.field("brake.volume_max")->value, 1.0));
    demo.send("salvar");
    demo.poll(d);
    CHECK(!d.unsaved);
}

void testIni() {
    const std::string ini = "[a]\nx = 1 ; c\n";
    CHECK(aa::ini::get(ini, "a", "x") == "1");
    CHECK(aa::ini::apply(ini, {{"a", "x", "2"}}).find("x = 2") != std::string::npos);
    CHECK(aa::ini::apply(ini, {{"b", "y", "3"}}).find("[b]") != std::string::npos);
    CHECK(aa::ini::sanitize("a;b\n#c") == "abc");
}

} // namespace

int main() {
    testWav();
    testConfig();
    testHotkeys();
    testBrakeSounds();
    testCommands();
    testJsonAndModel();
    testIni();
    std::printf("%d verificacoes, %d falhas\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
