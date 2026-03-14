openpedals_add_plugin(
    TARGET OpenPedalsParametricEQ
    PLUGIN_NAME "OP Parametric EQ"
    PLUGIN_CODE PrEQ
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
