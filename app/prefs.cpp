#include "prefs.h"

#include "shared/ini_edit.h"
#include "shared/ipc.h"

#include <windows.h>

#include <cstdlib>

namespace aa::app {

namespace {

std::wstring prefsPath() {
    const std::wstring d = ipc::folder();
    return d.empty() ? d : d + L"\\app.ini";
}

Lang systemLang() { return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_PORTUGUESE ? Lang::Pt : Lang::En; }

} // namespace

void AppPrefs::load() {
    lang = systemLang();
    std::string text;
    if (!ipc::readAll(prefsPath(), text)) return;
    const std::string l = ini::get(text, "app", "idioma");
    if (l == "pt") lang = Lang::Pt;
    if (l == "en") lang = Lang::En;
    topmost = ini::get(text, "app", "sempre_no_topo") == "1";
    tab = std::atoi(ini::get(text, "app", "aba").c_str());
    if (tab < 0 || tab > 2) tab = 0;
    lastConfigPath = ipc::widen(ini::get(text, "app", "ultimo_config"));
}

bool AppPrefs::save() const {
    const std::wstring path = prefsPath();
    if (path.empty()) return false;
    std::string text;
    ipc::readAll(path, text);
    text = ini::apply(text, {{"app", "idioma", lang == Lang::Pt ? "pt" : "en"},
                             {"app", "sempre_no_topo", topmost ? "1" : "0"},
                             {"app", "aba", std::to_string(tab)},
                             {"app", "ultimo_config", ipc::narrow(lastConfigPath)}});
    return ipc::writeAtomic(path, text);
}

} // namespace aa::app
