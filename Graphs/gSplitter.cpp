// This can die.

#include "gSplitter.h"
#include "graphwindow.h"
#include <QDebug>
gSplitter::gSplitter(QWidget *parent) :
    QSplitter(parent)
{
    z_timer=new QTimer(this);

    //timer=NULL;
 //   icnt=0;
}
gSplitter::gSplitter(Qt::Orientation orientation, QWidget *parent) :
    QSplitter(orientation,parent)
{
   // icnt=0;
    this->connect(this,SIGNAL(splitterMoved(int,int)),SLOT(mySplitterMoved(int,int)));
    z_timer=new QTimer(this);
}
gSplitter::~gSplitter()
{
    delete z_timer;
    this->disconnect(SLOT(mySplitterMoved(int,int)));
   // timer->stop();
}

void gSplitter::mySplitterMoved (int pos, int index)
{
    if (z_timer->isActive()) z_timer->stop();
    z_pos=pos;
    z_index=index;
    //this->setUpdatesEnabled(true);
    if (gGraphWindow *w=qobject_cast<gGraphWindow *>(widget(index-1))) {
        int s=sizes().at(index-1);
        //w->resizeGL(w->width(),pos);
        //w->updateGL();
        //w->updateGL();
        //w->paintGL();
    }
    if (gGraphWindow *w=qobject_cast<gGraphWindow *>(widget(index))) {
        int s=sizes().at(index);
        //w->resizeGL(w->width(),s);
        //w->updateGL();
        //w->paintGL();
    }
    qDebug() << ++icnt;
    z_timer->singleShot(50,this,SLOT(doUpdateGraph()));
    tm.start();
}

void gSplitter::doUpdateGraph()
{

    if (tm.elapsed()<50)
        return;
    //this->setUpdatesEnabled(true);

    if (gGraphWindow *w=qobject_cast<gGraphWindow *>(widget(z_index-1))) {
        //qDebug() << icnt << "Height" << w->height() << z_index << z_pos << w->Title();

        int s=sizes().at(z_index-1);

        QSize n(w->width(),s);
        QSize o(w->width(),s);
        //QResizeEvent e(n,o);
        //w->resizeEvent(&e);
        //w->resizeGL(w->width(),s);
        //w->paintGL();
    }
    if (gGraphWindow *w=qobject_cast<gGraphWindow *>(widget(z_index))) {
        qDebug() << icnt << "Height" << w->height() << z_index << z_pos << w->Title();

        int s=sizes().at(z_index);

        w->resizeGL(w->width(),s);
        w->paintGL();
    }
    //timer->stop();
    icnt=0;
 //   QSplitter::resizeEvent(&event);

}
