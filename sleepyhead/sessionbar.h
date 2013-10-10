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
    SBSeg() { session=NULL; color=QColor(); highlight=false; }
    SBSeg(Session * sess, QColor col) { session=sess; color=col; highlight=false; }
    SBSeg(const SBSeg & a) { session=a.session; color=a.color; highlight=a.highlight; }

    Session * session;
    QColor color;
    bool highlight;
};


class SessionBar : public QWidget
{
    Q_OBJECT
public:
    SessionBar(QWidget *parent = 0);
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

    QList<SBSeg> segments;
    SegType min();
    SegType max();
    QTimer timer;
};

#endif
