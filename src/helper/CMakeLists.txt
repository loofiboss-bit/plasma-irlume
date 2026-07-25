# SPDX-License-Identifier: BSD-2-Clause

add_library(irlume_auth_helper STATIC
    authhelper.cpp
    authhelper.h
)

set_target_properties(irlume_auth_helper PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(irlume_auth_helper PUBLIC Qt6::Core KF6::AuthCore)
target_include_directories(irlume_auth_helper PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(plasma-irlume-auth-helper main.cpp)
target_link_libraries(plasma-irlume-auth-helper PRIVATE irlume_auth_helper)

install(TARGETS plasma-irlume-auth-helper DESTINATION "${KAUTH_HELPER_INSTALL_DIR}")
kauth_install_helper_files(
    plasma-irlume-auth-helper
    io.github.loofibossbit.plasmairlume.helper
    root
)
