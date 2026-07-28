# SPDX-License-Identifier: BSD-2-Clause

kcmutils_add_qml_kcm(kcm_irlume
    DISABLE_DESKTOP_FILE_GENERATION
    SOURCES
        camerapreviewitem.cpp
        camerapreviewitem.h
        irlumekcm.cpp
        irlumekcm.h
)

target_link_libraries(kcm_irlume
    irlume_backend
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtilsQuick
)
