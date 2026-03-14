openpedals_add_plugin(
    TARGET OpenPedalsWah
    PLUGIN_NAME "OP Wah"
    PLUGIN_CODE Wahh
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
