openpedals_add_plugin(
    TARGET OpenPedalsReverseDelay
    PLUGIN_NAME "OP Reverse Delay"
    PLUGIN_CODE RDly
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
