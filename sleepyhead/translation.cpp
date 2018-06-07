/* Multilingual Support files
 *
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the source code
 * for more details. */

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

    // (Ordinary character sets will just use the name before the first '.' in the filename.)
    // (This u8 stuff deliberately kills Qt4.x build support - if you know another way feel free to
    //  change it, but Qt4 support is still going to die sooner or later)
    // Add any languages with special character set needs to this list
    QHash<QString, QString> langNames;
    langNames["zh"] = "\xe6\xbc\xa2\xe8\xaa\x9e\xe7\xb9\x81\xe9\xab\x94\xe5\xad\x97";
    langNames["es"] = "Espa\xc3\xb1ol";
    langNames["bg"] = "\xd0\xb1\xd1\x8a\xd0\xbb\xd0\xb3\xd0\xb0\xd1\x80\xd1\x81\xd0\xba\xd0\xb8";
    langNames["fr"] = "\x46\x72\x61\x6e\xc3\xa7\x61\x69\x73";
    langNames["en_UK"] = "English UK";
    // CHECK: Will the above break with MS VisualC++ compiler?

    QHash<QString, QString> langFiles;

#ifdef Q_OS_MAC
    QString transdir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../Resources/Translations/");
#else
    const QString transdir = QCoreApplication::applicationDirPath() + "/Translations/";
#endif

    QDir dir(transdir);
    qDebug() << "Scanning" << transdir.toLocal8Bit().data();
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList("*.qm"));

    QFileInfoList list = dir.entryInfoList();
    QString language = settings.value(LangSetting).toString();

    QString langfile, langname;

    // Add default language (English)
    const QString en="en_US";
    langFiles[en]="English.en_US.qm";
    langNames[en]="English US";

    // Scan through available translations, and add them to the list
    QStringList availtrans;
    for (const auto & fi : list) {
        QString name = fi.fileName().section('.', 0, 0);
        QString code = fi.fileName().section('.', 1, 1);

        availtrans.push_back(name);

        if (langNames.contains(code)) {
            name = langNames[code];
        } else {
            langNames[code]=name;
        }

        langFiles[code]=fi.fileName();

    }
    qDebug() << "Available Translations:" << QString(availtrans.join(", ")).toLocal8Bit().data();

    if (language.isEmpty() || !langNames.contains(language)) {
        QDialog langsel(nullptr, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        QFont font;
        font.setPointSize(20);
        langsel.setFont(font);
        langsel.setWindowTitle("Language / Taal / Sprache / Langue / \xe8\xaf\xad\xe8\xa8\x80 / ... ");
        QHBoxLayout lang_layout(&langsel);

        QLabel img;
        img.setPixmap(QPixmap(":/docs/sheep.png"));

        // hard coded non translatable

        QPushButton lang_okbtn("->", &langsel);

        QVBoxLayout layout1;
        QVBoxLayout layout2;

        layout2.setMargin(6);
        lang_layout.setContentsMargins(4,4,4,4);
        lang_layout.setMargin(6);
        layout2.setSpacing(6);
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
            if (!langFiles.contains(code) || langFiles[code].isEmpty())
                continue;

            QListWidgetItem *item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, code);
            langlist.insertItem(row++, item);
            // Todo: Use base system language code
            if (code.compare(en) == 0) {
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
        settings.setValue(LangSetting, language);
    }

    langname=langNames[language];
    langfile=langFiles[language];

    if (language.compare(en) != 0) {
        qDebug() << "Loading " << langname << " Translation" << langfile << "from" << transdir;
        QTranslator * translator = new QTranslator();

        if (!langfile.isEmpty() && !translator->load(langfile, transdir)) {
            qWarning() << "Could not load translation" << langfile << "reverting to english :(";
        }

        qApp->installTranslator(translator);
    } else {
        qDebug() << "Using in-built english Translation";
    }
}
