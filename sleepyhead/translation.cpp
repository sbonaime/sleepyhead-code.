/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Multilingual Support files
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <QDebug>
#include <QStringList>
#include <QList>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDir>
#include <QSettings>
#include <QTranslator>

#include "translation.h"

void initTranslations(QSettings & settings) {


    QStringList welcome={"Welcome", "Welkom", "Willkommen", "Bienvenue", u8"歡迎", u8"ようこそ！"};

    // (Ordinary character sets will just use the name before the first '.' in the filename.)
    // (This u8 stuff deliberately kills Qt4.x build support - if you know another way feel free to
    //  change it, but Qt4 support is still going to die sooner or later)
    // Add any languages with special character set needs to this list
    QHash<QString, QString> langNames={
        { "cn", u8"漢語繁體字" },
        { "es", u8"Español" },
        { "bg", u8"български" },
        { "fr", u8"Français" },
    };
    // CHECK: Will the above break with MS VisualC++ compiler?

    QHash<QString, QString> langFiles;

#ifdef Q_OS_MAC
    QString transdir = QDir::cleanPath(QCoreApplication::applicationDirPath() +
                                   "/../Resources/Translations/");

#else
    const QString transdir = QCoreApplication::applicationDirPath() + "/Translations/";
#endif

    QDir dir(transdir);
    qDebug() << "Scanning \"" << transdir << "\" for translations";
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList("*.qm"));

    QFileInfoList list = dir.entryInfoList();
    QString language = settings.value("Settings/Language").toString();

    QString langfile, langname, langcode;

    // Add default language (English)
    const QString en="en";
    langFiles[en]="";
    langNames[en]="English";


    // Scan through available translations, and add them to the list
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fi = list.at(i);
        QString name = fi.fileName().section('.', 0, 0);
        QString code = fi.fileName().section('.', 1, 1);

        qDebug() << "Detected" << name << "Translation";

        if (langNames.contains(code)) {
            name = langNames[code];
        } else {
            langNames[code]=name;
        }

        langFiles[code]=fi.fileName();

    }

    if (language.isEmpty() || !langNames.contains(language)) {
        QDialog langsel(nullptr, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        QFont font;
        font.setPointSize(25);
        langsel.setFont(font);
        langsel.setWindowTitle(u8"Language / Taal / Sprache / Langue / 语言 / ... ");
        QHBoxLayout lang_layout(&langsel);

        QLabel img;
        img.setPixmap(QPixmap(":/docs/sheep.png"));

        // hard coded non translatable

        QComboBox lang_combo(&langsel);
        QPushButton lang_okbtn("->", &langsel);

        QVBoxLayout layout1(&langsel);
        QVBoxLayout layout2(&langsel);

        lang_layout.addLayout(&layout1);
        lang_layout.addLayout(&layout2);

        layout1.addWidget(&img);

        for (int i=0;i<welcome.size(); i++) {
            QLabel *welcomeLabel = new QLabel(welcome[i]);
            layout2.addWidget(welcomeLabel);
        }

        QWidget spacer;
        layout2.addWidget(&spacer,1);
        layout2.addWidget(&lang_combo, 1);
        layout2.addWidget(&lang_okbtn);

        for (auto it = langNames.begin(); it != langNames.end(); ++it) {
            const QString & code = it.key();
            const QString & name = it.value();
            lang_combo.addItem(name, code);
        }


        langsel.connect(&lang_okbtn, SIGNAL(clicked()), &langsel, SLOT(close()));

        langsel.exec();
        langsel.disconnect(&lang_okbtn, SIGNAL(clicked()), &langsel, SLOT(close()));
        langname = lang_combo.currentText();
        language = lang_combo.itemData(lang_combo.currentIndex()).toString();
        settings.setValue("Settings/Language", language);
    }

    langname=langNames[language];
    langfile=langFiles[language];

    qDebug() << "Loading " << langname << " Translation" << langfile << "from" << transdir;
    QTranslator * translator = new QTranslator();

    if (!langfile.isEmpty() && !translator->load(langfile, transdir)) {
        qWarning() << "Could not load translation" << langfile << "reverting to english :(";
    }

    qApp->installTranslator(translator);
}
