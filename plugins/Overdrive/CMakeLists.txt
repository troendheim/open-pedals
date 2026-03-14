bosslike_add_plugin(
    TARGET BosslikeOverdrive
    PLUGIN_NAME "Bosslike Overdrive"
    PLUGIN_CODE Ovdr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
