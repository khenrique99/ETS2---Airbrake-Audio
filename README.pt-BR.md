# Airbrake Audio

[English](README.md) | [Português (Brasil)](README.pt-BR.md)

Plugin de telemetria, somente de som, para o **Euro Truck Simulator 2** (e ATS). Toca um
ranger de freio com lona vitrificada enquanto o pedal de freio está pressionado e uma
saída de ar quando ele é solto.

> **Status: em desenvolvimento inicial (0.x).** O plugin compila e carrega como plugin de
> telemetria, mas ainda não foi validado dentro do jogo. Esperam-se mudanças antes da 1.0.0.

## Recursos

- **Freio pressionado:** som de freio em loop; o volume acompanha o pedal, com fade de
  entrada e saída.
- **Freio solto:** saída de ar. Uma freada longa toca o som inteiro, em volume cheio; uma
  curta toca mais baixo e é cortada cedo.
- **Controle de volume próprio:** ganho com limitador suave (`boost_db`), independente do
  controle de volume do jogo.
- **Tudo configurável** em um único arquivo `.ini` comentado.

## Não mexe na mecânica do jogo

O plugin apenas **lê** o pedal de freio e a velocidade pelo Telemetry SDK oficial da SCS e
toca áudio. Ele não altera temperatura do freio, força de frenagem nem física, e não lê nem
modifica a memória do jogo. Convive com outros plugins de aquecimento de freio.

Se outro plugin também toca sons de freio, os dois tocam ao mesmo tempo. Desative o som na
configuração do outro plugin em vez de apagar os arquivos dele.

## Instalação

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

## Conferir

Abra o `game.log.txt` (`Documentos\Euro Truck Simulator 2`) e procure
`[airbrake_audio] v... ready (...)`. Em caso de falha, o mesmo prefixo informa o problema:
arquivo de som não encontrado, formato não suportado ou nenhum dispositivo de áudio.

## Configurar

Edite `airbrake_audio.ini`; todas as opções têm comentários. Reinicie o jogo depois.

Para usar seus próprios sons, coloque arquivos WAV PCM 16-bit (mono ou estéreo) na pasta
`plugins\airbrake_audio` e ajuste `file=` nas seções `[brake]` e `[air_release]`. Para um
loop de freio, defina `loop_start_s` com o segundo em que o loop deve recomeçar.

## Compilar a partir do código

Requer o Visual Studio Build Tools com a carga "Desenvolvimento para desktop com C++".

```bat
winget install Microsoft.VisualStudio.2022.BuildTools
build.bat
```

A saída fica em `dist\plugins`. Também há um projeto CMake (`CMakeLists.txt`).

## Estrutura do projeto

```
src/        código do plugin (plugin.cpp, audio.h, config.h, scs_sdk_min.h, version.h)
config/     airbrake_audio.ini padrão
assets/     sons padrão
build.bat   script de compilação e empacotamento (MSVC)
```

## Contribuindo

Os commits seguem [Conventional Commits](https://www.conventionalcommits.org/) e as versões
seguem o [Versionamento Semântico](https://semver.org/lang/pt-BR/). Veja o
[CHANGELOG.md](CHANGELOG.md).

## Licença

Código: [MIT](LICENSE). Sons: veja [CREDITS.md](CREDITS.md).
