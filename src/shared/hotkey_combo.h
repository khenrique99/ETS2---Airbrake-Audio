#pragma once
// Atalho de teclado no formato do .ini ("CTRL+PLUS"), o mesmo do Trailer Control.
// Usado pelo plugin (ler o atalho) e pelo app (capturar e mostrar "Ctrl + =").
#include <string>

namespace aa::hotkey {

struct Combo {
    bool ctrl = false, shift = false, alt = false;
    unsigned vk = 0; // 0 = invalido

    bool valid() const { return vk != 0; }
};

// Atalho padrao: Ctrl + = (o Trailer Control usa Ctrl + -, ao lado).
constexpr const char *kDefault = "CTRL+PLUS";

// "CTRL+SHIFT+F9" -> Combo. Aceita tambem os nomes da 1.x (KeyboardEvent.code: "Minus", "KeyP"...).
bool parse(const std::string &text, Combo &out);

// Nome da tecla no .ini ("A", "F9", "MINUS", "NUMPAD3"...); vazio = nao suportada.
std::string vkToIniName(unsigned vk);

// Monta "CTRL+SHIFT+F9". Vazio se a tecla nao for suportada.
std::string makeCombo(bool ctrl, bool shift, bool alt, unsigned vk);

// Sem Ctrl/Alt (e fora de F1-F24) a tecla dispararia durante o jogo normal.
bool safeForGame(const Combo &c);

// "CTRL+PLUS" -> "Ctrl + ="
std::wstring pretty(const std::string &combo);

} // namespace aa::hotkey
