openpedals_add_plugin(
    TARGET OpenPedalsOctaver
    PLUGIN_NAME "OP Octaver"
    PLUGIN_CODE Octv
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
