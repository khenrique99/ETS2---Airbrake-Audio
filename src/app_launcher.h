#pragma once
#include <string>

namespace aa {

// Mostra a tela do Airbrake Audio (AirbrakeAudio.exe, na mesma pasta da DLL).
// Se ja estiver aberta, traz para a frente; senao, abre. Passa a janela do jogo para o app
// conseguir devolver o foco ("voltar ao jogo"). Pode ser chamado de qualquer thread.
// Devolve uma mensagem para o log.
std::string showSettingsApp();

} // namespace aa
