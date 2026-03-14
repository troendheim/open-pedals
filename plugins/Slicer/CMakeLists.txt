openpedals_add_plugin(
    TARGET OpenPedalsSlicer
    PLUGIN_NAME "OP Slicer"
    PLUGIN_CODE Slcr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
