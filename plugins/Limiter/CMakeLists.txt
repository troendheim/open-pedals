openpedals_add_plugin(
    TARGET OpenPedalsLimiter
    PLUGIN_NAME "OP Limiter"
    PLUGIN_CODE Lmtr
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
