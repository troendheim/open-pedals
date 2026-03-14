openpedals_add_plugin(
    TARGET OpenPedalsOverdrive
    PLUGIN_NAME "OP Overdrive"
    PLUGIN_CODE Ovdr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
