cmake_minimum_required(VERSION 3.16)
project(Reversed_Game LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The reconstructed game currently has only a Linux platform backend")
endif()

set(SOFT3D_LOG_BACKEND "NULL" CACHE STRING "Logging backend: NULL or STDIO")
set_property(CACHE SOFT3D_LOG_BACKEND PROPERTY STRINGS NULL STDIO)
string(TOUPPER "${SOFT3D_LOG_BACKEND}" SOFT3D_LOG_BACKEND)

add_library(soft3d
    src/platform/dummy/DummyAudioBackend.cpp
    src/platform/dummy/DummyVideoBackend.cpp
    src/platform/linux/File.cpp
    src/platform/linux/StdioLog.cpp
    src/platform/linux/SystemClock.cpp
    src/soft3d/Animation.cpp
    src/soft3d/AudioData.cpp
    src/soft3d/AudioDevice.cpp
    src/soft3d/AudioManager.cpp
    src/soft3d/BasicShader.cpp
    src/soft3d/BezierCurve3.cpp
    src/soft3d/Bsp.cpp
    src/soft3d/BspRender.cpp
    src/soft3d/Camera.cpp
    src/soft3d/CameraSpot.cpp
    src/soft3d/Dialog.cpp
    src/soft3d/Engine.cpp
    src/soft3d/Font.cpp
    src/soft3d/GameEngine.cpp
    src/soft3d/Input.cpp
    src/soft3d/MenuBackdrop.cpp
    src/soft3d/Object.cpp
    src/soft3d/ObjectRes.cpp
    src/soft3d/Raster.cpp
    src/soft3d/Renderable.cpp
    src/soft3d/Renderer.cpp
    src/soft3d/Resource.cpp
    src/soft3d/ScreenBlitter.cpp
    src/soft3d/SoundInstance.cpp
    src/soft3d/Texture.cpp
    src/soft3d/TextureRes.cpp
    src/soft3d/UiManager.cpp
    src/soft3d/VID_impl.cpp
    src/soft3d/s3dBase.cpp
    src/soft3d/s3dFilePak.cpp
    src/soft3d/s3dIO.cpp
    src/soft3d/s3dLog.cpp
    src/soft3d/s3dResManager.cpp
)
target_include_directories(soft3d PUBLIC src)
target_compile_features(soft3d PUBLIC cxx_std_17)
if(SOFT3D_LOG_BACKEND STREQUAL "STDIO")
    target_compile_definitions(soft3d PUBLIC SOFT3D_LOG_BACKEND_STDIO=1)
elseif(NOT SOFT3D_LOG_BACKEND STREQUAL "NULL")
    message(FATAL_ERROR "SOFT3D_LOG_BACKEND must be NULL or STDIO")
endif()
option(SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER
    "Decode high-nibble-first WAR audio instead of low-nibble-first SAU audio" OFF)
if(SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER)
    target_compile_definitions(soft3d PUBLIC SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER)
endif()
option(SOFT3D_SPK_ENGLISH_LAYOUT
    "Use 32-byte-path English SPK entries instead of 64-byte-path Chinese entries" OFF)
if(SOFT3D_SPK_ENGLISH_LAYOUT)
    target_compile_definitions(soft3d PUBLIC SOFT3D_SPK_ENGLISH_LAYOUT=1)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(soft3d PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(SOFT3D_BUILD_APP "Build the SDL3 application shell" ON)
if(SOFT3D_BUILD_APP)
    find_package(SDL3 REQUIRED CONFIG)
    add_executable(seven_days
        src/app/Application.cpp
        src/app/main.cpp
        src/platform/sdl/SdlAudioBackend.cpp
        src/platform/sdl/SdlClock.cpp
        src/platform/sdl/SdlInput.cpp
        src/platform/sdl/SdlVideoBackend.cpp
    )
    target_include_directories(seven_days PRIVATE src)
    target_compile_features(seven_days PRIVATE cxx_std_17)
    target_link_libraries(seven_days PRIVATE soft3d SDL3::SDL3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(seven_days PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(soft3d_filepak_tests tests/test_s3dfilepak.cpp)
    target_link_libraries(soft3d_filepak_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_filepak_tests PRIVATE
        SOFT3D_TEST_ARCHIVE="${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_filepak_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_filepak COMMAND soft3d_filepak_tests)

    add_executable(soft3d_resmanager_tests tests/test_s3dresmanager.cpp)
    target_link_libraries(soft3d_resmanager_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_resmanager_tests PRIVATE
        SOFT3D_TEST_ARCHIVE="${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_resmanager_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_resmanager COMMAND soft3d_resmanager_tests)

    add_executable(soft3d_io_tests tests/test_s3dio.cpp)
    target_link_libraries(soft3d_io_tests PRIVATE soft3d)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_io_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_io COMMAND soft3d_io_tests)

    add_executable(soft3d_log_tests tests/test_s3dlog.cpp)
    target_link_libraries(soft3d_log_tests PRIVATE soft3d)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_log_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_log COMMAND soft3d_log_tests)

    add_executable(soft3d_vidimpl_tests tests/test_vidimpl.cpp)
    target_link_libraries(soft3d_vidimpl_tests PRIVATE soft3d)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_vidimpl_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_vidimpl COMMAND soft3d_vidimpl_tests)

    add_executable(soft3d_engine_tests tests/test_engine.cpp)
    target_link_libraries(soft3d_engine_tests PRIVATE soft3d)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_engine COMMAND soft3d_engine_tests)

    add_executable(soft3d_input_tests tests/test_input.cpp)
    target_link_libraries(soft3d_input_tests PRIVATE soft3d)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_input_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_input COMMAND soft3d_input_tests)

    add_executable(soft3d_bsp_tests tests/test_bsp.cpp)
    target_link_libraries(soft3d_bsp_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_bsp_tests PRIVATE
        SOFT3D_TEST_ARCHIVE=u"${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_bsp_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_bsp COMMAND soft3d_bsp_tests)

    add_executable(soft3d_bsp_render_tests tests/test_bsprender.cpp)
    target_link_libraries(soft3d_bsp_render_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_bsp_render_tests PRIVATE
        SOFT3D_TEST_ARCHIVE=u"${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_bsp_render_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_bsp_render COMMAND soft3d_bsp_render_tests)

    add_executable(soft3d_spl_tests tests/test_spl.cpp)
    target_link_libraries(soft3d_spl_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_spl_tests PRIVATE
        SOFT3D_TEST_ARCHIVE=u"${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_spl_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_spl COMMAND soft3d_spl_tests)

    add_executable(soft3d_font_tests tests/test_font.cpp)
    target_link_libraries(soft3d_font_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_font_tests PRIVATE
        SOFT3D_TEST_ARCHIVE=u"${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_font_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_font COMMAND soft3d_font_tests)

    add_executable(soft3d_ui_tests tests/test_ui.cpp)
    target_link_libraries(soft3d_ui_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_ui_tests PRIVATE
        SOFT3D_TEST_ARCHIVE=u"${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_ui_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_ui COMMAND soft3d_ui_tests)

    add_executable(soft3d_audio_tests tests/test_audio.cpp)
    target_link_libraries(soft3d_audio_tests PRIVATE soft3d)
    target_compile_definitions(soft3d_audio_tests PRIVATE
        SOFT3D_TEST_ARCHIVE="${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res.spk"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(soft3d_audio_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME soft3d_audio COMMAND soft3d_audio_tests)

    if(TARGET seven_days)
        add_executable(soft3d_sdl_input_tests
            tests/test_sdlinput.cpp
            src/platform/sdl/SdlInput.cpp
        )
        target_link_libraries(soft3d_sdl_input_tests PRIVATE soft3d SDL3::SDL3)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(soft3d_sdl_input_tests PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        add_test(NAME soft3d_sdl_input COMMAND soft3d_sdl_input_tests)

        add_test(NAME seven_days_smoke
            COMMAND seven_days --frames 3 --renderer software --no-audio)
        set_tests_properties(seven_days_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy")
        add_test(NAME seven_days_audio_smoke
            COMMAND seven_days --frames 3 --renderer software)
        set_tests_properties(seven_days_audio_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endif()
endif()
