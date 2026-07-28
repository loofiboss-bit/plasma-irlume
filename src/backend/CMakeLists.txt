# SPDX-License-Identifier: BSD-2-Clause

add_library(kfaceauth_core STATIC
    camerapreviewsession.cpp
    camerapreviewsession.h
    faceauthbackend.h
    nativefaceauthbackend.cpp
    nativefaceauthbackend.h
    refreshcoordinator.cpp
    refreshcoordinator.h
    supportreport.cpp
    supportreport.h
    systemstate.cpp
    systemstate.h
    systemprobe.cpp
    systemprobe.h
    visionanalysissession.cpp
    visionanalysissession.h
)

set_target_properties(kfaceauth_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(kfaceauth_core
    PUBLIC
        preview_protocol
        Qt6::Concurrent
        Qt6::Core
        Qt6::Gui
)
target_include_directories(kfaceauth_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(kfaceauth_core PRIVATE
    KFACEAUTH_PREVIEW_WORKER_PATH="${CMAKE_INSTALL_PREFIX}/libexec/${KFACEAUTH_PREVIEW_WORKER}"
    KFACEAUTH_VISION_WORKER_PATH="${CMAKE_INSTALL_PREFIX}/libexec/${KFACEAUTH_VISION_WORKER}"
    KFACEAUTH_MODEL_ROOT="${CMAKE_INSTALL_PREFIX}/share/${KFACEAUTH_MODEL_DIRECTORY}"
    KFACEAUTH_SUPPORT_PREFIX="${KFACEAUTH_SUPPORT_PREFIX}"
)
