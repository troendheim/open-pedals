openpedals_add_plugin(
    TARGET OpenPedalsAmpSim
    PLUGIN_NAME "OP Amp Sim"
    PLUGIN_CODE AmSm
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
