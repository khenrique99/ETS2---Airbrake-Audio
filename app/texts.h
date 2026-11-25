#pragma once
// Textos da tela em portugues e ingles, e como cada ajuste aparece (rotulo, unidade, passo, aba).
#include <string>

namespace aa::app {

enum class Lang { Pt, En };

enum class Txt {
    ReturnToGame,
    ChipDemo,
    ChipWaiting,
    ChipPaused,
    ChipConnected,
    WaitingTitle,
    WaitingStep1,
    WaitingStep2,
    WaitingStep3,
    WaitingStale,
    SeeDemo,
    OpenFolder,
    Pedal,
    Speed,
    BrakingFor,
    Playing,
    SoundNone,
    SoundBrake,
    SoundAir,
    Settings,
    TabBrake,
    TabAir,
    TabGeneral,
    RestoreDefaults,
    Save,
    Unsaved,
    AllSaved,
    Demo,
    ExitDemo,
    Configure,
    AlwaysOnTop,
    NoSound,
    NoReply,
    CantWrite,
    DemoMessage,
    HotkeySaved,
    HotkeySavedOffline,
    // janela de Configuracoes
    DlgTitle,
    DlgHeading,
    DlgPress,
    DlgHintIdle,
    DlgHintCapture,
    DlgInvalidKey,
    DlgNeedModifier,
    DlgChange,
    DlgCancelCapture,
    DlgUseDefault,
    DlgNoIni,
    DlgCancel,
    DlgSave,
    DlgWriteError,
    Count
};

const wchar_t *T(Lang lang, Txt id);

enum class Tab { Brake = 0, Air = 1, General = 2 };

struct FieldMeta {
    const char *id;     // "brake.volume_max" (mesmo id do status.json)
    Tab tab;
    const wchar_t *pt;
    const wchar_t *en;
    const wchar_t *unit;
    double scale;       // valor mostrado = valor * scale (ex.: 0..1 -> 0..100 %)
    double step;        // passo no valor mostrado
    int decimals;
};

const FieldMeta *fieldMetas(size_t &count);
const FieldMeta *findMeta(const std::string &id);

} // namespace aa::app
