TEMPLATE = subdirs

SUBDIRS = 3rdparty sleepyhead

CONFIG += ordered

TRANSLATIONS += \
    Translations/Nederlands.nl_NL.ts \
    Translations/Francais.fr.ts \
    Translations/Svenska.se.ts \
    Translations/Deutsch.de_DE.ts \
    Translations/Espaniol.es.ts \
    Translations/Bulgarian.bg.ts


sleepyhead.depends = 3rdparty
