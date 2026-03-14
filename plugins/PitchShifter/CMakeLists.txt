openpedals_add_plugin(
    TARGET OpenPedalsPitchShifter
    PLUGIN_NAME "OP Pitch Shifter"
    PLUGIN_CODE PtSh
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
