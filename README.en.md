# ETS2 Airbrake Audio

[🇧🇷 Português](README.md) · **🇺🇸 English**

A **sound** plugin for **Euro Truck Simulator 2** (and ATS) plus a window (**AirbrakeAudio.exe**) to tune everything
from inside the game. It plays a glazed-lining brake squeal while the pedal is pressed and an air release when it is let go.

| | |
|---|---|
| **Version** | 2.0.0 |
| **Game** | ETS2 **1.62.x** (Steam, 64-bit); it only reads the pedal and speed through the Telemetry SDK |
| **System** | Windows 10/11 64-bit |
| **Mode** | single-player and multiplayer (it only plays local audio, it changes nothing in the game) |

## Download and install

1. Download **`AirbrakeAudio-...-plugins.zip`** from the
   [Releases](https://github.com/khenrique99/ETS2---Airbrake-Audio/releases) page. The release name carries the game
   version (`v2.0.0+ets2.1.62` = plugin 2.0.0 built for ETS2 1.62.x).
2. Extract it **into the game's `plugins` folder**:
   `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (create it if needed). You should get:

   ```
   plugins\airbrake_audio.dll
   plugins\airbrake_audio.ini
   plugins\AirbrakeAudio.exe
   plugins\airbrake_audio\brake_loop.wav
   plugins\airbrake_audio\brake_loop_low.wav
   plugins\airbrake_audio\air_release.wav
   ```

3. Start the game and accept the **"SDK features"** prompt.
4. Check `game.log.txt` (`Documents\Euro Truck Simulator 2`) for `[AirbrakeAudio] v... pronto`. On failure the same line
   says why (sound file not found, unsupported format, no audio device).

## Usage

- **Ctrl + =** in game opens the window on top. **Esc** or the same shortcut goes back to the game.
- The window shows the pedal, speed, brake time and the sound playing, live.
- **Settings** (Brake, Air release, General tabs) apply immediately; **Save** writes `airbrake_audio.ini`
  keeping its comments. **Restore defaults** resets everything (and only writes if you save).
- **Configurações** (footer, "Settings"): change the shortcut that opens the window — click **Change…**, press the combination, **Save**.
- **PT/EN** switches the window language. **Demo** simulates brake applications without the game running.
- Run the game in **borderless windowed** for the window to appear on top; in exclusive fullscreen Windows may hide it.
- The default shortcut (**Ctrl + =**) coexists with [Trailer Control](https://github.com/khenrique99/ETS2---Trailer-Control) (**Ctrl + -**).

To use your own sounds, put 16-bit PCM WAV files (mono or stereo) in `plugins\airbrake_audio` and set `file=` in
`[brake]` (`file=` high temperature, `file_low=` low temperature) and `file=` in `[air_release]`. For the brake loops,
set `loop_start_s` / `loop_start_low_s` (applies after restarting the game).

The brake has two sounds: with the pedal up to **65%** the **low temperature** one plays; from there on, the
**high temperature** one. The swap crossfades with the fades (no hard cut) and the threshold is adjustable in the Brake
tab ("High-temperature sound from", `high_from` in the `.ini`). The **air release** only plays on release if the
brake application spent at least **2 s** on the high temperature sound (`min_high_time_s` in `[air_release]`, 0 = always).

## It does not touch game mechanics

The plugin only **reads** the brake pedal and speed through the official SCS Telemetry SDK and plays audio. It does not
change brake temperature, braking force or physics, and it does not read or patch game memory. It coexists with
brake-heating plugins. If another plugin also plays brake sounds, both play at once: turn the sound off in that plugin's
own configuration instead of deleting its files.

## For developers

### Build

Requires Visual Studio 2022 or Build Tools (Desktop C++); CMake ships with it. **No SCS SDK download needed**
(`src/scs_sdk_min.h` declares only what is used).

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

It builds, runs the tests and writes the release zip to `dist\`. The name carries the plugin version (`src/version.h`)
and the game version (`-GameTag ETS2-1.62`, the default). `-SkipTests` skips the tests.

### Tests (no game)

- `aa_tests` — WAV, configuration, brake logic (with a fake audio output), commands, shortcuts, JSON and the window model.
- `plugin_selftest` — the **fake game** (`tests/fake_game.cpp`) loads the real DLL through the SDK, simulates the pedal
  and sends commands like the window would, checking `status.json`.

In-game script: [docs/TESTING-IN-GAME.md](docs/TESTING-IN-GAME.md).

### Plugin ↔ window protocol

Files in `Documents\ETS2AirbrakeAudio\`: `status.json` (plugin → window) and `command.txt` (window → plugin).
Protocol in [src/shared/ipc.h](src/shared/ipc.h).

## Contributing

Commits follow [Conventional Commits](https://www.conventionalcommits.org/) and versions follow
[Semantic Versioning](https://semver.org/); see [CHANGELOG.md](CHANGELOG.md).

## License

Code: © 2026 Kelvin Henrique Ferreira, under the [PolyForm Noncommercial 1.0.0](LICENSE): personal and non-commercial use is free; **selling it or using it commercially is not allowed** without the author's permission. Sounds: see [CREDITS.md](CREDITS.md) (the brake sound is third-party and **not** covered by this license).
Fan project, not affiliated with SCS Software. Euro Truck Simulator 2 is a trademark of SCS Software.
