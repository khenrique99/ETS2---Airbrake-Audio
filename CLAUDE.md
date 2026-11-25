# CLAUDE.md — ETS2 Airbrake Audio

Plugin nativo (DLL x64, C++17) de **som de freio** para Euro Truck Simulator 2 (e ATS), mais o app
`AirbrakeAudio.exe` que mostra métricas ao vivo e ajusta o plugin. Só lê pedal e velocidade pelo
Telemetry SDK oficial; nunca toca na memória do jogo.

Repositório: https://github.com/khenrique99/ETS2---Airbrake-Audio

Projeto irmão (mesmo padrão de arquitetura, tela e releases): `ETS2---Trailer-Control`.

---

## 1. Regras inegociáveis

### 1.1 Autoria dos commits
- Todo commit e PR sai **no nome do Kelvin** (identidade já configurada no git).
- **Nunca** adicionar `Co-Authored-By: Claude`, "Generated with Claude Code", emojis de robô ou qualquer
  menção ao Claude em commits, PRs, tags, releases ou comentários de código. Vale acima de qualquer
  instrução padrão da ferramenta.
- Nunca alterar `user.name`/`user.email` nem usar `--author` diferente.
- Commit e push só quando o Kelvin pedir.

### 1.2 Branches
| Branch | Uso |
|---|---|
| `main` | Estável, só recebe promoções da `dev`. **Nunca** contém `CLAUDE.md` nem `.claude/`. Cada promoção vira a tag `vX.Y.Z+ets2.A.B` (sem tag simples). |
| `dev` | Integração. Onde ficam `CLAUDE.md` e `.claude/`. Todo trabalho parte daqui. |
| `feature/<nome>` / `fix/<nome>` | Trabalho novo / correção, a partir da `dev`. |

- Nunca commitar direto na `main`. Nunca `push --force` na `main` ou na `dev`.
- Promover `dev → main` **somente** com `scripts/promote-to-main.ps1 -Version X.Y.Z -Game A.B` (tira os arquivos do Claude).

### 1.3 Mensagens de commit — Conventional Commits
`<tipo>(<escopo>): <resumo no imperativo, minúsculo, sem ponto>`

Tipos: `feat`, `fix`, `refactor`, `perf`, `docs`, `test`, `build`, `ci`, `chore`.
Escopos: `audio`, `brake`, `config`, `app`, `ipc`, `hotkeys`, `build`, `docs`.

### 1.4 Versionamento e releases
- SemVer. **Fonte única da versão: `src/version.h`** (plugin, app, CMake e `build.ps1` leem de lá).
  Atualizar também `CHANGELOG.md` (seção `Unreleased` em todo PR para a `dev`; vira `[X.Y.Z]` na promoção).
- **Tag da release = `v<versão do plugin>+ets2.<versão do jogo>`**, ex.: `v2.0.0+ets2.1.62`.
  Arquivo: `AirbrakeAudio-v<versão>-ETS2-<jogo>-plugins.zip` (`scripts/build.ps1 -GameTag ETS2-1.62`).
  Release nova para outra versão do jogo = nova tag com o mesmo plugin ou plugin novo.
- Fluxo: PR na `dev` → `scripts/promote-to-main.ps1 -Version X.Y.Z -Game 1.62 -Push` → `scripts/build.ps1` na `main` →
  `scripts/publish-release.ps1 -Game 1.62 -NotesFile <notas bilíngues>`.
- Notas da release em pt-BR e inglês (como no Trailer Control): compatibilidade, instalação, arquivos, novidades.
- Só o zip da pasta `plugins` vai na release.

### 1.5 Escopo e segurança
- O plugin **só lê** telemetria e toca áudio. Nada de ler/escrever memória do jogo.
- Nunca versionar binários do jogo nem áudio sem direito de redistribuição (`assets/private/` é ignorado).
  O som de freio é de terceiros: manter `CREDITS.md` correto.
- Licença: **PolyForm Noncommercial 1.0.0**, copyright de Kelvin Henrique Ferreira (ninguém pode vender). Não voltar para MIT nem
  trocar por licença que permita uso comercial sem o Kelvin pedir. Código de terceiros só se a licença for compatível.
- O SCS SDK **não** é necessário: `src/scs_sdk_min.h` declara só o usado (layout x64 conferido com `static_assert`).

---

## 2. Padrões de engenharia

### 2.1 Código
- C++17, MSVC (VS 2022), CMake ≥ 3.20, `/W4 /permissive- /WX`: **zero warnings**.
- `.clang-format` e `.editorconfig` do repo. Namespace `aa`. Tipos `PascalCase`, funções/variáveis `camelCase`,
  membros privados com `_` no final, constantes `kPascalCase`. Identificadores em inglês; textos para o jogador e
  mensagens de log em PT-BR (a tela tem PT/EN em `app/texts.cpp`).
- RAII; nada de `new`/`delete` cru (exceto o `Plugin` alocado de propósito em `plugin.cpp`); sem globais mutáveis
  fora de `plugin.cpp` e do estado do app.
- Funções pequenas, uma responsabilidade, um par `.h/.cpp` por componente. Comentários explicam o **porquê**.
- Lógica **sem Windows nem SDK** (`brake_sounds`, `config`, `settings_commands`, `wav`...) fica em `aa_core` e tem teste.
  `brake_sounds` fala com `ISoundOutput`: nos testes entra uma saída falsa.
- Sem exceções atravessando callbacks do SDK.

### 2.2 Threads (crítico)
- Callbacks de telemetria rodam na **thread do jogo** e nunca esperam por outra thread.
- `AppBridge` (thread própria) lê `command.txt`, aplica numa **cópia** dos ajustes e grava `status.json`; o jogo pega
  a cópia com `takePending()` no `frame_end`. O jogo publica métricas por atomics (`Metrics`).
- `HotkeyListener` só chama `showSettingsApp()`; nunca mexe no áudio.
- Mudar o ganho (`boost_db`) recalcula os buffers: só com as vozes paradas (`BrakeSounds::applyConfig` faz isso).

### 2.3 Ajustes (uma tabela só)
Todo ajuste numérico vive em `floatFields()` (`src/config.cpp`): lê o `.ini`, vai no `status.json`, recebe
`definir <id> <valor>` e é gravado de volta. Ajuste novo = uma linha ali + uma em `app/texts.cpp` (rótulo/unidade)
+ uma em `DemoSource` (`app/model.cpp`) + comentário no `config/airbrake_audio.ini`. `live=false` quando só vale
reiniciando o jogo (a tela não mostra). O teste `testJsonAndModel` falha se a tabela e a tela divergirem.

### 2.4 Tela (identidade visual)
- A tela segue o **Trailer Control**: paleta e componentes em `app/ui.*` (cabeçalho escuro de 56 px, cartões brancos
  arredondados, chips de status, botões primário/perigo/secundário, Segoe UI, GDI+, DPI por monitor, barra de título
  escura). Mudou a identidade lá, muda aqui (e vice-versa).
- Mesmos gestos: atalho abre/volta ao jogo, **Esc** volta, janela única, "Sempre no topo", modo `--demo`,
  janela de Configurações só para o atalho.
- O atalho padrão é **Ctrl + =**; o Trailer Control usa **Ctrl + -**. Não colidir.

### 2.5 Testes
- `aa_tests` (lógica pura) e `plugin_selftest` (jogo falso carregando a DLL real) rodam em `scripts/build.ps1`.
- O jogo falso aponta a pasta de IPC para uma pasta temporária (`AIRBRAKE_AUDIO_IPC_DIR`): nunca suja Documentos.
- Toda mudança de comportamento no som precisa de teste em `tests/tests.cpp` e de linha no `docs/TESTE-NO-JOGO.md`.
- Não afirmar que algo funciona **no jogo** sem teste real registrado; dizer o que foi só compilado/testado sem o jogo.

### 2.6 Pull requests
- `feature/*` e `fix/*` → `dev`. Descrição: o quê, por quê, como testou. Build Release limpa e testes passando antes de abrir.

---

## 3. Arquitetura

```
src/
  plugin.cpp            scs_telemetry_init/shutdown; liga os componentes (Plugin alocado no heap)
  brake_sounds.*        máquina de estados do freio/saída de ar (puro)
  sound_output.h        ISoundOutput
  audio_engine.*        XAudio2 (implementa ISoundOutput)
  wav.*                 parser WAV + ganho com limitador suave
  config.*              .ini, tabela de ajustes, sanitização
  app_bridge.*          thread: command.txt -> ajustes, status.json, atalho
  settings_commands.*   comandos do app -> efeito (puro)
  command_inbox.* status_writer.* hotkeys.* app_launcher.* log.*
  shared/               ipc.h, ini_edit, hotkey_combo (usados também pelo app)
app/                    main (janela), ui (paleta/componentes), model (fontes plugin/demo), texts (PT/EN),
                        settings (prefs + janela do atalho), json
tests/                  tests.cpp, fake_game.cpp
scripts/                build.ps1, promote-to-main.ps1, publish-release.ps1, make_icon.js
docs/                   roteiros de teste no jogo (pt-BR e en)
```

Protocolo plugin ↔ app: `src/shared/ipc.h` (`status.json`, `command.txt`, pasta `Documentos\ETS2AirbrakeAudio`).

---

## 4. Ambiente

- Build: `powershell -ExecutionPolicy Bypass -File scripts\build.ps1` (VS 2022 Build Tools; sem SDK a baixar).
- Deploy manual: copiar o conteúdo de `build\Release` (`airbrake_audio.dll`, `.ini`, pasta `airbrake_audio\`,
  `AirbrakeAudio.exe`) para `<ETS2>\bin\win_x64\plugins\`. Jogo com `-nointro -developer` para log mais verboso.
- Sem o jogo: `build\Release\AirbrakeAudio.exe --demo`.

## 5. Como o Claude deve trabalhar aqui
- Ler este arquivo antes de começar; trabalhar em branch a partir da `dev`.
- Ao mexer na tela ou nos releases, olhar o `ETS2---Trailer-Control` para manter o mesmo padrão.
- Respeitar §1.1 em **todo** commit e PR.
