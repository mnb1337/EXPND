cmake_minimum_required(VERSION 3.22)
project(EXPND VERSION 1.0.0)

add_subdirectory("C:/JUCE" JUCE_build)

juce_add_plugin(EXPND
    COMPANY_NAME "MNB"
    PLUGIN_MANUFACTURER_CODE 1337
    PLUGIN_CODE Expn
    FORMATS VST3
    PRODUCT_NAME "EXPND")

target_sources(EXPND PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(EXPND PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EXPND PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)