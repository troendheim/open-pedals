openpedals_add_plugin(
    TARGET OpenPedalsPhaser
    PLUGIN_NAME "OP Phaser"
    PLUGIN_CODE Phsr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
