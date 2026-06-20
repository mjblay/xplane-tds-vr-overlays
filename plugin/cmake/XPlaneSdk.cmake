if(NOT XPLANE_SDK_ROOT)
    if(DEFINED ENV{XPLANE_SDK_ROOT})
        set(XPLANE_SDK_ROOT "$ENV{XPLANE_SDK_ROOT}")
    else()
        message(FATAL_ERROR "Set XPLANE_SDK_ROOT to the extracted X-Plane SDK root containing CHeaders/ and Libraries/.")
    endif()
endif()

cmake_path(ABSOLUTE_PATH XPLANE_SDK_ROOT NORMALIZE)

set(XPLANE_SDK_XPLM_INCLUDE_DIR "${XPLANE_SDK_ROOT}/CHeaders/XPLM")
set(XPLANE_SDK_WIDGETS_INCLUDE_DIR "${XPLANE_SDK_ROOT}/CHeaders/Widgets")

if(NOT EXISTS "${XPLANE_SDK_XPLM_INCLUDE_DIR}/XPLMPlugin.h")
    message(FATAL_ERROR "XPLM headers not found at ${XPLANE_SDK_XPLM_INCLUDE_DIR}. Check XPLANE_SDK_ROOT.")
endif()

if(NOT EXISTS "${XPLANE_SDK_WIDGETS_INCLUDE_DIR}")
    message(FATAL_ERROR "Widgets headers folder not found at ${XPLANE_SDK_WIDGETS_INCLUDE_DIR}. Check XPLANE_SDK_ROOT.")
endif()

set(XPLANE_SDK_INCLUDE_DIRS
    "${XPLANE_SDK_XPLM_INCLUDE_DIR}"
    "${XPLANE_SDK_WIDGETS_INCLUDE_DIR}"
)

if(WIN32)
    set(XPLANE_SDK_XPLM_LIBRARY "${XPLANE_SDK_ROOT}/Libraries/Win/XPLM_64.lib")

    if(NOT EXISTS "${XPLANE_SDK_XPLM_LIBRARY}")
        message(FATAL_ERROR "Windows XPLM import library not found at ${XPLANE_SDK_XPLM_LIBRARY}. Check XPLANE_SDK_ROOT.")
    endif()
else()
    message(FATAL_ERROR "Only the Windows X-Plane SDK link path is configured in this skeleton.")
endif()
