/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib CMS50X Loader Implementation
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the cms50_data_version in cms50_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************

#include <QProgressBar>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QList>

using namespace std;

#include "cms50_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

// Possibly need to replan this to include oximetry

CMS50Loader::CMS50Loader()
{
}

CMS50Loader::~CMS50Loader()
{
}

bool CMS50Loader::Detect(const QString &path)
{
    Q_UNUSED(path);
    return false;
}

int CMS50Loader::Open(QString &path, Profile *profile)
{
    // CMS50 folder structure detection stuff here.

    // Not sure whether to both supporting SpO2 files here, as the timestamps are utterly useless for overlay purposes.
    // May just ignore the crap and support my CMS50 logger

    // Contains three files
    // Data Folder
    // SpO2 Review.ini
    // SpO2.ini
    if (!profile) {
        qWarning() << "Empty Profile in CMS50Loader::Open()";
        return 0;
    }

    // This bit needs modifying for the SPO2 folder detection.
    QDir dir(path);
    QString tmp = path + "/Data"; // The directory path containing the .spor/.spo2 files

    if ((dir.exists("SpO2 Review.ini") || dir.exists("SpO2.ini"))
            && dir.exists("Data")) {
        // SPO2Review/etc software

      //  return OpenCMS50(tmp, profile);
    }

    return 0;
}

Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile) {
        return nullptr;
    }

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    QList<Machine *> ml = profile->GetMachines(MT_OXIMETER);

    for (QList<Machine *>::iterator i = ml.begin(); i != ml.end(); i++) {
        if ((*i)->GetClass() == cms50_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug() << "Create CMS50 Machine Record";

    Machine *m = new Oximeter(profile, 0);
    m->SetClass(cms50_class_name);
    m->properties[STR_PROP_Brand] = "Contec";
    m->properties[STR_PROP_Model] = "CMS50X";
    m->properties[STR_PROP_DataVersion] = QString::number(cms50_data_version);

    profile->AddMachine(m);
    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + m->hexid() + "/";
    m->properties[STR_PROP_Path] = path;

    return m;
}



static bool cms50_initialized = false;

void CMS50Loader::Register()
{
    if (cms50_initialized) { return; }

    qDebug() << "Registering CMS50Loader";
    RegisterLoader(new CMS50Loader());
    cms50_initialized = true;
}

