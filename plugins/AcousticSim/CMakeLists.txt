openpedals_add_plugin(
    TARGET OpenPedalsAcousticSim
    PLUGIN_NAME "OP Acoustic Sim"
    PLUGIN_CODE AcSm
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
