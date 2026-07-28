# SPDX-License-Identifier: BSD-2-Clause

add_library(preview_protocol STATIC
    previewprotocol.cpp
    previewprotocol.h
)

set_target_properties(preview_protocol PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(preview_protocol PUBLIC Qt6::Core)
target_include_directories(preview_protocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(camera_provider STATIC
    cameraprovider.cpp
    cameraprovider.h
)
set_target_properties(camera_provider PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(camera_provider
    PUBLIC
        preview_protocol
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        PkgConfig::UDev
)
target_include_directories(camera_provider PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(plasma-irlume-camera-preview-worker
    main.cpp
    previewworker.cpp
    previewworker.h
)

target_link_libraries(plasma-irlume-camera-preview-worker
    PRIVATE
        camera_provider
        preview_protocol
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        PkgConfig::UDev
)

install(TARGETS plasma-irlume-camera-preview-worker
    RUNTIME DESTINATION libexec
)
