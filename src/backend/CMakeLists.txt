# SPDX-License-Identifier: BSD-2-Clause

add_library(irlume_backend STATIC
    fakeadapter.cpp
    fakeadapter.h
    systemstate.cpp
    systemstate.h
    systemprobe.cpp
    systemprobe.h
)

set_target_properties(irlume_backend PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(irlume_backend PUBLIC Qt6::Core)
target_include_directories(irlume_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
