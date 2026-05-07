cmake_minimum_required(VERSION 3.22)

project(HyperSonicAnalyzer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# JUCE を FetchContent で取得
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)

FetchContent_MakeAvailable(JUCE)

# プラグインの定義
juce_add_plugin(HyperSonicAnalyzer
    COMPANY_NAME "Groundless Electronics"
    COMPANY_WEBSITE "https://groundlesselectronics.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Grls
    PLUGIN_CODE Hysa
    FORMATS VST3 Standalone
    PRODUCT_NAME "HyperSonic Analyzer"
)

# ソースファイル
target_sources(HyperSonicAnalyzer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SpectralAnalyzer.cpp
        Source/SpectralAnalyzer.h
)

# JUCE モジュール
target_compile_definitions(HyperSonicAnalyzer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(HyperSonicAnalyzer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Additional strict warnings for our source files
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(HyperSonicAnalyzer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )
endif()
