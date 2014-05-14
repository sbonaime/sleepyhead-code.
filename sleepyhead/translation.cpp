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
#include <QListWidget>

#ifndef nullptr
#define nullptr NULL
#endif

#include "translation.h"

void initTranslations(QSettings & settings) {

    QStringList welcome;
    welcome.push_back("Welcome");
    welcome.push_back("Welkom");
    welcome.push_back("Willkommen");
    welcome.push_back("Bienvenue");
    welcome.push_back("歡迎");
    welcome.push_back("ようこそ！");

    // (Ordinary character sets will just use the name before the first '.' in the filename.)
    // (This u8 stuff deliberately kills Qt4.x build support - if you know another way feel free to
    //  change it, but Qt4 support is still going to die sooner or later)
    // Add any languages with special character set needs to this list
    QHash<QString, QString> langNames;
    langNames["cn"]="漢語繁體字";
    langNames["es"] = "Español";
    langNames["bg"] = "български";
    langNames["fr"] = "Français";
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
        langsel.setWindowTitle("Language / Taal / Sprache / Langue / 语言 / ... ");
        QHBoxLayout lang_layout(&langsel);

        QLabel img;
        img.setPixmap(QPixmap(":/docs/sheep.png"));

        // hard coded non translatable

        QPushButton lang_okbtn("->", &langsel);

        QVBoxLayout layout1;
        QVBoxLayout layout2;

        QListWidget langlist;

        lang_layout.addLayout(&layout1);
        lang_layout.addLayout(&layout2);

        layout1.addWidget(&img);
        layout2.addWidget(&langlist, 1);

        langlist.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        int row = 0;
        for (QHash<QString, QString>::iterator it = langNames.begin(); it != langNames.end(); ++it) {
            const QString & code = it.key();
            const QString & name = it.value();
            QListWidgetItem *item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, code);
            langlist.insertItem(row++, item);
            // Todo: Use base system language code
            if (code.compare("en")==0) {
                langlist.setCurrentItem(item);
            }
        }
        langlist.sortItems();
        layout2.addWidget(&lang_okbtn);

        langsel.connect(&langlist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), &langsel, SLOT(close()));
        langsel.connect(&lang_okbtn, SIGNAL(clicked()), &langsel, SLOT(close()));

        langsel.exec();
        langsel.disconnect(&lang_okbtn, SIGNAL(clicked()), &langsel, SLOT(close()));
        langsel.disconnect(&langlist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), &langsel, SLOT(close()));
        langname = langlist.currentItem()->text();
        language = langlist.currentItem()->data(Qt::UserRole).toString();
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
