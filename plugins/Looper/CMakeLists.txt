openpedals_add_plugin(
    TARGET OpenPedalsLooper
    PLUGIN_NAME "OP Looper"
    PLUGIN_CODE Loop
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
