# Airbrake Audio

[English](README.md) | [Português (Brasil)](README.pt-BR.md)

A sound-only telemetry plugin for **Euro Truck Simulator 2** (and ATS) that plays a
glazed-lining brake squeal while the brake pedal is pressed, and an air release when it
is let go.

> **Status: early development (0.x).** The plugin builds and loads as a telemetry plugin,
> but it has not been validated in-game yet. Expect changes before 1.0.0.

## Features

- **Brake pressed:** a looping brake sound; the volume follows the pedal and fades in/out.
- **Brake released:** an air release. A long application plays the whole sound at full
  volume; a short one plays quieter and is cut early.
- **Own volume control:** a gain stage with a soft limiter (`boost_db`), independent of the
  game's volume slider.
- **Everything configurable** in one commented `.ini` file.

## It does not touch game mechanics

The plugin only **reads** the brake pedal and the speed through the official SCS Telemetry
SDK and plays audio. It does not change brake temperature, braking force or physics, and it
does not read or patch game memory. It coexists with other brake-heating plugins.

If another plugin also plays brake sounds, both will play at once. Turn the sound off in
that plugin's own configuration instead of deleting its files.

## Install

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

## Verify

Open `game.log.txt` (`Documents\Euro Truck Simulator 2`) and look for
`[airbrake_audio] v... ready (...)`. On failure the same prefix reports what is wrong:
sound file not found, unsupported format, or no audio output device.

## Configure

Edit `airbrake_audio.ini`; every option is commented. Restart the game afterwards.

To use your own sounds, put 16-bit PCM WAV files (mono or stereo) in the
`plugins\airbrake_audio` folder and set `file=` in the `[brake]` and `[air_release]`
sections. For a brake loop, set `loop_start_s` to the second where the loop should restart.

## Build from source

Requires the Visual Studio Build Tools with the "Desktop development with C++" workload.

```bat
winget install Microsoft.VisualStudio.2022.BuildTools
build.bat
```

The output is `dist\plugins`. A CMake project is also provided (`CMakeLists.txt`).

## Project layout

```
src/        plugin source (plugin.cpp, audio.h, config.h, scs_sdk_min.h, version.h)
config/     default airbrake_audio.ini
assets/     default sounds
build.bat   MSVC build + packaging script
```

## Contributing

Commits follow [Conventional Commits](https://www.conventionalcommits.org/) and versions
follow [Semantic Versioning](https://semver.org/). See [CHANGELOG.md](CHANGELOG.md).

## License

Code: [MIT](LICENSE). Sounds: see [CREDITS.md](CREDITS.md).
