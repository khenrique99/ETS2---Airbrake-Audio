cmake_minimum_required(VERSION 3.20)
project(airbrake_audio CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")   # static CRT: no vcruntime dependency

add_library(airbrake_audio SHARED src/plugin.cpp)
target_compile_definitions(airbrake_audio PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(airbrake_audio PRIVATE xaudio2 ole32 ws2_32 shell32 user32)
set_target_properties(airbrake_audio PROPERTIES PREFIX "")
