TEMPLATE = subdirs

SUBDIRS = 3rdparty src

CONFIG += ordered

TRANSLATIONS += \
    Translations/Nederlands.nl_NL.ts \
    Translations/Francais.fr.ts \
    Translations/Svenska.se.ts \
    Translations/Deutsch.de_DE.ts \
    Translations/Espaniol.es.ts

src.depends = 3rdparty
