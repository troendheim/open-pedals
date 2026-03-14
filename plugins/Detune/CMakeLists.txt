openpedals_add_plugin(
    TARGET OpenPedalsDetune
    PLUGIN_NAME "OP Detune"
    PLUGIN_CODE Dtne
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
