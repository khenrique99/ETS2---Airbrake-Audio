#include "texts.h"

namespace aa::app {

namespace {

struct Pair {
    const wchar_t *pt;
    const wchar_t *en;
};

// Na ordem de Txt.
const Pair kTexts[] = {
    {L"Voltar ao jogo:  Esc  ou  ", L"Back to the game:  Esc  or  "},
    {L"Demonstração", L"Demo"},
    {L"Aguardando o jogo", L"Waiting for the game"},
    {L"Jogo pausado", L"Game paused"},
    {L"Jogo conectado", L"Game connected"},
    {L"Aguardando o jogo…", L"Waiting for the game…"},
    {L"1. Copie o conteúdo do zip para  …\\Euro Truck Simulator 2\\bin\\win_x64\\plugins",
     L"1. Copy the zip contents into  …\\Euro Truck Simulator 2\\bin\\win_x64\\plugins"},
    {L"2. Abra o ETS2 e aceite o aviso de \"SDK features\".", L"2. Start ETS2 and accept the \"SDK features\" prompt."},
    {L"3. Entre no jogo com o caminhão: esta tela conecta sozinha.",
     L"3. Get into your truck: this window connects by itself."},
    {L"Existe um status.json antigo, mas o plugin não está atualizando (jogo fechado?).",
     L"There is an old status.json, but the plugin is not updating it (game closed?)."},
    {L"Ver demonstração", L"See demo"},
    {L"Abrir pasta", L"Open folder"},
    {L"Pedal", L"Pedal"},
    {L"Velocidade", L"Speed"},
    {L"Freando há", L"Braking for"},
    {L"Som tocando", L"Playing"},
    {L"nenhum", L"none"},
    {L"freio", L"brake"},
    {L"saída de ar", L"air release"},
    {L"Ajustes", L"Settings"},
    {L"Freio", L"Brake"},
    {L"Saída de ar", L"Air release"},
    {L"Geral", L"General"},
    {L"Restaurar padrão", L"Restore defaults"},
    {L"Salvar", L"Save"},
    {L"alterações não salvas", L"unsaved changes"},
    {L"tudo salvo no .ini", L"all saved to the .ini"},
    {L"Demonstração", L"Demo"},
    {L"Sair da demo", L"Exit demo"},
    {L"Configurações", L"Settings"},
    {L"Sempre no topo", L"Always on top"},
    {L"Sem som: ", L"No sound: "},
    {L"Sem resposta do plugin (o jogo está aberto?).", L"No reply from the plugin (is the game running?)."},
    {L"Não foi possível gravar o comando na pasta Documentos\\ETS2AirbrakeAudio.",
     L"Could not write the command to Documents\\ETS2AirbrakeAudio."},
    {L"Modo demonstração: nada é enviado ao jogo.", L"Demo mode: nothing is sent to the game."},
    {L"Atalho salvo.", L"Shortcut saved."},
    {L"Atalho salvo. Vale na próxima vez que o jogo abrir.", L"Shortcut saved. It applies the next time the game starts."},
    {L"Configurações", L"Settings"},
    {L"Atalho no jogo para abrir o Airbrake Audio", L"In-game shortcut that opens Airbrake Audio"},
    {L"Aperte a combinação…", L"Press the combination…"},
    {L"Clique em Alterar e aperte a combinação de teclas que quiser.",
     L"Click Change and press the key combination you want."},
    {L"Ex.: Ctrl + =, Ctrl + F9, Alt + B.  Esc cancela.", L"E.g. Ctrl + =, Ctrl + F9, Alt + B.  Esc cancels."},
    {L"Essa tecla não pode ser usada. Tente outra.", L"That key cannot be used. Try another one."},
    {L"Use junto com Ctrl ou Alt (ou uma tecla F1–F24).", L"Use it with Ctrl or Alt (or an F1–F24 key)."},
    {L"Alterar…", L"Change…"},
    {L"Cancelar captura", L"Cancel capture"},
    {L"Usar o padrão (Ctrl + =)", L"Use the default (Ctrl + =)"},
    {L"Abra o jogo com o plugin uma vez para a tela achar o airbrake_audio.ini.",
     L"Start the game with the plugin once so this window can find airbrake_audio.ini."},
    {L"Cancelar", L"Cancel"},
    {L"Salvar", L"Save"},
    {L"Não consegui gravar o arquivo. A pasta do jogo pode estar protegida: edite \"hotkey\" no .ini à mão.",
     L"Could not write the file. The game folder may be protected: edit \"hotkey\" in the .ini by hand."},
};
static_assert(sizeof(kTexts) / sizeof(kTexts[0]) == static_cast<size_t>(Txt::Count), "um texto para cada Txt");

const FieldMeta kMetas[] = {
    {"brake.volume_min", Tab::Brake, L"Volume mínimo (pedal leve)", L"Minimum volume (light pedal)", L"%", 100, 1, 0},
    {"brake.volume_max", Tab::Brake, L"Volume máximo (pedal no fundo)", L"Maximum volume (full pedal)", L"%", 100, 1, 0},
    {"brake.min_speed_kmh", Tab::Brake, L"Mudo abaixo de", L"Muted below", L"km/h", 1, 1, 0},
    {"brake.fade_in_ms", Tab::Brake, L"Fade de entrada", L"Fade in", L"ms", 1, 10, 0},
    {"brake.fade_out_ms", Tab::Brake, L"Fade de saída", L"Fade out", L"ms", 1, 10, 0},
    {"air_release.volume", Tab::Air, L"Volume", L"Volume", L"%", 100, 1, 0},
    {"air_release.full_brake_time_s", Tab::Air, L"Freada para o som inteiro", L"Brake time for the full sound", L"s", 1,
     0.5, 1},
    {"air_release.short_volume", Tab::Air, L"Volume na freada curta", L"Volume after a short brake", L"%", 100, 1, 0},
    {"air_release.short_length_s", Tab::Air, L"Duração na freada curta", L"Length after a short brake", L"s", 1, 0.05,
     2},
    {"air_release.audible_length_s", Tab::Air, L"Corte máximo antes do inteiro", L"Longest early cut", L"s", 1, 0.05, 2},
    {"air_release.cut_fade_ms", Tab::Air, L"Fade do corte", L"Cut fade", L"ms", 1, 5, 0},
    {"air_release.min_brake_time_ms", Tab::Air, L"Freada mínima", L"Minimum brake time", L"ms", 1, 50, 0},
    {"air_release.cooldown_ms", Tab::Air, L"Intervalo entre saídas", L"Time between releases", L"ms", 1, 100, 0},
    {"output.boost_db", Tab::General, L"Ganho (boost)", L"Gain (boost)", L"dB", 1, 0.5, 1},
    {"trigger.threshold_on", Tab::General, L"Pedal: começa a frear acima de", L"Pedal: braking starts above", L"%", 100,
     1, 0},
    {"trigger.threshold_off", Tab::General, L"Pedal: solto abaixo de", L"Pedal: released below", L"%", 100, 1, 0},
};

} // namespace

const wchar_t *T(Lang lang, Txt id) {
    const Pair &p = kTexts[static_cast<size_t>(id)];
    return lang == Lang::Pt ? p.pt : p.en;
}

const FieldMeta *fieldMetas(size_t &count) {
    count = sizeof(kMetas) / sizeof(kMetas[0]);
    return kMetas;
}

const FieldMeta *findMeta(const std::string &id) {
    for (const FieldMeta &m : kMetas)
        if (id == m.id) return &m;
    return nullptr;
}

} // namespace aa::app
