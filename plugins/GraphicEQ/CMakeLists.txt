openpedals_add_plugin(
    TARGET OpenPedalsGraphicEQ
    PLUGIN_NAME "OP Graphic EQ"
    PLUGIN_CODE GrEQ
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
