cmake_minimum_required(VERSION 3.16)
project(Reversed_Game LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The reconstructed game currently has only a Linux platform backend")
endif()

set(SOFT3D_LOG_BACKEND "NULL" CACHE STRING "Logging backend: NULL or STDIO")
set_property(CACHE SOFT3D_LOG_BACKEND PROPERTY STRINGS NULL STDIO)
string(TOUPPER "${SOFT3D_LOG_BACKEND}" SOFT3D_LOG_BACKEND)

add_library(soft3d
    src/platform/linux/File.cpp
    src/platform/linux/StdioLog.cpp
    src/soft3d/Animation.cpp
    src/soft3d/AudioData.cpp
    src/soft3d/ObjectRes.cpp
    src/soft3d/Resource.cpp
    src/soft3d/Texture.cpp
    src/soft3d/TextureRes.cpp
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

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(soft3d PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(SOFT3D_BUILD_APP "Build the SDL3 application shell" ON)
if(SOFT3D_BUILD_APP)
    find_package(SDL3 REQUIRED CONFIG)
    add_executable(seven_days
        src/app/Application.cpp
        src/app/EngineStubs.cpp
        src/app/main.cpp
    )
    target_include_directories(seven_days PRIVATE src)
    target_compile_features(seven_days PRIVATE cxx_std_17)
    target_link_libraries(seven_days PRIVATE SDL3::SDL3)
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

    if(TARGET seven_days)
        add_test(NAME seven_days_smoke
            COMMAND seven_days --frames 3 --renderer software --no-audio)
        set_tests_properties(seven_days_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()
endif()
