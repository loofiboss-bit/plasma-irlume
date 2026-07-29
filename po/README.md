# Translations

Place gettext catalogs in `<locale>/kcm_kfaceauth.po`, following the KDE i18n
layout. CMake installs available catalogs through `ki18n_install()`, and the
Fedora package collects them with `%find_lang`.

English is the source language. Keep the Swedish catalog complete when active
UI or backend messages change:

```bash
xgettext --language=JavaScript --from-code=UTF-8 \
  --keyword=i18n --keyword=i18np:1,2 \
  -o po/kcm_kfaceauth-qml.pot \
  src/kcm/ui/*.qml src/kcm/ui/components/*.qml
xgettext --language=C++ --from-code=UTF-8 \
  --keyword=translate:1 --keyword=userText:1 \
  -o po/kcm_kfaceauth-cpp.pot src/backend/*.cpp src/preview/*.cpp
msgcat --use-first -o po/kcm_kfaceauth.pot \
  po/kcm_kfaceauth-qml.pot po/kcm_kfaceauth-cpp.pot
msgmerge --update --backup=none \
  po/sv/kcm_kfaceauth.po po/kcm_kfaceauth.pot
msgfmt --check --check-format -o /dev/null po/sv/kcm_kfaceauth.po
```
