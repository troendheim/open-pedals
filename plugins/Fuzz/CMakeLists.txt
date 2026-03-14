openpedals_add_plugin(
    TARGET OpenPedalsFuzz
    PLUGIN_NAME "OP Fuzz"
    PLUGIN_CODE Fuzz
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
