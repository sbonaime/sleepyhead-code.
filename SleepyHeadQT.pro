TEMPLATE = subdirs

CONFIG -= use_bundled_libs

# Need them for windows..
win32:CONFIG += use_bundled_libs

use_bundled_libs {
    SUBDIRS = 3rdparty
}

SUBDIRS += sleepyhead

use_bundled_libs {
    CONFIG += ordered
    sleepyhead.depends = 3rdparty
}

TRANSLATIONS += \
    Translations/Nederlands.nl_NL.ts \
    Translations/Francais.fr.ts \
    Translations/Svenska.se.ts \
    Translations/Deutsch.de_DE.ts \
    Translations/Espaniol.es.ts \
    Translations/Bulgarian.bg.ts


