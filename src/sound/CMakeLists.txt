#
# 86Box    A hypervisor and IBM PC system emulator that specializes in
#          running old operating systems and software designed for IBM
#          PC systems and compatibles from 1981 through fairly recent
#          system designs based on the PCI bus.
#
#          This file is part of the 86Box distribution.
#
#          CMake build script.
#
# Authors: David Hrdlička, <hrdlickadavid@outlook.com>
#          Jasmine Iwanek, <jriwanek@gmail.com>
#
#          Copyright 2020-2021 David Hrdlička.
#          Copyright 2024      Jasmine Iwanek.
#

add_library(snd OBJECT
    sound.c
    snd_opl.c
    snd_opl_nuked.c
    snd_opl_ymfm.cpp
    snd_resid.cpp
    midi.c
    snd_speaker.c
    snd_pssj.c
    snd_lpt_dac.c
    snd_ac97_codec.c
    snd_ac97_via.c
    snd_lpt_dss.c
    snd_ps1.c
    snd_adlib.c
    snd_adlibgold.c
    snd_ad1848.c
    snd_audiopci.c
    snd_azt2316a.c
    snd_cms.c
    snd_cmi8x38.c
    snd_covox.c
    snd_cs423x.c
    snd_gus.c
    snd_sb.c
    snd_sb_dsp.c
    snd_emu8k.c
    snd_mmb.c
    snd_mpu401.c
    snd_pas16.c
    snd_sn76489.c
    snd_ssi2001.c
    snd_wss.c
    snd_ym7128.c
    snd_optimc.c
    snd_opl_esfm.c
)

# TODO: Should platform-specific audio driver be here?
if(AUDIO4)
    target_sources(snd PRIVATE audio4.c)
elseif(SNDIO)
    target_sources(snd PRIVATE sndio.c)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(SNDIO IMPORTED_TARGET sndio)
    if(SNDIO_FOUND)
        target_link_libraries(86Box PkgConfig::SNDIO)
    else()
        find_path(SNDIO_INCLUDE_DIR NAMES "sndio.h")
        find_library(SNDIO_LIBRARY sndio)

        target_link_libraries(86Box ${SNDIO_LIBRARY})
    endif()

    include_directories(${SNDIO_INCLUDE_DIRS})
elseif(OPENAL)
    if(VCPKG_TOOLCHAIN)
        find_package(OpenAL CONFIG REQUIRED)
    elseif(MINGW)
        find_package(OpenAL MODULE REQUIRED)
    else()
        find_package(OpenAL REQUIRED)
    endif()

    if(TARGET OpenAL::OpenAL)
        target_link_libraries(86Box OpenAL::OpenAL)
        if(WIN32 AND STATIC_BUILD)
            target_link_libraries(OpenAL::OpenAL INTERFACE avrt)
        endif()
    else()
        target_link_libraries(86Box ${OPENAL_LIBRARY})
        if(WIN32 AND STATIC_BUILD)
            target_link_libraries(${OPENAL_LIBRARY} INTERFACE avrt)
        endif()
    endif()

    include_directories(${OPENAL_INCLUDE_DIR})

    target_sources(snd PRIVATE openal.c)
else()
    if(WIN32)
        option(FAUDIO "Use FAudio instead of XAudio2" OFF)
    endif()

    target_sources(snd PRIVATE xaudio2.c)

    if(NOT WIN32 OR FAUDIO)
        find_package(PkgConfig REQUIRED)

        # Use FAudio, a reimplementation of XAudio2
        pkg_check_modules(FAUDIO IMPORTED_TARGET FAudio)
        if(FAUDIO_FOUND)
            target_link_libraries(86Box PkgConfig::FAUDIO)
        else()
            find_path(FAUDIO_INCLUDE_DIR NAMES "FAudio.h")
            find_library(FAUDIO_LIBRARY FAudio)

            target_link_libraries(86Box ${FAUDIO_LIBRARY})
        endif()

        include_directories(${FAUDIO_INCLUDE_DIRS})

        set_property(SOURCE xaudio2.c PROPERTY COMPILE_DEFINITIONS USE_FAUDIO)
    endif()
endif()

if(RTMIDI)
    if(VCPKG_TOOLCHAIN)
        # vcpkg includes a config file for rtmidi
        find_package(RtMidi REQUIRED)
        target_link_libraries(86Box RtMidi::rtmidi)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(RTMIDI REQUIRED IMPORTED_TARGET rtmidi)
        target_link_libraries(86Box PkgConfig::RTMIDI)

        if(WIN32)
            target_link_libraries(PkgConfig::RTMIDI INTERFACE winmm)
        endif()
     endif()

     target_compile_definitions(snd PRIVATE USE_RTMIDI)
     target_sources(snd PRIVATE midi_rtmidi.cpp)
endif()

if(FLUIDSYNTH)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FLUIDSYNTH REQUIRED IMPORTED_TARGET fluidsynth)
    target_link_libraries(86Box PkgConfig::FLUIDSYNTH)
    if(STATIC_BUILD)
        target_link_libraries(86Box -static ${FLUIDSYNTH_STATIC_LIBRARIES} -fopenmp)
        if(WIN32)
            add_compile_definitions(FLUIDSYNTH_NOT_A_DLL)

            if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
                target_link_libraries(86Box psapi)
            endif()
        endif()
    endif()

    target_compile_definitions(snd PRIVATE USE_FLUIDSYNTH)
    target_sources(snd PRIVATE midi_fluidsynth.c)
endif()

if(MUNT)
    target_compile_definitions(snd PRIVATE USE_MUNT)
    target_sources(snd PRIVATE midi_mt32.c)

    option(MUNT_EXTERNAL "Link against the system-provided MUNT library" OFF)
    mark_as_advanced(MUNT_EXTERNAL)

    if(MUNT_EXTERNAL)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(MT32EMU REQUIRED IMPORTED_TARGET mt32emu)
        target_link_libraries(86Box PkgConfig::MT32EMU)
    else()
        add_subdirectory(munt)
        target_link_libraries(86Box mt32emu)
    endif()
endif()

add_subdirectory(ayumi)
target_link_libraries(86Box ayumi)

add_subdirectory(esfmu)
target_link_libraries(86Box esfmu)

add_subdirectory(ymfm)
target_link_libraries(86Box ymfm)

if(OPL4ML)
    target_compile_definitions(snd PRIVATE USE_OPL4ML)
    target_sources(snd PRIVATE midi_opl4.c midi_opl4_yrw801.c)
endif()
 
find_package(PkgConfig)
pkg_check_modules(SERIALPORT libserialport)

if(SERIALPORT_FOUND OR DEFINED LIBSERIALPORT_ROOT)
    add_compile_definitions(USE_LIBSERIALPORT=1)

    if(APPLE)
        include_directories(${LIBSERIALPORT_ROOT}/include)
        target_link_libraries(86Box ${LIBSERIALPORT_ROOT}/lib/libserialport.dylib)
    elseif(WIN32)
        include_directories(${SERIALPORT_INCLUDE_DIRS})
        target_link_libraries(86Box ${SERIALPORT_LIBRARIES} SetupAPI)
    else()
        include_directories(${SERIALPORT_INCLUDE_DIRS})
        target_link_libraries(86Box ${SERIALPORT_LIBRARIES})
    endif()
    target_sources(snd PRIVATE
    snd_opl2board.c
    snd_opl_opl2board.cpp
)
endif()

add_subdirectory(resid-fp)
target_link_libraries(86Box resid-fp)
