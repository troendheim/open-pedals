openpedals_add_plugin(
    TARGET OpenPedalsCompressor
    PLUGIN_NAME "OP Compressor"
    PLUGIN_CODE Comp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
