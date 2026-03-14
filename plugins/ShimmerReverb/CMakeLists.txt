openpedals_add_plugin(
    TARGET OpenPedalsShimmerReverb
    PLUGIN_NAME "OP Shimmer Reverb"
    PLUGIN_CODE ShRv
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
