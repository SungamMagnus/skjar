cmake_minimum_required(VERSION 3.22)

# Must precede project(): these drive the compiler checks. Universal binary,
# for the same reason as the other Sungam plug-ins — left unset, an Intel CMake
# under Rosetta silently builds an x86_64-only plug-in that arm64 hosts skip.
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
            "macOS build architectures" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
            "Minimum macOS version" FORCE)
    endif()
endif()

project(Skjar VERSION 0.1.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "/Applications/JUCE" CACHE PATH "Path to a JUCE source checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR} — pass -DJUCE_DIR=/path/to/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

set(SKJAR_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp
    Source/Sungam.cpp
    Source/YouTube.cpp
    Source/VideoView.mm)

juce_add_plugin(Skjar
    COMPANY_NAME            "Sungam"
    BUNDLE_ID               "com.sungam.skjar"
    PLUGIN_MANUFACTURER_CODE Sngm
    PLUGIN_CODE             Skjr
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "Skjar"
    # Bundles and files keep the plain spelling; hosts and the panel show this.
    PLUGIN_NAME             "Skjár"
    DESCRIPTION             "A YouTube video that follows the transport"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES         Fx Tools
    AU_MAIN_TYPE            kAudioUnitType_Effect)

target_sources(Skjar PRIVATE ${SKJAR_SOURCES})

# The player is our own WKWebView rather than juce::WebBrowserComponent: the
# YouTube embed refuses to play (error 153) without an origin, and only
# loadHTMLString:baseURL: lets us give the page one.
target_compile_definitions(Skjar PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0)

target_include_directories(Skjar PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(Skjar
    PRIVATE
        juce::juce_audio_utils
        "-framework WebKit"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Dev tools: parse URLs and drive the player page headless, and render the
# editor to a PNG. -DSKJAR_TOOLS=ON
option(SKJAR_TOOLS "Build the URL/player checker and the panel renderer" OFF)
if(SKJAR_TOOLS)
    juce_add_console_app(player_check PRODUCT_NAME "player_check" NEEDS_CURL FALSE)
    target_sources(player_check PRIVATE tools/player_check.mm Source/YouTube.cpp Source/VideoView.mm)
    target_compile_definitions(player_check PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_include_directories(player_check PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_link_libraries(player_check PRIVATE juce::juce_gui_extra "-framework WebKit")

    juce_add_console_app(panel_shot PRODUCT_NAME "panel_shot" NEEDS_CURL FALSE)
    target_sources(panel_shot PRIVATE tools/panel_shot.cpp ${SKJAR_SOURCES})
    target_compile_definitions(panel_shot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Skjár"
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_include_directories(panel_shot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_link_libraries(panel_shot PRIVATE juce::juce_audio_utils "-framework WebKit")
endif()
