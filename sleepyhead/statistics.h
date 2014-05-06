/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Summary Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef SUMMARY_H
#define SUMMARY_H

#include <QObject>
#include <QHash>
#include <QList>
#include "SleepLib/schema.h"

enum StatCalcType {
    SC_UNDEFINED=0, SC_COLUMNHEADERS, SC_HEADING, SC_SUBHEADING, SC_MEDIAN, SC_AVG, SC_WAVG, SC_90P, SC_MIN, SC_MAX, SC_CPH, SC_SPH, SC_AHI, SC_HOURS, SC_COMPLIANCE, SC_DAYS
};

struct StatisticsRow {
    StatisticsRow() { calc=SC_UNDEFINED; }
    StatisticsRow(QString src, QString calc, QString type) {
        this->src = src;
        this->calc = lookupCalc(calc);
        this->type = lookupType(type);
    }
    StatisticsRow(QString src, StatCalcType calc, MachineType type) {
        this->src = src;
        this->calc = calc;
        this->type = type;
    }
    StatisticsRow(const StatisticsRow &copy) {
        src=copy.src;
        calc=copy.calc;
        type=copy.type;
    }
    QString src;
    StatCalcType calc;
    MachineType type;

    StatCalcType lookupCalc(QString calc)
    {
        if (calc.compare("avg",Qt::CaseInsensitive)==0) {
            return SC_AVG;
        } else if (calc.compare("w-avg",Qt::CaseInsensitive)==0) {
            return SC_WAVG;
        } else if (calc.compare("median",Qt::CaseInsensitive)==0) {
            return SC_MEDIAN;
        } else if (calc.compare("90%",Qt::CaseInsensitive)==0) {
            return SC_90P;
        } else if (calc.compare("min", Qt::CaseInsensitive)==0) {
            return SC_MIN;
        } else if (calc.compare("max", Qt::CaseInsensitive)==0) {
            return SC_MAX;
        } else if (calc.compare("cph", Qt::CaseInsensitive)==0) {
            return SC_CPH;
        } else if (calc.compare("sph", Qt::CaseInsensitive)==0) {
            return SC_SPH;
        } else if (calc.compare("ahi", Qt::CaseInsensitive)==0) {
            return SC_AHI;
        } else if (calc.compare("hours", Qt::CaseInsensitive)==0) {
            return SC_HOURS;
        } else if (calc.compare("compliance", Qt::CaseInsensitive)==0) {
            return SC_COMPLIANCE;
        } else if (calc.compare("days", Qt::CaseInsensitive)==0) {
            return SC_DAYS;
        } else if (calc.compare("heading", Qt::CaseInsensitive)==0) {
            return SC_HEADING;
        } else if (calc.compare("subheading", Qt::CaseInsensitive)==0) {
            return SC_SUBHEADING;
        }
        return SC_UNDEFINED;
    }

    MachineType lookupType(QString type)
    {
        if (type.compare("cpap", Qt::CaseInsensitive)==0) {
            return MT_CPAP;
        } else if (type.compare("oximeter", Qt::CaseInsensitive)==0) {
            return MT_OXIMETER;
        } else if (type.compare("sleepstage", Qt::CaseInsensitive)==0) {
            return MT_SLEEPSTAGE;
        }
        return MT_UNKNOWN;
    }


    ChannelID channel() {
        return schema::channel[src].id();
    }

    QString value(QDate start, QDate end);
};

class Statistics : public QObject
{
    Q_OBJECT
  public:
    explicit Statistics(QObject *parent = 0);

    QString GenerateHTML();

  protected:
    // Using a map to maintain order
    QList<StatisticsRow> rows;
    QMap<StatCalcType, QString> calcnames;
    QMap<MachineType, QString> machinenames;

  signals:

  public slots:

};

#endif // SUMMARY_H
