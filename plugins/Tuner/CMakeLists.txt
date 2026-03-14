openpedals_add_plugin(
    TARGET OpenPedalsTuner
    PLUGIN_NAME "OP Tuner"
    PLUGIN_CODE Tunr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
