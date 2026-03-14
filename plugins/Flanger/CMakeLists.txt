openpedals_add_plugin(
    TARGET OpenPedalsFlanger
    PLUGIN_NAME "OP Flanger"
    PLUGIN_CODE Flgr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
