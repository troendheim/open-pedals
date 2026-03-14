openpedals_add_plugin(
    TARGET OpenPedalsNoiseGate
    PLUGIN_NAME "OP Noise Gate"
    PLUGIN_CODE NGte
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
