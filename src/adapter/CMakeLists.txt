# SPDX-License-Identifier: BSD-2-Clause

add_library(irlume_adapter STATIC
    irlumeprocess.cpp
    irlumeprocess.h
)

set_target_properties(irlume_adapter PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(irlume_adapter PUBLIC Qt6::Core)
target_include_directories(irlume_adapter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
