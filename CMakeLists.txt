cmake_minimum_required(VERSION 3.20)

# Versao unica em src/version.h
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" AA_VERSION_LINE REGEX "define AIRBRAKE_AUDIO_VERSION \"")
string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" AA_VERSION "${AA_VERSION_LINE}")
project(airbrake_audio VERSION ${AA_VERSION} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Runtime estatico: o usuario nao precisa instalar o VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(AA_DEFINES WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
set(AA_MSVC_OPTIONS /W4 /permissive- /utf-8 /WX)

function(aa_target_defaults target)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/app")
    target_compile_definitions(${target} PRIVATE ${AA_DEFINES})
    if(MSVC)
        target_compile_options(${target} PRIVATE ${AA_MSVC_OPTIONS})
    endif()
endfunction()

# ---------------------------------------------------------------- logica sem Windows/SDK (testavel)
add_library(aa_core STATIC
    src/config.cpp
    src/wav.cpp
    src/brake_sounds.cpp
    src/command_inbox.cpp
    src/settings_commands.cpp
    src/status_writer.cpp
    src/shared/ini_edit.cpp
    src/shared/hotkey_combo.cpp
)
aa_target_defaults(aa_core)

# ---------------------------------------------------------------- plugin (DLL do jogo)
# Usa src/scs_sdk_min.h: nao precisa baixar o SCS SDK.
add_library(airbrake_audio SHARED
    src/plugin.cpp
    src/log.cpp
    src/audio_engine.cpp
    src/app_bridge.cpp
    src/app_launcher.cpp
    src/hotkeys.cpp
    src/exports.def
)
aa_target_defaults(airbrake_audio)
target_link_libraries(airbrake_audio PRIVATE aa_core xaudio2 ole32 shell32 user32)
set_target_properties(airbrake_audio PROPERTIES PREFIX "")

# .ini e sons ao lado da DLL (mesmo layout da pasta plugins do jogo)
add_custom_command(TARGET airbrake_audio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config/airbrake_audio.ini" "$<TARGET_FILE_DIR:airbrake_audio>/airbrake_audio.ini"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:airbrake_audio>/airbrake_audio"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/sounds/brake_loop.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/brake_loop_low.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/air_release.wav"
        "$<TARGET_FILE_DIR:airbrake_audio>/airbrake_audio/")

# ---------------------------------------------------------------- app externo (janela)
add_executable(airbrake_audio_app WIN32
    app/main.cpp
    app/ui.cpp
    app/model.cpp
    app/texts.cpp
    app/settings.cpp
    app/json.cpp
    app/app.rc
)
aa_target_defaults(airbrake_audio_app)
target_link_libraries(airbrake_audio_app PRIVATE aa_core user32 gdi32 gdiplus shell32 ole32 comctl32 dwmapi)
set_target_properties(airbrake_audio_app PROPERTIES OUTPUT_NAME "AirbrakeAudio")
if(MSVC)
    # O manifesto vem do app.rc (DPI por monitor, controles v6, UTF-8)
    target_link_options(airbrake_audio_app PRIVATE /MANIFEST:NO)
endif()

# ---------------------------------------------------------------- testes (sem o jogo)
enable_testing()
add_executable(aa_tests tests/tests.cpp app/json.cpp app/model.cpp app/texts.cpp)
aa_target_defaults(aa_tests)
target_link_libraries(aa_tests PRIVATE aa_core shell32 ole32)
add_test(NAME aa_tests COMMAND aa_tests)

# "Jogo falso": carrega a DLL real pelo SDK, simula o pedal e conversa como o app.
add_executable(fake_game tests/fake_game.cpp app/json.cpp)
aa_target_defaults(fake_game)
target_link_libraries(fake_game PRIVATE shell32 ole32)
add_dependencies(fake_game airbrake_audio)
add_test(NAME plugin_selftest COMMAND fake_game "$<TARGET_FILE:airbrake_audio>" --selftest)
