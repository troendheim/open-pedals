openpedals_add_plugin(
    TARGET OpenPedalsDistortion
    PLUGIN_NAME "OP Distortion"
    PLUGIN_CODE Dist
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
