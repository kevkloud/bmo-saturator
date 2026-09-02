cmake_minimum_required(VERSION 3.22)
project(BMOSaturator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The DSP core has no JUCE dependency, so its tests and the measurement harness
# can be built and run on a machine with no audio framework at all -- including
# a CI container. Configure with -DBMO_DSP_ONLY=ON to skip the plugin target and
# everything JUCE brings with it.
option(BMO_DSP_ONLY "Build only the JUCE-free DSP tests and measurement tool" OFF)

if(NOT BMO_DSP_ONLY)
    # Audio Units are macOS-only; everywhere else it is VST3 plus the standalone.
    set(BMO_FORMATS VST3 Standalone)

    if(APPLE)
        list(APPEND BMO_FORMATS AU)

        # Dev builds are native arm64. For a release universal binary, configure
        # with -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
        if(NOT CMAKE_OSX_ARCHITECTURES)
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
        endif()

        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()

    # Only development builds install into the user's plugin folders. A Release
    # build is for packaging and must not clobber the binary you are testing with.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(BMO_INSTALL_AFTER_BUILD FALSE)
    else()
        set(BMO_INSTALL_AFTER_BUILD TRUE)
    endif()

    if(MSVC)
        # Static runtime, so the plugin does not depend on a redistributable being
        # installed on the listener's machine.
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    add_subdirectory(libs/JUCE)

    # The manufacturer code is shared with the rest of the suite and is an input
    # to the VST3 and AU identity: changing it would orphan every session that
    # references any LT3 Audio plugin. The plugin code is this product's alone
    # and is equally permanent. Neither may be edited -- see docs/plan.md.
    juce_add_plugin(BMOSaturator
        COMPANY_NAME                "LT3 Audio"
        BUNDLE_ID                   "com.lt3audio.bmosaturator"
        PLUGIN_MANUFACTURER_CODE    LT3a
        PLUGIN_CODE                 Bsat
        FORMATS                     ${BMO_FORMATS}
        PRODUCT_NAME                "BMO Saturator"
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD     ${BMO_INSTALL_AFTER_BUILD}
        VST3_CATEGORIES             "Fx" "Distortion"
        AU_MAIN_TYPE                kAudioUnitType_Effect)

    target_sources(BMOSaturator PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/params/ParameterLayout.cpp
        src/dsp/DspCore.cpp
        src/gui/Theme.cpp
        src/gui/LookAndFeel.cpp
        src/gui/Controls.cpp
        src/gui/PresetBar.cpp
        src/presets/PresetManager.cpp)

    target_include_directories(BMOSaturator PRIVATE src)

    target_compile_definitions(BMOSaturator PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0)

    target_link_libraries(BMOSaturator
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

#==============================================================================
# Tests. The DSP and parameter layer are deliberately host-agnostic, so they can
# be driven directly here without instantiating a plugin host.
#==============================================================================
enable_testing()

set(BMO_DSP_SOURCES src/dsp/DspCore.cpp)

# The DSP core is deliberately free of any JUCE dependency, so this target
# builds as plain C++ with no framework at all.
add_executable(BmoDspTests tests/DspTests.cpp ${BMO_DSP_SOURCES})
target_include_directories(BmoDspTests PRIVATE src)
add_test(NAME dsp COMMAND BmoDspTests)

# Offline measurement harness. Drives the DSP core with no host involved.
add_executable(measure tools/measure/main.cpp ${BMO_DSP_SOURCES})
target_include_directories(measure PRIVATE src)

if(NOT BMO_DSP_ONLY)
    foreach(target BmoDspTests measure)
        target_link_libraries(${target} PRIVATE
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
    endforeach()

    # A plain executable rather than juce_add_console_app: the latter forces
    # JUCE_STANDALONE_APPLICATION=1, which collides with the definitions the
    # plugin target exports.
    add_executable(BmoParameterTests tests/ParameterTests.cpp)
    target_include_directories(BmoParameterTests PRIVATE src)
    # The plugin links its JUCE modules PRIVATE, so they do not propagate through
    # the shared-code target; the test binary needs them named explicitly.
    target_link_libraries(BmoParameterTests PRIVATE
        BMOSaturator
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_events
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    add_test(NAME parameters COMMAND BmoParameterTests)
endif()
