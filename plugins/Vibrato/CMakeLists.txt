openpedals_add_plugin(
    TARGET OpenPedalsVibrato
    PLUGIN_NAME "OP Vibrato"
    PLUGIN_CODE Vibr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
