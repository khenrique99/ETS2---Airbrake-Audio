#include "shared/hotkey_combo.h"

#include <windows.h>

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace aa::hotkey {

namespace {

std::string upper(std::string s) {
    for (char &c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string trimmed(const std::string &s) {
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t") - b + 1);
}

struct Named {
    const char *ini;
    unsigned vk;
    const wchar_t *shown;
};

// Nomes aceitos no .ini; o primeiro de cada tecla e o que o app grava.
const Named kNamed[] = {
    {"MINUS", VK_OEM_MINUS, L"-"},      {"PLUS", VK_OEM_PLUS, L"="},       {"NUMMINUS", VK_SUBTRACT, L"- (num)"},
    {"NUMPLUS", VK_ADD, L"+ (num)"},    {"LEFT", VK_LEFT, L"←"},           {"RIGHT", VK_RIGHT, L"→"},
    {"UP", VK_UP, L"↑"},                {"DOWN", VK_DOWN, L"↓"},           {"HOME", VK_HOME, L"Home"},
    {"END", VK_END, L"End"},            {"INSERT", VK_INSERT, L"Insert"},  {"DELETE", VK_DELETE, L"Delete"},
    {"PGUP", VK_PRIOR, L"Page Up"},     {"PGDN", VK_NEXT, L"Page Down"},   {"SPACE", VK_SPACE, L"Espaço"},
    {"ENTER", VK_RETURN, L"Enter"},     {"TAB", VK_TAB, L"Tab"},           {"BACKSPACE", VK_BACK, L"Backspace"},
    // nomes da 1.x (KeyboardEvent.code), so para ler .ini antigo
    {"-", VK_OEM_MINUS, L"-"},          {"EQUAL", VK_OEM_PLUS, L"="},      {"NUMPADSUBTRACT", VK_SUBTRACT, L"- (num)"},
    {"NUMPADADD", VK_ADD, L"+ (num)"},  {"PAGEUP", VK_PRIOR, L"Page Up"},  {"PAGEDOWN", VK_NEXT, L"Page Down"},
};

unsigned keyNameToVk(const std::string &name) {
    for (const Named &n : kNamed)
        if (name == n.ini) return n.vk;
    if (name.size() == 1) {
        const char c = name[0];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return static_cast<unsigned>(c);
    }
    if (name.size() == 4 && name.rfind("KEY", 0) == 0) return keyNameToVk(name.substr(3));
    if (name.size() == 6 && name.rfind("DIGIT", 0) == 0) return keyNameToVk(name.substr(5));
    if (name.size() >= 2 && name.size() <= 3 && name[0] == 'F') {
        const int n = std::atoi(name.c_str() + 1);
        if (n >= 1 && n <= 24 && std::to_string(n) == name.substr(1)) return static_cast<unsigned>(VK_F1 + n - 1);
    }
    if (name.size() == 7 && name.rfind("NUMPAD", 0) == 0 && name[6] >= '0' && name[6] <= '9')
        return static_cast<unsigned>(VK_NUMPAD0 + (name[6] - '0'));
    return 0;
}

} // namespace

bool parse(const std::string &text, Combo &out) {
    Combo c;
    std::stringstream ss(text);
    std::string part;
    while (std::getline(ss, part, '+')) {
        const std::string p = upper(trimmed(part));
        if (p.empty()) continue;
        if (p == "CTRL") c.ctrl = true;
        else if (p == "SHIFT") c.shift = true;
        else if (p == "ALT") c.alt = true;
        else if (c.vk == 0) c.vk = keyNameToVk(p);
        else return false; // duas teclas "de verdade"
    }
    if (!c.valid()) return false;
    out = c;
    return true;
}

std::string vkToIniName(unsigned vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, static_cast<char>(vk));
    if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return "NUMPAD" + std::to_string(vk - VK_NUMPAD0);
    for (const Named &n : kNamed)
        if (n.vk == vk) return n.ini;
    return {};
}

std::string makeCombo(bool ctrl, bool shift, bool alt, unsigned vk) {
    const std::string key = vkToIniName(vk);
    if (key.empty()) return {};
    std::string c;
    if (ctrl) c += "CTRL+";
    if (shift) c += "SHIFT+";
    if (alt) c += "ALT+";
    return c + key;
}

bool safeForGame(const Combo &c) { return c.valid() && (c.ctrl || c.alt || (c.vk >= VK_F1 && c.vk <= VK_F24)); }

std::wstring pretty(const std::string &combo) {
    Combo c;
    if (!parse(combo, c)) return std::wstring(combo.begin(), combo.end());
    std::wstring out;
    if (c.ctrl) out += L"Ctrl + ";
    if (c.shift) out += L"Shift + ";
    if (c.alt) out += L"Alt + ";
    const std::string key = vkToIniName(c.vk);
    for (const Named &n : kNamed) {
        if (key == n.ini) return out + n.shown;
    }
    if (key.rfind("NUMPAD", 0) == 0) return out + L"Num " + std::wstring(1, static_cast<wchar_t>(key[6]));
    return out + std::wstring(key.begin(), key.end());
}

} // namespace aa::hotkey
