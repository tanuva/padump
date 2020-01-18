cmake_minimum_required (VERSION 3.0)

project (padump)
set (CMAKE_CXX_STANDARD 14)

add_executable (padump
    src/main.cpp
)

#target_compile_options(groggle PUBLIC "-pthread")
set_property(TARGET padump
    APPEND PROPERTY
    LINK_FLAGS "-pthread"
)

# Dependencies
# ============

find_package(PkgConfig REQUIRED)

find_path(PULSEAUDIO_INCLUDE_DIR
    NAMES pulse/pulseaudio.h
    DOC "The PulseAudio include directory"
)

find_library(PULSEAUDIO_LIBRARY
    NAMES pulse
    DOC "The PulseAudio library"
)

target_include_directories(padump PUBLIC ${PULSEAUDIO_INCLUDE_DIR})
target_link_libraries(padump ${PULSEAUDIO_LIBRARY})

