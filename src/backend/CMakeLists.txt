# SPDX-License-Identifier: BSD-2-Clause

add_library(irlume_backend STATIC
    authactionrunner.cpp
    authactionrunner.h
    authconfiguration.cpp
    authconfiguration.h
    fakeadapter.cpp
    fakeadapter.h
    profilemodel.cpp
    profilemodel.h
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
        Qt6::Core
        Qt6::Gui
        KF6::AuthCore
)
target_include_directories(irlume_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
