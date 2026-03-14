bosslike_add_plugin(
    TARGET BosslikeReverb
    PLUGIN_NAME "Bosslike Reverb"
    PLUGIN_CODE Rvrb
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
