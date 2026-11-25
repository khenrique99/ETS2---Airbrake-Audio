#include "settings_commands.h"

namespace aa {

namespace {

CommandOutcome fail(const std::string &message) {
    CommandOutcome o;
    o.message = message;
    return o;
}

CommandOutcome setField(const Command &cmd, Config &config) {
    if (cmd.args.size() != 2) return fail("Uso: definir <ajuste> <valor>");
    const FloatField *field = findField(cmd.args[0]);
    if (!field) return fail("Ajuste desconhecido: " + cmd.args[0]);
    if (!field->live) return fail(cmd.args[0] + " so vale ao reiniciar o jogo: edite o airbrake_audio.ini");
    float value = 0.0f;
    if (!parseNumber(cmd.args[1], value)) return fail("Valor invalido: " + cmd.args[1]);

    config.*(field->member) = value;
    sanitize(config);
    CommandOutcome o;
    o.ok = true;
    o.configChanged = true;
    o.message = field->id() + " = " + formatNumber(config.*(field->member));
    return o;
}

CommandOutcome resetDefaults(Config &config) {
    const Config defaults;
    size_t count = 0;
    const FloatField *fields = floatFields(count);
    for (size_t i = 0; i < count; ++i) config.*(fields[i].member) = defaults.*(fields[i].member);
    CommandOutcome o;
    o.ok = true;
    o.configChanged = true;
    o.message = "Valores padrao aplicados. Salve para manter.";
    return o;
}

} // namespace

CommandOutcome interpretCommand(const Command &cmd, Config &config) {
    if (cmd.action == "definir") return setField(cmd, config);
    if (cmd.action == "padrao") return resetDefaults(config);
    if (cmd.action == "salvar") {
        CommandOutcome o;
        o.ok = true;
        o.saveRequested = true;
        return o;
    }
    if (cmd.action == "recarregar_atalho") {
        CommandOutcome o;
        o.ok = true;
        o.reloadHotkey = true;
        return o;
    }
    return fail("Acao desconhecida: " + cmd.action);
}

} // namespace aa
