#ifndef SESSIONBAR_H
#define SESSIONBAR_H

#include <QList>
#include <QColor>
#include <QWidget>
#include <QTimer>

#include "SleepLib/session.h"

typedef qint64 SegType;

class SBSeg {
public:
    SBSeg();
    SBSeg(Session * sess, QColor col);
//    SBSeg(const SBSeg & a);

    QColor color;
    bool highlight;
    Session * session;
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
    void add(Session * sess, QColor col) { if (sess) segments.push_back(SBSeg(sess,col)); }

protected slots:
    void updateTimer();
signals:
    void toggledSession(Session * sess);
protected:
    void paintEvent(QPaintEvent * event);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    SegType min();
    SegType max();

    QList<SBSeg> segments;
    QTimer timer;
};
//Q_DECLARE_METATYPE(SessionBar)

#endif
