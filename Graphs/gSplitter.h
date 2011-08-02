#ifndef GSPLITTER_H
#define GSPLITTER_H

#include <QSplitter>
#include <QResizeEvent>
#include <QTime>
#include <QTimer>

class gSplitter : public QSplitter
{
    Q_OBJECT
public:
    explicit gSplitter(QWidget *parent = 0);
    explicit gSplitter(Qt::Orientation orientation, QWidget *parent = 0);
    virtual ~gSplitter();
signals:

public slots:
    void mySplitterMoved(int pos, int index);
    void doUpdateGraph();

protected:
    QTimer * z_timer;
    int z_pos,z_index;
    int icnt;
    QTime tm;

};

#endif // GSPLITTER_H
