# SPDX-License-Identifier: BSD-2-Clause

install(FILES kcm_irlume.desktop DESTINATION "${KDE_INSTALL_APPDIR}")

kauth_install_actions(
    io.github.loofibossbit.plasmairlume.helper
    io.github.loofiboss_bit.plasma_irlume.actions
)
