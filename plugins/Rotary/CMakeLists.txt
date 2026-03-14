openpedals_add_plugin(
    TARGET OpenPedalsRotary
    PLUGIN_NAME "OP Rotary"
    PLUGIN_CODE Rtry
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
