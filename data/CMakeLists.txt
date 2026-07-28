# SPDX-License-Identifier: BSD-2-Clause

configure_file(
    kcm_kfaceauth.desktop.in
    "${CMAKE_CURRENT_BINARY_DIR}/${KFACEAUTH_KCM_ID}.desktop"
    @ONLY
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${KFACEAUTH_KCM_ID}.desktop"
    DESTINATION "${KDE_INSTALL_APPDIR}"
)
