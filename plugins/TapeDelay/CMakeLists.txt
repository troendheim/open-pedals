openpedals_add_plugin(
    TARGET OpenPedalsTapeDelay
    PLUGIN_NAME "OP Tape Delay"
    PLUGIN_CODE TDly
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
