openpedals_add_plugin(
    TARGET OpenPedalsReverb
    PLUGIN_NAME "OpenPedals Reverb"
    PLUGIN_CODE Rvrb
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
