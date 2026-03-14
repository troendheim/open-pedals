bosslike_add_plugin(
    TARGET BosslikeDelay
    PLUGIN_NAME "Bosslike Delay"
    PLUGIN_CODE DDly
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
