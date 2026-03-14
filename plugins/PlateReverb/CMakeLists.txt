openpedals_add_plugin(
    TARGET OpenPedalsPlateReverb
    PLUGIN_NAME "OP Plate Reverb"
    PLUGIN_CODE PRvb
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
