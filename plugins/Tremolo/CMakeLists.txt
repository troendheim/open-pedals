openpedals_add_plugin(
    TARGET OpenPedalsTremolo
    PLUGIN_NAME "OP Tremolo"
    PLUGIN_CODE Trem
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
