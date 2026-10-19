cmake_minimum_required(VERSION 3.22)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
project(MUCULATOR VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "$ENV{HOME}/Downloads/JUCE" CACHE PATH "Path to a local JUCE checkout")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    # No local JUCE (e.g. on Windows / CI): download it.
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# On macOS copy to ~/Library/Audio/Plug-Ins. On Windows the VST3 folder needs admin rights,
# so it is off by default there (enable with -DMUCULATOR_INSTALL_PLUGIN=ON from an admin shell).
if(APPLE)
    option(MUCULATOR_INSTALL_PLUGIN "Copy plugin to the system plug-in folder after build" ON)
    set(MUCULATOR_FORMATS AU VST3 Standalone)
else()
    option(MUCULATOR_INSTALL_PLUGIN "Copy plugin to the system plug-in folder after build" OFF)
    set(MUCULATOR_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Muculator
    COMPANY_NAME "Muculator"
    PLUGIN_MANUFACTURER_CODE Mucu
    PLUGIN_CODE Mclc
    FORMATS ${MUCULATOR_FORMATS}
    PRODUCT_NAME "Muculator"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${MUCULATOR_INSTALL_PLUGIN}
    VST3_CATEGORIES Fx Tools
    AU_MAIN_TYPE kAudioUnitType_Effect
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

set(MUCULATOR_UI_SOURCES
    Source/UI/ConsoleLookAndFeel.cpp
    Source/UI/Widgets.cpp
    Source/Modules/ModuleBase.cpp
    Source/Modules/ReverbModule.cpp
    Source/Modules/DelayModule.cpp
    Source/Modules/CompressorModule.cpp
    Source/Modules/PitchModule.cpp
    Source/Modules/ConverterModule.cpp
    Source/Modules/LoopModule.cpp)

target_sources(Muculator PRIVATE
    Source/Calc.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${MUCULATOR_UI_SOURCES})

target_include_directories(Muculator PRIVATE Source)

target_compile_definitions(Muculator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Muculator
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# ---- Pure-math unit tests (no JUCE) ----
add_executable(CalcTests Tests/CalcTests.cpp Source/Calc.cpp)
target_include_directories(CalcTests PRIVATE Source)

# ---- UI snapshot tool: renders every module to PNG for visual checks ----
juce_add_console_app(MuculatorSnapshot PRODUCT_NAME "MuculatorSnapshot")
target_sources(MuculatorSnapshot PRIVATE
    Tests/Snapshot.cpp
    Source/Calc.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${MUCULATOR_UI_SOURCES})
target_include_directories(MuculatorSnapshot PRIVATE Source)
target_compile_definitions(MuculatorSnapshot PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 MUCULATOR_SNAPSHOT=1
    JucePlugin_Name="Muculator")
target_link_libraries(MuculatorSnapshot PRIVATE
    juce::juce_audio_utils juce::juce_recommended_config_flags)
