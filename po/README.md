# Translations

Place gettext catalogs in `<locale>/kcm_irlume.po`, following the KDE i18n
layout. CMake installs available catalogs through `ki18n_install()`, and the
Fedora package collects them with `%find_lang`.

The V2 catalog covers every QML user-interface string in Swedish. English
remains the source language. Keep the catalog complete when UI text changes:

```bash
xgettext --language=JavaScript --from-code=UTF-8 \
  --keyword=i18n --keyword=i18np:1,2 \
  -o po/kcm_irlume-qml.pot src/kcm/ui/*.qml src/kcm/ui/components/*.qml
xgettext --language=C++ --from-code=UTF-8 \
  --keyword=translate:1 --keyword=QCoreApplication::translate:2 \
  -o po/kcm_irlume-cpp.pot src/backend/*.cpp
xgettext --language=C++ --from-code=UTF-8 \
  --keyword=translate:1c,2 \
  -o po/kcm_irlume-context.pot src/backend/fakeadapter.cpp
msgcat --use-first -o po/kcm_irlume.pot \
  po/kcm_irlume-qml.pot po/kcm_irlume-cpp.pot po/kcm_irlume-context.pot
msgmerge --update --backup=none po/sv/kcm_irlume.po po/kcm_irlume.pot
msgfmt --check --check-format -o /dev/null po/sv/kcm_irlume.po
```
