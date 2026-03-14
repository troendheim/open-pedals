openpedals_add_plugin(
    TARGET OpenPedalsAutoWah
    PLUGIN_NAME "OP Auto-Wah"
    PLUGIN_CODE AWah
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
