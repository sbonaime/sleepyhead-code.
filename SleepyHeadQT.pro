TEMPLATE = subdirs

win32:SUBDIRS = 3rdparty

SUBDIRS += sleepyhead

CONFIG += ordered
win32:sleepyhead.depends = 3rdparty

TRANSLATIONS += \
    Translations/Nederlands.nl_NL.ts \
    Translations/Francais.fr.ts \
    Translations/Svenska.se.ts \
    Translations/Deutsch.de_DE.ts \
    Translations/Espaniol.es.ts \
    Translations/Bulgarian.bg.ts


