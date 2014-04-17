/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SessionBar
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef SESSIONBAR_H
#define SESSIONBAR_H

#include <QList>
#include <QColor>
#include <QWidget>
#include <QTimer>

#include "SleepLib/session.h"

typedef qint64 SegType;

class SBSeg
{
  public:
    SBSeg();
    SBSeg(Session *sess, QColor col);
    //    SBSeg(const SBSeg & a);

    QColor color;
    bool highlight;
    Session *session;
};


class SessionBar : public QWidget
{
    Q_OBJECT
  public:
    SessionBar(QWidget *parent = 0);
    //    // Q_DECLARE_METATYPE requires a copy-constructor
    //  SessionBar(const SessionBar &);

    virtual ~SessionBar();
    void clear() { segments.clear(); }
    void add(Session *sess, QColor col) { if (sess) { segments.push_back(SBSeg(sess, col)); } }

  protected slots:
    void updateTimer();
  signals:
    void toggledSession(Session *sess);
  protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    SegType min();
    SegType max();

    QList<SBSeg> segments;
    QTimer timer;
};
//Q_DECLARE_METATYPE(SessionBar)

#endif
