bosslike_add_plugin(
    TARGET BosslikeCompressor
    PLUGIN_NAME "Bosslike Compressor"
    PLUGIN_CODE Comp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
