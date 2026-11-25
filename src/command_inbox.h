#pragma once
#include <string>
#include <vector>

namespace aa {

// Comando vindo do app (Documentos\ETS2AirbrakeAudio\command.txt). Ver shared/ipc.h.
struct Command {
    long long seq = 0;
    std::string action;            // ex.: "definir"
    std::vector<std::string> args; // ex.: {"brake.volume_max", "0.8"}
};

// Le e consome command.txt. Barato quando nao ha arquivo (uma consulta de atributos).
std::vector<Command> takeCommands();

// Interpreta "<seq> <acao> [arg1] [arg2]". Rejeita qualquer coisa fora do formato. Exposto para teste.
bool parseCommandLine(const std::string &line, Command &out);

} // namespace aa
