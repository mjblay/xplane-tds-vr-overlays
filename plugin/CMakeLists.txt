cmake_minimum_required(VERSION 3.20)

project(XPTOOverlayTuner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XPlaneSdk)

add_library(xpto_overlay_tuner MODULE
    src/xpto_menu.cpp
    src/xpto_object_instance.cpp
    src/xpto_plugin.cpp
    src/xpto_window.cpp
)

target_include_directories(xpto_overlay_tuner
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${XPLANE_SDK_INCLUDE_DIRS}
)

target_compile_definitions(xpto_overlay_tuner
    PRIVATE
        IBM=1
        XPLM200=1
        XPLM210=1
        XPLM300=1
        XPLM301=1
        XPLM303=1
        XPLM400=1
)

target_link_libraries(xpto_overlay_tuner
    PRIVATE
        ${XPLANE_SDK_XPLM_LIBRARY}
)

set_target_properties(xpto_overlay_tuner PROPERTIES
    PREFIX ""
    OUTPUT_NAME "XPTO"
    SUFFIX ".xpl"
)

if(WIN32)
    set_target_properties(xpto_overlay_tuner PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/XPTO/win_x64"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/XPTO/win_x64"
    )
endif()

add_custom_command(TARGET xpto_overlay_tuner POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:xpto_overlay_tuner>/../assets"
    COMMENT "Staging XPTO plugin assets"
)