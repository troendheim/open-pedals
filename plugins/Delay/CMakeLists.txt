openpedals_add_plugin(
    TARGET OpenPedalsDelay
    PLUGIN_NAME "Delay"
    PLUGIN_CODE DDly
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
