# Airbrake Audio

[Português (Brasil)](#português-brasil) | [English](#english)

---

## Português (Brasil)

Plugin de telemetria, somente de som, para o **Euro Truck Simulator 2** (e ATS). Toca um
ranger de freio com lona vitrificada enquanto o pedal de freio está pressionado e uma
saída de ar quando ele é solto.

> **Status: em desenvolvimento inicial (0.x).** O plugin foi validado dentro do jogo
> (carrega e toca os sons). Esperam-se ajustes antes da 1.0.0.

### Recursos

- **Freio pressionado:** som de freio em loop; o volume acompanha o pedal, com fade de
  entrada e saída.
- **Freio solto:** saída de ar. Uma freada longa toca o som inteiro, em volume cheio; uma
  curta toca mais baixo e é cortada cedo.
- **Controle de volume próprio:** ganho com limitador suave (`boost_db`), independente do
  controle de volume do jogo.
- **Tudo configurável** em um único arquivo `.ini` comentado.

### Não mexe na mecânica do jogo

O plugin apenas **lê** o pedal de freio e a velocidade pelo Telemetry SDK oficial da SCS e
toca áudio. Ele não altera temperatura do freio, força de frenagem nem física, e não lê nem
modifica a memória do jogo. Convive com outros plugins de aquecimento de freio.

Se outro plugin também toca sons de freio, os dois tocam ao mesmo tempo. Desative o som na
configuração do outro plugin em vez de apagar os arquivos dele.

### Instalação

1. Baixe `airbrake-audio-<versão>-win64.zip` na página de
   [Releases](../../releases).
2. Copie o **conteúdo** da pasta `plugins` para
   `...\Euro Truck Simulator 2\bin\win_x64\plugins` (crie a pasta se necessário):

   ```
   plugins\airbrake_audio.dll
   plugins\airbrake_audio.ini
   plugins\airbrake_audio\brake_loop.wav
   plugins\airbrake_audio\air_release.wav
   ```

3. Abra o jogo e aceite o aviso único sobre recursos avançados do SDK.

Requer o jogo em 64 bits, no Windows 10 ou superior.

### Conferir

Abra o `game.log.txt` (`Documentos\Euro Truck Simulator 2`) e procure
`[airbrake_audio] v... ready (...)`. Em caso de falha, o mesmo prefixo informa o problema:
arquivo de som não encontrado, formato não suportado ou nenhum dispositivo de áudio.

### Configurar

Edite `airbrake_audio.ini`; todas as opções têm comentários. Reinicie o jogo depois.

Para usar seus próprios sons, coloque arquivos WAV PCM 16-bit (mono ou estéreo) na pasta
`plugins\airbrake_audio` e ajuste `file=` nas seções `[brake]` e `[air_release]`. Para um
loop de freio, defina `loop_start_s` com o segundo em que o loop deve recomeçar.

### Compilar a partir do código

Requer o Visual Studio Build Tools com a carga "Desenvolvimento para desktop com C++".

```bat
winget install Microsoft.VisualStudio.2022.BuildTools
build.bat
```

A saída fica em `dist\plugins`. Também há um projeto CMake (`CMakeLists.txt`).

### Estrutura do projeto

```
src/        código do plugin (plugin.cpp, audio.h, config.h, scs_sdk_min.h, version.h)
config/     airbrake_audio.ini padrão
assets/     sons padrão
build.bat   script de compilação e empacotamento (MSVC)
```

### Contribuindo

Os commits seguem [Conventional Commits](https://www.conventionalcommits.org/) e as versões
seguem o [Versionamento Semântico](https://semver.org/lang/pt-BR/). Veja o
[CHANGELOG.md](CHANGELOG.md).

### Licença

Código: [MIT](LICENSE). Sons: veja [CREDITS.md](CREDITS.md) (o som de freio é de
terceiros e **não** é coberto pela MIT).

---

## English

A sound-only telemetry plugin for **Euro Truck Simulator 2** (and ATS) that plays a
glazed-lining brake squeal while the brake pedal is pressed, and an air release when it
is let go.

> **Status: early development (0.x).** The plugin has been validated in-game (it loads and
> plays the sounds). Expect adjustments before 1.0.0.

### Features

- **Brake pressed:** a looping brake sound; the volume follows the pedal and fades in/out.
- **Brake released:** an air release. A long application plays the whole sound at full
  volume; a short one plays quieter and is cut early.
- **Own volume control:** a gain stage with a soft limiter (`boost_db`), independent of the
  game's volume slider.
- **Everything configurable** in one commented `.ini` file.

### It does not touch game mechanics

The plugin only **reads** the brake pedal and the speed through the official SCS Telemetry
SDK and plays audio. It does not change brake temperature, braking force or physics, and it
does not read or patch game memory. It coexists with other brake-heating plugins.

If another plugin also plays brake sounds, both will play at once. Turn the sound off in
that plugin's own configuration instead of deleting its files.

### Install

1. Download `airbrake-audio-<version>-win64.zip` from the
   [Releases](../../releases) page.
2. Copy the **contents** of the `plugins` folder to
   `...\Euro Truck Simulator 2\bin\win_x64\plugins` (create it if needed):

   ```
   plugins\airbrake_audio.dll
   plugins\airbrake_audio.ini
   plugins\airbrake_audio\brake_loop.wav
   plugins\airbrake_audio\air_release.wav
   ```

3. Start the game and accept the one-time prompt about advanced SDK features.

Requires a 64-bit game on Windows 10 or later.

### Verify

Open `game.log.txt` (`Documents\Euro Truck Simulator 2`) and look for
`[airbrake_audio] v... ready (...)`. On failure the same prefix reports what is wrong:
sound file not found, unsupported format, or no audio output device.

### Configure

Edit `airbrake_audio.ini`; every option is commented. Restart the game afterwards.

To use your own sounds, put 16-bit PCM WAV files (mono or stereo) in the
`plugins\airbrake_audio` folder and set `file=` in the `[brake]` and `[air_release]`
sections. For a brake loop, set `loop_start_s` to the second where the loop should restart.

### Build from source

Requires the Visual Studio Build Tools with the "Desktop development with C++" workload.

```bat
winget install Microsoft.VisualStudio.2022.BuildTools
build.bat
```

The output is `dist\plugins`. A CMake project is also provided (`CMakeLists.txt`).

### Project layout

```
src/        plugin source (plugin.cpp, audio.h, config.h, scs_sdk_min.h, version.h)
config/     default airbrake_audio.ini
assets/     default sounds
build.bat   MSVC build + packaging script
```

### Contributing

Commits follow [Conventional Commits](https://www.conventionalcommits.org/) and versions
follow [Semantic Versioning](https://semver.org/). See [CHANGELOG.md](CHANGELOG.md).

### License

Code: [MIT](LICENSE). Sounds: see [CREDITS.md](CREDITS.md) (the brake sound is third-party
and is **not** covered by the MIT license).
