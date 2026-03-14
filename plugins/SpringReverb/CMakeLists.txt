openpedals_add_plugin(
    TARGET OpenPedalsSpringReverb
    PLUGIN_NAME "OP Spring Reverb"
    PLUGIN_CODE SRvb
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
