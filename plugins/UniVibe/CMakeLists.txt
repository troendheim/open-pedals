openpedals_add_plugin(
    TARGET OpenPedalsUniVibe
    PLUGIN_NAME "OP Uni-Vibe"
    PLUGIN_CODE UVib
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.h
)
