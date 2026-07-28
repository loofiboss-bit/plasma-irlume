# SPDX-License-Identifier: BSD-2-Clause

add_library(irlume_backend STATIC
    authactionrunner.cpp
    authactionrunner.h
    authconfiguration.cpp
    authconfiguration.h
    camerapreviewsession.cpp
    camerapreviewsession.h
    fakeadapter.cpp
    fakeadapter.h
    profilemodel.cpp
    profilemodel.h
    refreshcoordinator.cpp
    refreshcoordinator.h
    supportreport.cpp
    supportreport.h
    systemstate.cpp
    systemstate.h
    systemprobe.cpp
    systemprobe.h
)

set_target_properties(irlume_backend PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(irlume_backend
    PUBLIC
        irlume_adapter
        preview_protocol
        Qt6::Concurrent
        Qt6::Core
        Qt6::Gui
)
target_include_directories(irlume_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(irlume_backend PRIVATE
    PLASMA_IRLUME_PREVIEW_WORKER_PATH="${CMAKE_INSTALL_PREFIX}/libexec/plasma-irlume-camera-preview-worker"
)
