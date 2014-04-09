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

class Summary : public QObject
{
    Q_OBJECT
public:
    explicit Summary(QObject *parent = 0);
    
    static QString GenerateHTML();

signals:
    
public slots:
    
};

#endif // SUMMARY_H
