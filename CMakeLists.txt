cmake_minimum_required(VERSION 3.22)

project(BosslikeVSTS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal Binary for Apple Silicon + Intel
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

# Add JUCE
add_subdirectory(JUCE)

# Shared DSP library (header-only)
add_library(BosslikeDSP INTERFACE)
target_include_directories(BosslikeDSP INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)

# Helper function to create a plugin with standard settings
function(bosslike_add_plugin)
    set(oneValueArgs TARGET PLUGIN_NAME PLUGIN_CODE DESCRIPTION)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(PLUGIN "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    juce_add_plugin(${PLUGIN_TARGET}
        COMPANY_NAME "Bosslike"
        PLUGIN_MANUFACTURER_CODE BsLk
        PLUGIN_CODE ${PLUGIN_PLUGIN_CODE}
        FORMATS AU
        PRODUCT_NAME "${PLUGIN_PLUGIN_NAME}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
    )

    target_sources(${PLUGIN_TARGET} PRIVATE ${PLUGIN_SOURCES})

    target_compile_definitions(${PLUGIN_TARGET} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
    )

    target_link_libraries(${PLUGIN_TARGET}
        PRIVATE
            BosslikeDSP
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
endfunction()

# Add all plugins
add_subdirectory(plugins/Overdrive)
add_subdirectory(plugins/Delay)
add_subdirectory(plugins/Chorus)
add_subdirectory(plugins/Reverb)
add_subdirectory(plugins/Compressor)
