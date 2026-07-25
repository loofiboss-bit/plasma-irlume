# Translations

Place gettext catalogs in `<locale>/kcm_irlume.po`, following the KDE i18n
layout. CMake installs available catalogs through `ki18n_install()`, and the
Fedora package collects them with `%find_lang`.

The experimental 1.0.0 release includes a small Swedish catalog for the
application name and primary navigation. Additional strings and languages can
be added incrementally without changing the package manifest.
