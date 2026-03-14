openpedals_add_plugin(
    TARGET OpenPedalsChorus
    PLUGIN_NAME "OpenPedals Chorus"
    PLUGIN_CODE Chrs
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
