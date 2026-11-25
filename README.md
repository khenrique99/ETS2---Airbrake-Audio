# ETS2 Airbrake Audio

**🇧🇷 Português** · [🇺🇸 English](README.en.md)

Plugin de **som** para **Euro Truck Simulator 2** (e ATS) + uma tela (**AirbrakeAudio.exe**) para ajustar tudo
dentro do jogo. Toca um ranger de freio com lona vitrificada enquanto o pedal está apertado e uma saída de ar
quando ele é solto.

| | |
|---|---|
| **Versão** | 2.0.0 |
| **Jogo** | ETS2 **1.62.x** (Steam, 64 bits); só lê pedal e velocidade pelo Telemetry SDK |
| **Sistema** | Windows 10/11 64 bits |
| **Modo** | single-player e multiplayer (só toca áudio local, não altera nada do jogo) |

## Download e instalação

1. Baixe **`AirbrakeAudio-...-plugins.zip`** na página de
   [Releases](https://github.com/khenrique99/ETS2---Airbrake-Audio/releases). O nome da versão indica o jogo
   (`v2.0.0+ets2.1.62` = plugin 2.0.0 feito para o ETS2 1.62.x).
2. Extraia o conteúdo **dentro da pasta `plugins`** do jogo:
   `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (crie a pasta se não existir).
   Devem ficar lá:

   ```
   plugins\airbrake_audio.dll
   plugins\airbrake_audio.ini
   plugins\AirbrakeAudio.exe
   plugins\airbrake_audio\brake_loop.wav
   plugins\airbrake_audio\air_release.wav
   ```

3. Abra o jogo e aceite o aviso de **"SDK features"**.
4. Confira no `game.log.txt` (`Documentos\Euro Truck Simulator 2`) a linha `[AirbrakeAudio] v... pronto`.
   Se der problema, a mesma linha diz o motivo (arquivo de som não encontrado, formato não suportado, sem dispositivo de áudio).

## Como usar

- **Ctrl + =** dentro do jogo abre a tela por cima. **Esc** ou o mesmo atalho voltam ao jogo.
- A tela mostra ao vivo o pedal, a velocidade, o tempo de freada e o som que está tocando.
- Os **Ajustes** (abas Freio, Saída de ar, Geral) valem na hora; **Salvar** grava no `airbrake_audio.ini`
  sem apagar os comentários. **Restaurar padrão** volta tudo ao original (e só grava se você salvar).
- **Configurações** (rodapé): troque o atalho que abre a tela — clique em **Alterar…**, aperte a combinação e **Salvar**.
- **PT/EN** troca o idioma da tela. **Demonstração** simula freadas sem o jogo aberto.
- Use o jogo em **janela sem borda** para a tela aparecer por cima; em tela cheia exclusiva o Windows pode escondê-la.
- O atalho padrão (**Ctrl + =**) convive com o do [Trailer Control](https://github.com/khenrique99/ETS2---Trailer-Control) (**Ctrl + -**).

Para usar seus próprios sons, coloque arquivos WAV PCM 16 bits (mono ou estéreo) em `plugins\airbrake_audio` e
ajuste `file=` em `[brake]` e `[air_release]`. Para um loop de freio, defina `loop_start_s` (vale ao reiniciar o jogo).

## Não mexe na mecânica do jogo

O plugin só **lê** o pedal de freio e a velocidade pelo Telemetry SDK oficial da SCS e toca áudio. Não altera
temperatura do freio, força de frenagem nem física, e não lê nem modifica a memória do jogo. Convive com plugins de
aquecimento de freio. Se outro plugin também toca sons de freio, os dois tocam ao mesmo tempo: desative o som na
configuração do outro plugin em vez de apagar os arquivos dele.

## Para desenvolvedores

### Compilar

Requisitos: Visual Studio 2022 ou Build Tools (C++ desktop); o CMake vem junto. **Não precisa baixar o SCS SDK**
(`src/scs_sdk_min.h` declara só o que é usado).

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

Compila, roda os testes e gera em `dist\` o zip da release. O nome leva a versão do plugin (`src/version.h`) e a do
jogo (`-GameTag ETS2-1.62`, padrão). `-SkipTests` pula os testes.

### Testes (sem o jogo)

- `aa_tests` — WAV, configuração, lógica do freio (com saída de áudio falsa), comandos, atalhos, JSON e modelo da tela.
- `plugin_selftest` — o **jogo falso** (`tests/fake_game.cpp`) carrega a DLL de verdade pelo SDK, simula pedal e
  manda comandos como a tela faria, conferindo o `status.json`.

Roteiro no jogo: [docs/TESTE-NO-JOGO.md](docs/TESTE-NO-JOGO.md).

### Comunicação plugin ↔ tela

Arquivos em `Documentos\ETS2AirbrakeAudio\`: `status.json` (plugin → tela) e `command.txt` (tela → plugin).
Protocolo em [src/shared/ipc.h](src/shared/ipc.h).

### Estrutura

```
src/
  plugin.cpp            entradas scs_telemetry_*; liga os componentes
  brake_sounds.*        lógica do freio e da saída de ar (sem Windows: testável)
  sound_output.h        interface de saída de som (ISoundOutput)
  audio_engine.*        XAudio2: toca os WAV
  wav.*                 leitura de WAV e ganho com limitador
  config.*              airbrake_audio.ini (tabela única de ajustes)
  app_bridge.*          thread: command.txt, status.json, atalho
  settings_commands.*   o que cada comando da tela faz
  command_inbox.* status_writer.* hotkeys.* app_launcher.* log.*
  shared/               protocolo plugin <-> tela, edição de .ini, atalhos
app/                    AirbrakeAudio.exe (Win32 + GDI+, sem dependências): main, ui, model, texts, settings
tests/                  testes + jogo falso
assets/sounds/          sons padrão
```

## Contribuindo

Commits seguem [Conventional Commits](https://www.conventionalcommits.org/) e as versões seguem o
[Versionamento Semântico](https://semver.org/lang/pt-BR/); veja o [CHANGELOG.md](CHANGELOG.md).

## Licença

Código: © 2026 Kelvin Henrique Ferreira, sob a [PolyForm Noncommercial 1.0.0](LICENSE): uso pessoal e não comercial liberado; **vender ou usar comercialmente não é permitido** sem autorização do autor. Sons: veja [CREDITS.md](CREDITS.md) (o som de freio é de terceiros e **não** é coberto por esta licença).
Projeto de fã, sem vínculo com a SCS Software. Euro Truck Simulator 2 é marca da SCS Software.
