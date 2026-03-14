openpedals_add_plugin(
    TARGET OpenPedalsHarmonizer
    PLUGIN_NAME "OP Harmonizer"
    PLUGIN_CODE Harm
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
