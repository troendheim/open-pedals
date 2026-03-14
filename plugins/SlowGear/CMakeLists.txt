openpedals_add_plugin(
    TARGET OpenPedalsSlowGear
    PLUGIN_NAME "OP Slow Gear"
    PLUGIN_CODE SlGr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
