openpedals_add_plugin(
    TARGET OpenPedalsHumanizer
    PLUGIN_NAME "OP Humanizer"
    PLUGIN_CODE Hmzr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
