openpedals_add_plugin(
    TARGET OpenPedalsAnalogDelay
    PLUGIN_NAME "OP Analog Delay"
    PLUGIN_CODE ADly
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
