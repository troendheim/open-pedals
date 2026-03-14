openpedals_add_plugin(
    TARGET OpenPedalsRingMod
    PLUGIN_NAME "OP Ring Mod"
    PLUGIN_CODE RngM
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
