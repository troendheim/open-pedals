bosslike_add_plugin(
    TARGET BosslikeChorus
    PLUGIN_NAME "Bosslike Chorus"
    PLUGIN_CODE Chrs
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
