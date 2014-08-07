/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gGraphView Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/gGraphView.h"

#include <QDir>
#include <QFontMetrics>
#include <QLabel>
#include <QPixmapCache>
#include <QTimer>
#include <QFontMetrics>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
# include <QWindow>
#endif

#ifdef DEBUG_EFFICIENCY
// Only works in 4.8
# include <QElapsedTimer>
#endif

#include <cmath>

#include "mainwindow.h"
#include "Graphs/glcommon.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gFlagsLine.h"
#include "SleepLib/profiles.h"

extern MainWindow *mainwin;
extern QLabel *qstatus2;


gToolTip::gToolTip(gGraphView *graphview)
    : m_graphview(graphview)
{

    m_pos.setX(0);
    m_pos.setY(0);
    m_visible = false;
    m_alignment = TT_AlignCenter;
    m_spacer = 8; // pixels around text area
    timer = new QTimer(graphview);
    connect(timer, SIGNAL(timeout()), SLOT(timerDone()));
}

gToolTip::~gToolTip()
{
    disconnect(timer, 0, 0, 0);
    delete timer;
}
//void gToolTip::calcSize(QString text,int &w, int &h)
//{
/*GetTextExtent(text,w,h);
w+=m_spacer*2;
h+=m_spacer*2; */
//}

void gToolTip::display(QString text, int x, int y, ToolTipAlignment align, int timeout)
{
    if (timeout <= 0) {
        timeout = p_profile->general->tooltipTimeout();
    }
    m_alignment = align;

    m_text = text;
    m_visible = true;
    // TODO: split multiline here
    //calcSize(m_text,tw,th);

    m_pos.setX(x);
    m_pos.setY(y);

    //tw+=m_spacer*2;
    //th+=m_spacer*2;
    //th*=2;
    if (timer->isActive()) {
        timer->stop();
    }

    timer->setSingleShot(true);
    timer->start(timeout);
    m_invalidate = true;
}

void gToolTip::cancel()
{
    m_visible = false;
    timer->stop();
}

void gToolTip::paint(QPainter &painter)     //actually paints it.
{
    if (!m_visible) { return; }

    int x = m_pos.x();
    int y = m_pos.y();

    QRect rect(x, y, 0, 0);

    painter.setFont(*defaultfont);

    rect = painter.boundingRect(rect, Qt::AlignCenter, m_text);

    int w = rect.width() + m_spacer * 2;
    int xx = rect.x() - m_spacer;

    if (xx < 0) { xx = 0; }

    rect.setLeft(xx);
    rect.setTop(rect.y() - rect.height() / 2);
    rect.setWidth(w);

    int z = rect.x() + rect.width();

    if (z > m_graphview->width() - 10) {
        rect.setLeft(m_graphview->width() - 2 - rect.width());
        rect.setRight(m_graphview->width() - 2);
    }

    int h = rect.height();

    if (rect.y() < 0) {
        rect.setY(0);
        rect.setHeight(h);
    }

    if (m_alignment == TT_AlignRight) {
        rect.moveTopRight(m_pos);
        if ((x-w) < 0) {
            rect.moveLeft(0);
        }
    } else if (m_alignment == TT_AlignLeft) {
        rect.moveTopLeft(m_pos);
    }


    QBrush brush(QColor(255, 255, 128, 230));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(QColor(0, 0, 0, 255));

    painter.drawRoundedRect(rect, 5, 5);
    painter.setBrush(Qt::black);

    painter.setFont(*defaultfont);

    painter.drawText(rect, Qt::AlignCenter, m_text);
}

void gToolTip::timerDone()
{
    m_visible = false;
    m_graphview->redraw();
    m_graphview->resetMouse();
}

#ifdef ENABLE_THREADED_DRAWING

gThread::gThread(gGraphView *g)
{
    graphview = g;
    mutex.lock();
}
gThread::~gThread()
{
    if (isRunning()) {
        m_running = false;
        mutex.unlock();
        wait();
        terminate();
    }
}

void gThread::run()
{
    m_running = true;
    gGraph *g;

    while (m_running) {
        mutex.lock();

        //mutex.unlock();
        if (!m_running) { break; }

        do {
            g = graphview->popGraph();

            if (g) {
                g->paint(QRegion(g->m_lastbounds));
                //int i=0;
            } else {
                //mutex.lock();
                graphview->masterlock->release(1); // This thread gives up for now..

            }
        } while (g);
    }
}

#endif // ENABLE_THREADED_DRAWING

void gGraphView::queGraph(gGraph *g, int left, int top, int width, int height)
{
    g->m_rect = QRect(left, top, width, height);
#ifdef ENABLED_THREADED_DRAWING
    dl_mutex.lock();
#endif
    m_drawlist.push_back(g);
#ifdef ENABLED_THREADED_DRAWING
    dl_mutex.unlock();
#endif
}

void gGraphView::trashGraphs()
{
    // Don't actually want to delete them here.. we are just borrowing the graphs
    m_graphs.clear();
    m_graphsbyname.clear();
}

// Take the next graph to render from the drawing list
gGraph *gGraphView::popGraph()
{
    gGraph *g;
#ifdef ENABLED_THREADED_DRAWING
    dl_mutex.lock();
#endif

    if (!m_drawlist.isEmpty()) {
        g = m_drawlist.at(0);
        m_drawlist.pop_front();
    } else { g = nullptr; }

#ifdef ENABLED_THREADED_DRAWING
    dl_mutex.unlock();
#endif
    return g;
}

gGraphView::gGraphView(QWidget *parent, gGraphView *shared)
#ifdef BROKEN_OPENGL_BUILD
    : QWidget(parent),
#else
    : QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::DirectRendering | QGL::HasOverlay | QGL::Rgba),parent,shared),
#endif
      m_offsetY(0), m_offsetX(0), m_scaleY(1.0), m_scrollbar(nullptr)
{
    m_shared = shared;
    m_sizer_index = m_graph_index = 0;
    m_metaselect = m_button_down = m_graph_dragging = m_sizer_dragging = false;
    m_lastypos = m_lastxpos = 0;
    m_horiz_travel = 0;
    m_minx = m_maxx = 0;
    m_day = nullptr;
    m_selected_graph = nullptr;
    m_scrollbar = nullptr;
    m_point_released = m_point_clicked = QPoint(0,0);

    horizScrollTime.start();
    vertScrollTime.start();

    this->setMouseTracking(true);
    m_emptytext = QObject::tr("No Data");
    InitGraphGlobals(); // FIXME: sstangl: handle error return.
#ifdef ENABLE_THREADED_DRAWING
    m_idealthreads = QThread::idealThreadCount();

    if (m_idealthreads <= 0) { m_idealthreads = 1; }

    masterlock = new QSemaphore(m_idealthreads);
#endif
    m_tooltip = new gToolTip(this);
    /*for (int i=0;i<m_idealthreads;i++) {
        gThread * gt=new gThread(this);
        m_threads.push_back(gt);
        //gt->start();
    }*/

    setFocusPolicy(Qt::StrongFocus);
    m_showsplitter = true;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(refreshTimeout()));
    print_scaleY = print_scaleX = 1.0;

    redrawtimer = new QTimer(this);
    connect(redrawtimer, SIGNAL(timeout()), SLOT(redraw()));

    m_fadingOut = false;
    m_fadingIn = false;
    m_inAnimation = false;
    m_limbo = false;
    m_fadedir = false;
    m_blockUpdates = false;
    use_pixmap_cache = p_profile->appearance->usePixmapCaching();

   // pixmapcache.setCacheLimit(10240*2);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    m_dpr = devicePixelRatio();
    m_dpr = 1;
#endif

#ifndef BROKEN_OPENGL_BUILD
    setAutoFillBackground(false);
    setAutoBufferSwap(false);
#endif
}

void gGraphView::closeEvent(QCloseEvent * event)
{

    timer->stop();
    redrawtimer->stop();
    disconnect(redrawtimer, 0, 0, 0);
    disconnect(timer, 0, 0, 0);
    timer->deleteLater();
    redrawtimer->deleteLater();
    pixmapcache.clear();
    if (m_scrollbar) {
        this->disconnect(m_scrollbar, SIGNAL(sliderMoved(int)), 0, 0);
    }
#ifdef BROKEN_OPENGL_BUILD
    QWidget::closeEvent(event);
#else
    QGLWidget::closeEvent(event);
#endif
}

gGraphView::~gGraphView()
{
#ifndef BROKEN_OPENGL_BUILD
    doneCurrent();
#endif

#ifdef ENABLE_THREADED_DRAWING

    for (int i = 0; i < m_threads.size(); i++) {
        delete m_threads[i];
    }

    delete masterlock;
#endif

    // Note: This will cause a crash if two graphs accidentally have the same name
    for (int i = 0; i < m_graphs.size(); i++) {
        delete m_graphs[i];
        m_graphs[i]=NULL;
    }

    delete m_tooltip;
    m_graphs.clear();
}

void gGraphView::dumpInfo()
{
    QDate date = mainwin->getDaily()->getDate();
    QString text = "==================== CPAP Information Dump ====================";
    mainwin->log(text);

    Day * day = p_profile->GetGoodDay(date, MT_CPAP);
    if (day) {
        QDateTime dt=QDateTime::fromMSecsSinceEpoch(day->first());

        mainwin->log(QString("Available Channels for %1").arg(dt.toString("MMM dd yyyy")));
        QHash<schema::ChanType, QList<schema::Channel *> > list;

        for (int i=0; i< day->size(); ++i) {
            Session * sess = day->sessions.at(i);
            QHash<ChannelID, QVector<EventList *> >::iterator it;
            for (it = sess->eventlist.begin(); it != sess->eventlist.end(); ++it) {
                ChannelID code = it.key();
                schema::Channel * chan = &schema::channel[code];
                list[chan->type()].append(chan);
            }
        }

        QHash<schema::ChanType, QList<schema::Channel *> >::iterator lit;
        for (lit = list.begin(); lit != list.end(); ++lit) {
            switch (lit.key()) {
            case schema::DATA:
                text = "DATA: ";
                break;
            case schema::SETTING:
                text = "SETTING: ";
                break;
            case schema::FLAG:
                text = "FLAG: ";
                break;
            case schema::MINOR_FLAG:
                text = "MINOR_FLAG: ";
                break;
            case schema::SPAN:
                text = "SPAN: ";
                break;
            case schema::WAVEFORM:
                text = "WAVEFORM: ";
                break;
            case schema::UNKNOWN:
                text = "UNKNOWN: ";
                break;
            default:
                break;
            }
            QStringList str;
            for (int i=0; i< lit.value().size(); ++i) {
                str.append(lit.value().at(i)->code());
            }
            str.sort();
            text.append(str.join(", "));
            mainwin->log(text);
        }
    }
//    for (int i=0;i<m_graphs.size();i++) {
//        m_graphs[i]->dumpInfo();
//    }
}

bool gGraphView::usePixmapCache()
{
    //use_pixmap_cache is an overide setting
    return p_profile->appearance->usePixmapCaching();
}

#define CACHE_DRAWTEXT
#ifndef CACHE_DRAWTEXT
// Render all qued text via QPainter method
void gGraphView::DrawTextQue(QPainter &painter)
{
    int w, h;

    // not sure if global antialiasing would be better..
    //painter.setRenderHint(QPainter::TextAntialiasing, p_profile->appearance->antiAliasing());
    int m_textque_items = m_textque.size();
    for (int i = 0; i < m_textque_items; ++i) {
        TextQue &q = m_textque[i];
        painter.setPen(q.color);
        painter.setRenderHint(QPainter::TextAntialiasing, q.antialias);
        QFont font = *q.font;
        painter.setFont(font);

        if (q.angle == 0) { // normal text

            painter.drawText(q.x, q.y, q.text);
        } else { // rotated text
            w = painter.fontMetrics().width(q.text);
            h = painter.fontMetrics().xHeight() + 2;

            painter.translate(q.x, q.y);
            painter.rotate(-q.angle);
            painter.drawText(floor(-w / 2.0), floor(-h / 2.0), q.text);
            painter.rotate(+q.angle);
            painter.translate(-q.x, -q.y);
        }
        strings_drawn_this_frame++;
        q.text.clear();
    }

    m_textque.clear();
}

#else
// Render graphs with QPainter or pixmap caching, depending on preferences
void gGraphView::DrawTextQue(QPainter &painter)
{
    // process the text drawing queue
    int m_textque_items = m_textque.size();

    int h,w;


    for (int i = 0; i < m_textque_items; ++i) {
        TextQue &q = m_textque[i];

        // can do antialiased text via texture cache fine on mac
        if (usePixmapCache()) {
            // Generate the pixmap cache "key"
            QString hstr = QString("%1:%2:%3").
                    arg(q.text).
                    arg(q.color.name()).
                    arg(q.font->pointSize());

            QPixmap pm;
            const int buf = 8;
            if (!QPixmapCache::find(hstr, &pm)) {

                QFontMetrics fm(*q.font);
               // QRect rect=fm.tightBoundingRect(q.text);
                w = fm.width(q.text);
                h = fm.height()+buf;

                pm=QPixmap(w, h);
                pm.fill(Qt::transparent);

                QPainter imgpainter(&pm);

                imgpainter.setPen(q.color);

                imgpainter.setFont(*q.font);

                imgpainter.setRenderHint(QPainter::TextAntialiasing, q.antialias);
                imgpainter.drawText(0, h-buf, q.text);
                imgpainter.end();

                QPixmapCache::insert(hstr, pm);
                strings_drawn_this_frame++;
            } else {
                //cached
                strings_cached_this_frame++;
            }

            h = pm.height();
            w = pm.width();
            if (q.angle != 0) {
                float xxx = q.x - h - (h / 2);
                float yyy = q.y + w / 2; // + buf / 2;

                xxx+=4;
                yyy+=4;

                painter.translate(xxx, yyy);
                painter.rotate(-q.angle);
                painter.drawPixmap(QRect(0, h / 2, w, h), pm);
                painter.rotate(+q.angle);
                painter.translate(-xxx, -yyy);
            } else {
                painter.drawPixmap(QRect(q.x - buf / 2 + 4, q.y - h + buf, w, h), pm);
            }
        } else {
            // Just draw the fonts..
            painter.setPen(QColor(q.color));
            painter.setFont(*q.font);

            if (q.angle == 0) {
                painter.drawText(q.x, q.y, q.text);
            } else {
                painter.setFont(*q.font);

                w = painter.fontMetrics().width(q.text);
                h = painter.fontMetrics().xHeight() + 2;

                painter.translate(q.x, q.y);
                painter.rotate(-q.angle);
                painter.drawText(floor(-w / 2.0), floor(-h / 2.0), q.text);
                painter.rotate(+q.angle);
                painter.translate(-q.x, -q.y);
            }
            strings_drawn_this_frame++;

        }

        //q.text.clear();
        //q.text.squeeze();
    }

    m_textque.clear();
}
#endif

void gGraphView::AddTextQue(const QString &text, short x, short y, float angle, QColor color,
                            QFont *font, bool antialias)
{
#ifdef ENABLED_THREADED_DRAWING
    text_mutex.lock();
#endif
    m_textque.append(TextQue(x,y,angle,text,color,font,antialias));
#ifdef ENABLED_THREADED_DRAWING
    text_mutex.unlock();
#endif
//    q.text = text;
//    q.x = x;
//    q.y = y;
//    q.angle = angle;
//    q.color = color;
//    q.font = font;
//    q.antialias = antialias;
}

void gGraphView::addGraph(gGraph *g, short group)
{
    if (!g) {
        qDebug() << "Attempted to add an empty graph!";
        return;
    }

    if (!m_graphs.contains(g)) {
        g->setGroup(group);
        m_graphs.push_back(g);

        if (!m_graphsbyname.contains(g->name())) {
            m_graphsbyname[g->name()] = g;
        } else {
            qDebug() << "Can't have to graphs with the same title in one GraphView!!";
        }

        // updateScrollBar();
    }
}

// Calculate total height of all graphs including spacers
float gGraphView::totalHeight()
{
    float th = 0;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) { continue; }

        th += m_graphs[i]->height() + graphSpacer;
    }

    return ceil(th);
}

float gGraphView::findTop(gGraph *graph)
{
    float th = -m_offsetY;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i] == graph) { break; }

        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) { continue; }

        th += m_graphs[i]->height() * m_scaleY + graphSpacer;
    }

    return ceil(th);
}

float gGraphView::scaleHeight()
{
    float th = 0;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) { continue; }

        th += m_graphs[i]->height() * m_scaleY + graphSpacer;
    }

    return ceil(th);
}

void gGraphView::updateScale()
{
    float th = totalHeight(); // height of all graphs
    float h = height();     // height of main widget

    if (th < h) {
        th -= visibleGraphs() * graphSpacer;   // compensate for spacer height
        m_scaleY = h / th;  // less graphs than fits on screen, so scale to fit
    } else {
        m_scaleY = 1.0;
    }

    updateScrollBar();
}


void gGraphView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e); // This ques a redraw event..

    updateScale();

    for (int i = 0; i < m_graphs.size(); i++) {
        m_graphs[i]->resize(e->size().width(), m_graphs[i]->height()*m_scaleY);
    }
}

void gGraphView::scrollbarValueChanged(int val)
{
    //qDebug() << "Scrollbar Changed" << val;
    if (m_offsetY != val) {
        m_offsetY = val;
        redraw(); // do this on a timer?
    }
}

void gGraphView::updateSelectionTime()
{
    qint64 xx = m_maxx - m_minx;
    double d = xx / 86400000L;
    int h = xx / 3600000L;
    int m = (xx / 60000) % 60;
    int s = (xx / 1000) % 60;
    int ms(xx % 1000);
    QString str;

    if (d > 1) {
        /*QDate d1=QDateTime::fromTime_t(m_minx/1000).toUTC().date();
        QDate d2=QDateTime::fromTime_t(m_maxx/1000).toUTC().date();
        d=p_profile->countDays(MT_CPAP,d1,d2); */

        str.sprintf("%1.0f days", ceil(d));
    } else {
        str.sprintf("%02i:%02i:%02i:%03i", h, m, s, ms);
    }

    if (qstatus2) {
        qstatus2->setText(str);
    }

}
void gGraphView::GetRXBounds(qint64 &st, qint64 &et)
{
    //qint64 m1=0,m2=0;
    gGraph *g = nullptr;

    for (int i = 0; i < m_graphs.size(); i++) {
        g = m_graphs[i];

        if (g->group() == 0) {
            break;
        }
    }

    st = g->rmin_x;
    et = g->rmax_x;
}

void gGraphView::ResetBounds(bool refresh) //short group)
{
    Q_UNUSED(refresh)
    qint64 m1 = 0, m2 = 0;
    gGraph *g = nullptr;

    for (int i = 0; i < m_graphs.size(); i++) {
        m_graphs[i]->ResetBounds();

        if (!m_graphs[i]->min_x) { continue; }

        g = m_graphs[i];

        if (!m1 || m_graphs[i]->min_x < m1) { m1 = m_graphs[i]->min_x; }

        if (!m2 || m_graphs[i]->max_x > m2) { m2 = m_graphs[i]->max_x; }
    }

    if (p_profile->general->linkGroups()) {
        for (int i = 0; i < m_graphs.size(); i++) {
            m_graphs[i]->SetMinX(m1);
            m_graphs[i]->SetMaxX(m2);
        }
    }

    if (!g) { g = m_graphs[0]; }

    m_minx = g->min_x;
    m_maxx = g->max_x;

    qint64 xx = g->max_x - g->min_x;
    double d = xx / 86400000L;
    int h = xx / 3600000L;
    int m = (xx / 60000) % 60;
    int s = (xx / 1000) % 60;
    int ms(xx % 1000);
    QString str;

    if (d > 1) {
        /*QDate d1=QDateTime::fromTime_t(m_minx/1000).toUTC().date();
        QDate d2=QDateTime::fromTime_t(m_maxx/1000).toUTC().date();
        d=p_profile->countDays(MT_CPAP,d1,d2); */

        str.sprintf("%1.0f days", ceil(d));
    } else {
        str.sprintf("%02i:%02i:%02i:%03i", h, m, s, ms);
    }

    if (qstatus2) {
        qstatus2->setText(str);
    }

    updateScale();
}

void gGraphView::GetXBounds(qint64 &st, qint64 &et)
{
    st = m_minx;
    et = m_maxx;
}

void gGraphView::SetXBounds(qint64 minx, qint64 maxx, short group, bool refresh)
{
    for (int i = 0; i < m_graphs.size(); i++) {
        if (p_profile->general->linkGroups() || (m_graphs[i]->group() == group)) {
            m_graphs[i]->SetXBounds(minx, maxx);
        }
    }

    m_minx = minx;
    m_maxx = maxx;

    qint64 xx = maxx - minx;
    double d = xx / 86400000L;
    int h = xx / 3600000L;
    int m = (xx / 60000) % 60;
    int s = (xx / 1000) % 60;
    int ms(xx % 1000);
    QString str = "";

    if (d > 1) {
        str.sprintf("%1.0f days", ceil(xx / 86400000.0));
    } else {
        str.sprintf("%02i:%02i:%02i:%03i", h, m, s, ms);
    }

    if (qstatus2) {
        qstatus2->setText(str);
    }

    if (refresh) { redraw(); }
}

void gGraphView::updateScrollBar()
{
    if (!m_scrollbar || (m_graphs.size() == 0)) {
        return;
    }

    float th = scaleHeight(); // height of all graphs
    float h = height();     // height of main widget

    float vis = 0;

    for (int i = 0; i < m_graphs.size(); i++) {
        vis += (m_graphs[i]->isEmpty() || !m_graphs[i]->visible()) ? 0 : 1;
    }

    if (th < h) { // less graphs than fits on screen

        m_scrollbar->setMaximum(0); // turn scrollbar off.

    } else {  // more graphs than fit on screen
        //m_scaleY=1.0;
        float avgheight = th / vis;
        m_scrollbar->setPageStep(avgheight);
        m_scrollbar->setSingleStep(avgheight / 8.0);
        m_scrollbar->setMaximum(th - height());

        if (m_offsetY > th - height()) {
            m_offsetY = th - height();
        }
    }
}

void gGraphView::setScrollBar(MyScrollBar *sb)
{
    m_scrollbar = sb;
    m_scrollbar->setMinimum(0);
    updateScrollBar();
    this->connect(m_scrollbar, SIGNAL(valueChanged(int)), SLOT(scrollbarValueChanged(int)));
}

bool gGraphView::renderGraphs(QPainter &painter)
{
    float px = m_offsetX;
    float py = -m_offsetY;
    int numgraphs = 0;
    float h, w;
    //ax=px;//-m_offsetX;

    //bool threaded;

    // Tempory hack using this pref..
    //#ifdef ENABLED_THREADED_DRAWING
    /*if (profile->session->multithreading()) { // && (m_idealthreads>1)) {
        threaded=true;
        for (int i=0;i<m_idealthreads;i++) {
            if (!m_threads[i]->isRunning())
                m_threads[i]->start();
        }
    } else threaded=false; */
    //#endif
    //threaded=false;


    lines_drawn_this_frame = 0;
    quads_drawn_this_frame = 0;

    // Calculate the height of pinned graphs

    float pinned_height = 0; // pixel height total
    int pinned_graphs = 0; // count

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty()) { continue; }

        if (!m_graphs[i]->visible()) { continue; }

        if (!m_graphs[i]->isPinned()) { continue; }

        h = m_graphs[i]->height() * m_scaleY;
        pinned_height += h + graphSpacer;
        pinned_graphs++;
    }

    py += pinned_height; // start drawing at the end of pinned space

    // Draw non pinned graphs
    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty()) { continue; }

        if (!m_graphs[i]->visible()) { continue; }

        if (m_graphs[i]->isPinned()) { continue; }

        numgraphs++;
        h = m_graphs[i]->height() * m_scaleY;

        // set clipping?

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((py + h + graphSpacer) >= 0) {
            w = width();
            int tw = (m_graphs[i]->showTitle() ? titleWidth : 0);

            queGraph(m_graphs[i], px + tw, py, width() - tw, h);

            if ((m_graphs.size() > 1) && m_showsplitter) {
                // draw the splitter handle

                painter.setPen(QColor(220, 220, 220, 255));
                painter.drawLine(0, py + h, w, py + h);
                painter.setPen(QColor(158,158,158,255));
                painter.drawLine(0, py + h + 1, w, py + h + 1);
                painter.setPen(QColor(240, 240, 240, 255));
                painter.drawLine(0, py + h + 2, w, py + h + 2);

            }

        }

        py = ceil(py + h + graphSpacer);
    }

    // Physically draw the unpinned graphs
    int s = m_drawlist.size();

    for (int i = 0; i < s; i++) {
        gGraph *g = m_drawlist.at(0);
        m_drawlist.pop_front();
        g->paint(painter, QRegion(g->m_rect));
    }

    if (m_graphs.size() > 1) {
        DrawTextQue(painter);

        // Draw a gradient behind pinned graphs
        QLinearGradient linearGrad(QPointF(100, 100), QPointF(width() / 2, 100));
        linearGrad.setColorAt(0, QColor(216, 216, 255));
        linearGrad.setColorAt(1, Qt::white);

        painter.fillRect(0, 0, width(), pinned_height, QBrush(linearGrad));
    }

    py = 0; // start drawing at top...

    // Draw Pinned graphs
    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty()) { continue; }

        if (!m_graphs[i]->visible()) { continue; }

        if (!m_graphs[i]->isPinned()) { continue; }

        h = m_graphs[i]->height() * m_scaleY;
        numgraphs++;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((py + h + graphSpacer) >= 0) {
            w = width();
            int tw = (m_graphs[i]->showTitle() ? titleWidth : 0);

            queGraph(m_graphs[i], px + tw, py, width() - tw, h);

            if ((m_graphs.size() > 1) && m_showsplitter) {
                // draw the splitter handle
                painter.setPen(QColor(220, 220, 220, 255));
                painter.drawLine(0, py + h, w, py + h);
                painter.setPen(QColor(128, 128, 128, 255));
                painter.drawLine(0, py + h + 1, w, py + h + 1);
                painter.setPen(QColor(190, 190, 190, 255));
                painter.drawLine(0, py + h + 2, w, py + h + 2);
            }

        }

        py = ceil(py + h + graphSpacer);
    }

    //int thr=m_idealthreads;
#ifdef ENABLED_THREADED_DRAWING
    if (threaded) {
        for (int i = 0; i < m_idealthreads; i++) {
            masterlock->acquire(1);
            m_threads[i]->mutex.unlock();
        }

        // wait till all the threads are done
        // ask for all the CPU's back..
        masterlock->acquire(m_idealthreads);
        masterlock->release(m_idealthreads);
    } else {
#endif
        s = m_drawlist.size();

        for (int i = 0; i < s; i++) {
            gGraph *g = m_drawlist.at(0);
            m_drawlist.pop_front();
            g->paint(painter, QRegion(g->m_rect));
        }

#ifdef ENABLED_THREADED_DRAWING
    }
#endif
    //int elapsed=time.elapsed();
    //QColor col=Qt::black;

    //    lines->setSize(linesize);

     DrawTextQue(painter);
    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);

    return numgraphs > 0;
}

#ifdef BROKEN_OPENGL_BUILD
void gGraphView::paintEvent(QPaintEvent *)
#else
void gGraphView::paintGL()
#endif
{
#ifdef DEBUG_EFFICIENCY
    QElapsedTimer time;
    time.start();
#endif

    if (redrawtimer->isActive()) {
        redrawtimer->stop();
    }

    bool render_cube = false; //p_profile->appearance->animations(); // do something to

    if (width() <= 0) { return; }
    if (height() <= 0) { return; }

    // Create QPainter object, note this is only valid from paintGL events!
    QPainter painter(this);

    QRect bgrect(0, 0, width(), height());
    painter.fillRect(bgrect,QBrush(QColor(255,255,255)));

    bool graphs_drawn = true;

    lines_drawn_this_frame = 0;
    quads_drawn_this_frame = 0;
    strings_drawn_this_frame = 0;
    strings_cached_this_frame = 0;

    graphs_drawn = renderGraphs(painter);

    if (!graphs_drawn) { // No graphs drawn?
        int x, y;
        GetTextExtent(m_emptytext, x, y, bigfont);
        int tp;

        if (render_cube && this->isVisible()) {
//            renderCube(painter);

            tp = height() - (y / 2);
        } else {
            tp = height() / 2 + y / 2;
        }

        // Then display the empty text message
        QColor col = Qt::black;
        AddTextQue(m_emptytext, (width() / 2) - x / 2, tp, 0.0, col, bigfont);
    }
    DrawTextQue(painter);

    m_tooltip->paint(painter);

#ifdef DEBUG_EFFICIENCY
    const int rs = 10;
    static double ring[rs] = {0};
    static int rp = 0;

    // Show FPS and draw time
    if (m_showsplitter && p_profile->general->showDebug()) {
        QString ss;
        qint64 ela = time.nsecsElapsed();
        double ms = double(ela) / 1000000.0;
        ring[rp++] = 1000.0 / ms;
        rp %= rs;
        double v = 0;

        for (int i = 0; i < rs; i++) {
            v += ring[i];
        }

        double fps = v / double(rs);
        ss = "Debug Mode " + QString::number(fps, 'f', 1) + "fps "
                + QString::number(lines_drawn_this_frame, 'f', 0) + " lines "
//                + QString::number(quads_drawn_this_frame, 'f', 0) + " quads "
                + QString::number(strings_drawn_this_frame, 'f', 0) + " strings "
                + QString::number(strings_cached_this_frame, 'f', 0) + " cached ";

        int w, h;
        // this uses tightBoundingRect, which is different on Mac than it is on Windows & Linux.
        GetTextExtent(ss, w, h);
        QColor col = Qt::white;

        if (m_graphs.size() > 0) {
            painter.fillRect(width() - m_graphs[0]->marginRight(), 0, m_graphs[0]->marginRight(), w, QBrush(col));
        }
#ifndef Q_OS_MAC
        //   if (usePixmapCache()) xx+=4; else xx-=3;
#endif
        AddTextQue(ss, width(), w / 2, 90, QColor(Qt::black), defaultfont);
        DrawTextQue(painter);
    }
//    painter.setPen(Qt::lightGray);
//    painter.drawLine(0, 0, 0, height());
//    painter.drawLine(0, 0, width(), 0);
//    painter.setPen(Qt::darkGray);
    //painter.drawLine(width(), 0, width(), height());

#endif

    painter.end();

#ifndef BROKEN_OPENGL_BUILD
    swapBuffers();
#endif
    if (this->isVisible() && !graphs_drawn && render_cube) { // keep the cube spinning
        redrawtimer->setInterval(1000.0 / 50); // 50 FPS
        redrawtimer->setSingleShot(true);
        redrawtimer->start();
    }
}

void gGraphView::leaveEvent(QEvent * event)
{
    Q_UNUSED(event);
    if (m_metaselect) {
        m_metaselect = false;
        timedRedraw(0);
    }
    releaseKeyboard();
}


// For manual scrolling
void gGraphView::setOffsetY(int offsetY)
{
    if (m_offsetY != offsetY) {
        m_offsetY = offsetY;
        redraw(); //issue full redraw..
    }
}

// For manual X scrolling (not really needed)
void gGraphView::setOffsetX(int offsetX)
{
    if (m_offsetX != offsetX) {
        m_offsetX = offsetX;
        redraw(); //issue redraw
    }
}

void gGraphView::mouseMoveEvent(QMouseEvent *event)
{
    grabKeyboard();

    int x = event->x();
    int y = event->y();

    m_mouse = QPoint(x, y);

    if (m_sizer_dragging) { // Resize handle being dragged
        float my = y - m_sizer_point.y();
        //qDebug() << "Sizer moved vertically" << m_sizer_index << my*m_scaleY;
        float h = m_graphs[m_sizer_index]->height();
        h += my / m_scaleY;

        if (h > m_graphs[m_sizer_index]->minHeight()) {
            m_graphs[m_sizer_index]->setHeight(h);
            m_sizer_point.setX(x);
            m_sizer_point.setY(y);
            updateScrollBar();
            redraw();
        }

        return;
    }

    if (m_graph_dragging) { // Title bar being dragged to reorder
        gGraph *p;
        int yy = m_sizer_point.y();
        bool empty;

        if (y < yy) {

            for (int i = m_graph_index - 1; i >= 0; i--) {
                if (m_graphs[i]->isPinned() != m_graphs[m_graph_index]->isPinned()) {
                    // fix cursor
                    continue;
                }

                empty = m_graphs[i]->isEmpty() || (!m_graphs[i]->visible());
                // swapping upwards.
                int yy2 = yy - graphSpacer - m_graphs[i]->height() * m_scaleY;
                yy2 += m_graphs[m_graph_index]->height() * m_scaleY;

                if (y < yy2) {
                    //qDebug() << "Graph Reorder" << m_graph_index;
                    p = m_graphs[m_graph_index];
                    m_graphs[m_graph_index] = m_graphs[i];
                    m_graphs[i] = p;

                    if (!empty) {
                        m_sizer_point.setY(yy - graphSpacer - m_graphs[m_graph_index]->height()*m_scaleY);
                        redraw();
                    }

                    m_graph_index--;
                }

                if (!empty) { break; }

            }
        } else if (y > yy + graphSpacer + m_graphs[m_graph_index]->height()*m_scaleY) {
            // swapping downwards
            //qDebug() << "Graph Reorder" << m_graph_index;
            for (int i = m_graph_index + 1; i < m_graphs.size(); i++) {
                if (m_graphs[i]->isPinned() != m_graphs[m_graph_index]->isPinned()) {
                    //m_graph_dragging=false;
                    // fix cursor
                    continue;
                }

                empty = m_graphs[i]->isEmpty() || (!m_graphs[i]->visible());
                p = m_graphs[m_graph_index];
                m_graphs[m_graph_index] = m_graphs[i];
                m_graphs[i] = p;

                if (!empty) {
                    m_sizer_point.setY(yy + graphSpacer + m_graphs[m_graph_index]->height()*m_scaleY);
                    redraw();
                }

                m_graph_index++;

                if (!empty) { break; }
            }
        }

        return;
    }

    float py = 0, pinned_height = 0, h;
    bool done = false;

    // Do pinned graphs first
    for (int i = 0; i < m_graphs.size(); i++) {

        if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || !m_graphs[i]->isPinned()) {
            continue;
        }

        h = m_graphs[i]->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if (!((y >= py + m_graphs[i]->top) && (y < py + h - m_graphs[i]->bottom))) {
            if (m_graphs[i]->isSelected()) {
                m_graphs[i]->deselect();
                timedRedraw(150);
            }
        }

        // Update Mouse Cursor shape
        if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
            this->setCursor(Qt::SplitVCursor);
            done = true;
        } else if ((y >= py + 1) && (y < py + h)) {
            if (x >= titleWidth + 10) {
                this->setCursor(Qt::ArrowCursor);
            } else {
                this->setCursor(Qt::OpenHandCursor);
            }


            m_horiz_travel += qAbs(x - m_lastxpos) + qAbs(y - m_lastypos);
            m_lastxpos = x;
            m_lastypos = y;
            //           QPoint p(x,y);
            //           QMouseEvent e(event->type(),p,event->button(),event->buttons(),event->modifiers());

            m_graphs[i]->mouseMoveEvent(event);

            done = true;
        }

        py += h + graphSpacer;

    }

    py = -m_offsetY;
    py += pinned_height;

    // Propagate mouseMove events to relevant graphs
    if (!done)
        for (int i = 0; i < m_graphs.size(); i++) {

            if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || m_graphs[i]->isPinned()) {
                continue;
            }

            h = m_graphs[i]->height() * m_scaleY;

            if (py > height()) {
                break;    // we are done.. can't draw anymore
            }

            if (!((y >= py + m_graphs[i]->top) && (y < py + h - m_graphs[i]->bottom))) {
                if (m_graphs[i]->isSelected()) {
                    m_graphs[i]->deselect();
                    timedRedraw(150);
                }
            }

            // Update Mouse Cursor shape
            if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
                this->setCursor(Qt::SplitVCursor);
            } else if ((y >= py + 1) && (y < py + h)) {
                if (x >= titleWidth + 10) {
                    this->setCursor(Qt::ArrowCursor);
                } else {
                    this->setCursor(Qt::OpenHandCursor);
                }


                m_horiz_travel += qAbs(x - m_lastxpos) + qAbs(y - m_lastypos);
                m_lastxpos = x;
                m_lastypos = y;
                gGraph *g = m_graphs[i];
                if (g) {
                    g->mouseMoveEvent(event);
                }

            }

            /*            else if (!m_button_down && (y >= py) && (y < py+m_graphs[i]->top)) {
                            // Mouse cursor is in top graph margin.
                        } else if (!m_button_down && (y >= py+h-m_graphs[i]->bottom) && (y <= py+h)) {
                            // Mouse cursor is in bottom grpah margin.
                        } else if (m_button_down || ((y >= py+m_graphs[i]->top) && (y < py + h-m_graphs[i]->bottom))) {
                            if (m_button_down || (x >= titleWidth+10)) { //(gYAxis::Margin-5)
                                this->setCursor(Qt::ArrowCursor);
                                m_horiz_travel+=qAbs(x-m_lastxpos)+qAbs(y-m_lastypos);
                                m_lastxpos=x;
                                m_lastypos=y;
                                QPoint p(x-titleWidth,y-py);
                                QMouseEvent e(event->type(),p,event->button(),event->buttons(),event->modifiers());

                                m_graphs[i]->mouseMoveEvent(&e);
                                if (!m_button_down && (x<=titleWidth+(gYAxis::Margin-5))) {
                                    //qDebug() << "Hovering over" << m_graphs[i]->title();
                                    if (m_graphsbytitle[STR_TR_EventFlags]==m_graphs[i]) {
                                        QVector<Layer *> & layers=m_graphs[i]->layers();
                                        gFlagsGroup *fg;
                                        for (int i=0;i<layers.size();i++) {
                                            if ((fg=dynamic_cast<gFlagsGroup *>(layers[i]))!=nullptr) {
                                                float bh=fg->barHeight();
                                                int count=fg->count();
                                                float yp=py+m_graphs[i]->marginTop();
                                                    yp=y-yp;
                                                float th=(float(count)*bh);
                                                if (yp>=0 && yp<th) {
                                                    int i=yp/bh;
                                                    if (i<count) {
                                                        ChannelID code=fg->visibleLayers()[i]->code();
                                                        QString ttip=schema::channel[code].description();
                                                        m_tooltip->display(ttip,x,y-20,p_profile->general->tooltipTimeout());
                                                        redraw();
                                                        //qDebug() << code << ttip;
                                                    }
                                                }

                                                break;
                                            }
                                        }
                                    } else {
                                        if (!m_graphs[i]->units().isEmpty()) {
                                            m_tooltip->display(m_graphs[i]->units(),x,y-20,p_profile->general->tooltipTimeout());
                                            redraw();
                                        }
                                    }
                                }
                            } else {

                                this->setCursor(Qt::OpenHandCursor);
                            }

                        } */

            //   }
            py += h + graphSpacer;
        }

}

void gGraphView::mousePressEvent(QMouseEvent *event)
{
    int x = event->x();
    int y = event->y();

    float h, pinned_height = 0, py = 0;

    bool done = false;

    // first handle pinned graphs.
    // Calculate total height of all pinned graphs
    for (int i = 0; i < m_graphs.size(); i++) {
        gGraph *g = m_graphs[i];

        if (!g || g->isEmpty() || !g->visible() || !g->isPinned()) {
            continue;
        }

        h = g->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;
        }

        if ((py + h + graphSpacer) >= 0) {
            if ((y >= py + h - 1) && (y <= py + h + graphSpacer)) {
                this->setCursor(Qt::SplitVCursor);
                m_sizer_dragging = true;
                m_sizer_index = i;
                m_sizer_point.setX(x);
                m_sizer_point.setY(y);
                done = true;
            } else if ((y >= py) && (y < py + h)) {
                //qDebug() << "Clicked" << i;
                if (x < titleWidth + 20) {
                    // clicked on title to drag graph..
                    // Note: reorder has to be limited to pinned graphs.
                    m_graph_dragging = true;
                    m_tooltip->cancel();

                    timedRedraw(50);
                    m_graph_index = i;
                    m_sizer_point.setX(x);
                    m_sizer_point.setY(py); // point at top of graph..
                    this->setCursor(Qt::ClosedHandCursor);
                } else

                {
                    if (m_metaselect) {
                        if (m_selected_graph) {
                            m_selected_graph->m_selecting_area = false;
                        }
                    }
                    // send event to graph..
                    m_point_clicked = QPoint(event->x(), event->y());

                    //QMouseEvent e(event->type(),m_point_clicked,event->button(),event->buttons(),event->modifiers());


                    m_button_down = true;
                    m_metaselect = event->modifiers() && Qt::AltModifier;
                    m_horiz_travel = 0;
                    m_graph_index = i;
                    m_selected_graph = g;
                    g->mousePressEvent(event);
                }

                done = true;
            }

        }

        py += h + graphSpacer;
    }



    // then handle the remainder...
    py = -m_offsetY;
    py += pinned_height;

    if (!done)
        for (int i = 0; i < m_graphs.size(); i++) {
            gGraph * g = m_graphs[i];
            if (!g) continue;

            if (!g || g->isEmpty() || !g->visible() || g->isPinned()) { continue; }

            h = g->height() * m_scaleY;

            if (py > height()) {
                break;
            }

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py + h - 1) && (y <= py + h + graphSpacer)) {
                    this->setCursor(Qt::SplitVCursor);
                    m_sizer_dragging = true;
                    m_sizer_index = i;
                    m_sizer_point.setX(x);
                    m_sizer_point.setY(y);
                    //qDebug() << "Sizer clicked" << i;
                } else if ((y >= py) && (y < py + h)) {
                    //qDebug() << "Clicked" << i;
                    if (x < titleWidth + 20) { // clicked on title to drag graph..
                        m_graph_dragging = true;
                        m_tooltip->cancel();
                        redraw();
                        m_graph_index = i;
                        m_sizer_point.setX(x);
                        m_sizer_point.setY(py); // point at top of graph..
                        this->setCursor(Qt::ClosedHandCursor);
                    }

                    {
                        if (m_metaselect) {
                            if (m_selected_graph) {
                                m_selected_graph->m_selecting_area = false;
                            }
                        }
                        // send event to graph..
                        m_point_clicked = QPoint(event->x(), event->y());
                        //QMouseEvent e(event->type(),m_point_clicked,event->button(),event->buttons(),event->modifiers());
                        m_button_down = true;
                        m_metaselect = event->modifiers() && Qt::AltModifier;

                        m_horiz_travel = 0;
                        m_graph_index = i;
                        m_selected_graph = g;
                        g->mousePressEvent(event);
                    }
                }

            }

            py += h + graphSpacer;
        }
}

void gGraphView::mouseReleaseEvent(QMouseEvent *event)
{

    int x = event->x();
    int y = event->y();

    float h, py = 0, pinned_height = 0;
    bool done = false;


    // Copy to a local variable to make sure this gets cleared
    bool button_down = m_button_down;
    m_button_down = false;

    // Handle pinned graphs first
    for (int i = 0; i < m_graphs.size(); i++) {
        gGraph *g = m_graphs[i];

        if (!g || g->isEmpty() || !g->visible() || !g->isPinned()) {
            continue;
        }

        h = g->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
            this->setCursor(Qt::SplitVCursor);
            done = true;
        } else if ((y >= py + 1) && (y <= py + h)) {

            //            if (!m_sizer_dragging && !m_graph_dragging) {
            //               g->mouseReleaseEvent(event);
            //            }

            if (x >= titleWidth + 10) {
                this->setCursor(Qt::ArrowCursor);
            } else {
                this->setCursor(Qt::OpenHandCursor);
            }

            done = true;
        }

        py += h + graphSpacer;
    }

    // Now do the unpinned ones
    py = -m_offsetY;
    py += pinned_height;

    if (done)
        for (int i = 0; i < m_graphs.size(); i++) {
            gGraph *g = m_graphs[i];

            if (!g || g->isEmpty() || !g->visible() || g->isPinned()) {
                continue;
            }

            h = g->height() * m_scaleY;

            if (py > height()) {
                break;    // we are done.. can't draw anymore
            }

            if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
                this->setCursor(Qt::SplitVCursor);
            } else if ((y >= py + 1) && (y <= py + h)) {

                //            if (!m_sizer_dragging && !m_graph_dragging) {
                //                g->mouseReleaseEvent(event);
                //            }

                if (x >= titleWidth + 10) {
                    this->setCursor(Qt::ArrowCursor);
                } else {
                    this->setCursor(Qt::OpenHandCursor);
                }
            }

            py += h + graphSpacer;
        }

    if (m_sizer_dragging) {
        m_sizer_dragging = false;
        return;
    }

    if (m_graph_dragging) {
        m_graph_dragging = false;

        // not sure why the cursor code doesn't catch this..
        if (x >= titleWidth + 10) {
            this->setCursor(Qt::ArrowCursor);
        } else {
            this->setCursor(Qt::OpenHandCursor);
        }

        return;
    }

    // The graph that got the button press gets the release event
    if (button_down) {
//        m_button_down = false;
        m_metaselect = event->modifiers() & Qt::AltModifier;
        saveHistory();

        if (m_metaselect) {
            m_point_released = event->pos();
        } else {
            if ((m_graph_index >= 0) && (m_graphs[m_graph_index])) {
                m_graphs[m_graph_index]->mouseReleaseEvent(event);
            }
        }
    }
}

void gGraphView::keyReleaseEvent(QKeyEvent *event)
{
    if (m_metaselect && !(event->modifiers() & Qt::AltModifier)) {
        QMouseEvent mevent(QEvent::MouseButtonRelease, m_point_released, Qt::LeftButton, Qt::LeftButton, event->modifiers());
        if (m_graph_index>=0) {
            m_graphs[m_graph_index]->mouseReleaseEvent(&mevent);
        }

        m_metaselect = false;

        timedRedraw(50);
    }
    if (event->key() == Qt::Key_Escape) {
        if (history.size() > 0) {
            SelectionHistoryItem h = history.takeFirst();
            SetXBounds(h.minx, h.maxx);

            // could Forward push this to another list?
        } else {
            ResetBounds();

        }
        return;
    }
#ifdef BROKEN_OPENGL_BUILD
        QWidget::keyReleaseEvent(event);
#else
        QGLWidget::keyReleaseEvent(event);
#endif
}


void gGraphView::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event); // signal missing.. a qt change might "fix" this if we are not careful.

    int x = event->x();
    int y = event->y();

    float h, py = 0, pinned_height = 0;
    bool done = false;

    // Handle pinned graphs first
    for (int i = 0; i < m_graphs.size(); i++) {
        gGraph *g = m_graphs[i];
        if (!g || g->isEmpty() || !g->visible() || !g->isPinned()) {
            continue;
        }

        h = g->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((py + h + graphSpacer) >= 0) {
            if ((y >= py) && (y <= py + h)) {
                if (x < titleWidth) {
                    // What to do when double clicked on the graph title ??

                    g->mouseDoubleClickEvent(event);
                    // pin the graph??
                    g->setPinned(false);
                    redraw();
                } else {
                    // send event to graph..
                    g->mouseDoubleClickEvent(event);
                }

                done = true;
            } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                // What to do when double clicked on the resize handle?
                done = true;
            }
        }

        py += h;
        py += graphSpacer; // do we want the extra spacer down the bottom?
    }


    py = -m_offsetY;
    py += pinned_height;

    if (!done) // then handle unpinned graphs
        for (int i = 0; i < m_graphs.size(); i++) {
            gGraph *g = m_graphs[i];
            if (!g || g->isEmpty() || !g->visible() || g->isPinned()) {
                continue;
            }

            h = g->height() * m_scaleY;

            if (py > height()) {
                break;
            }

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py) && (y <= py + h)) {
                    if (x < titleWidth) {
                        // What to do when double clicked on the graph title ??
                        g->mouseDoubleClickEvent(event);

                        g->setPinned(true);
                        redraw();
                    } else {
                        // send event to graph..
                        g->mouseDoubleClickEvent(event);
                    }
                } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                    // What to do when double clicked on the resize handle?
                }

            }

            py += h;
            py += graphSpacer; // do we want the extra spacer down the bottom?
        }
}

void gGraphView::wheelEvent(QWheelEvent *event)
{
    // Hmm.. I could optionalize this to change mousewheel behaviour without affecting the scrollbar now..

    if (m_button_down)
        return;

    if ((event->modifiers() & Qt::ControlModifier)) {
        int x = event->x();
        int y = event->y();

        float py = -m_offsetY;
        float h;

        for (int i = 0; i < m_graphs.size(); i++) {
            gGraph *g = m_graphs[i];
            if (!g || g->isEmpty() || !g->visible()) { continue; }

            h = g->height() * m_scaleY;

            if (py > height()) {
                break;
            }

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py) && (y <= py + h)) {
                    if (x < titleWidth) {
                        // What to do when ctrl+wheel is used on the graph title ??
                    } else {
                        // send event to graph..
                        g->wheelEvent(event);
                    }
                } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                    // What to do when the wheel is used on the resize handle?
                }

            }

            py += h;
            py += graphSpacer; // do we want the extra spacer down the bottom?
        }
    } else {
        int scrollDampening = p_profile->general->scrollDampening();

        if (event->orientation() == Qt::Vertical) { // Vertical Scrolling
            if (horizScrollTime.elapsed() < scrollDampening) {
                return;
            }

            if (m_scrollbar)
                m_scrollbar->SendWheelEvent(event); // Just forwarding the event to scrollbar for now..
            m_tooltip->cancel();
            vertScrollTime.start();
        } else { //Horizontal Panning
            // (This is a total pain in the butt on MacBook touchpads..)

            if (vertScrollTime.elapsed() < scrollDampening) {
                return;
            }

            horizScrollTime.start();
            gGraph *g = nullptr;
            int group = 0;

            // Pick the first valid graph in the primary group
            for (int i = 0; i < m_graphs.size(); i++) {
                if (!m_graphs[i]) continue;
                if (m_graphs[i]->group() == group) {
                    if (!m_graphs[i]->isEmpty() && m_graphs[i]->visible()) {
                        g = m_graphs[i];
                        break;
                    }
                }
            }

            if (!g) {
                // just pick any graph then
                for (int i = 0; i < m_graphs.size(); i++) {
                    if (!m_graphs[i]) continue;
                    if (!m_graphs[i]->isEmpty()) {
                        g = m_graphs[i];
                        group = g->group();
                        break;
                    }
                }
            }

            if (!g) { return; }

            double xx = (g->max_x - g->min_x);
            double zoom = 240.0;

            int delta = event->delta();

            if (delta > 0) {
                g->min_x -= (xx / zoom) * (float)abs(delta);
            } else {
                g->min_x += (xx / zoom) * (float)abs(delta);
            }

            g->max_x = g->min_x + xx;

            if (g->min_x < g->rmin_x) {
                g->min_x = g->rmin_x;
                g->max_x = g->rmin_x + xx;
            }

            if (g->max_x > g->rmax_x) {
                g->max_x = g->rmax_x;
                g->min_x = g->max_x - xx;
            }

            saveHistory();
            SetXBounds(g->min_x, g->max_x, group);
        }
    }
}

void gGraphView::getSelectionTimes(qint64 & start, qint64 & end)
{
    if (m_graph_index >= 0) {
        gGraph *g = m_graphs[m_graph_index];
        if (!g) {
            start = 0;
            end = 0;
            return;
        }
        int x1 = g->m_selection.x() + titleWidth;
        int x2 = x1 + g->m_selection.width();
        start = g->screenToTime(x1);
        end = g->screenToTime(x2);
    }
}

void gGraphView::keyPressEvent(QKeyEvent *event)
{
    m_metaselect = event->modifiers() & Qt::AltModifier;
    if (m_metaselect && ((event->key() == Qt::Key_B) || (event->key() == 8747))) {
        if (mainwin->getDaily()->graphView() == this) {
            if (m_graph_index >= 0) {
                m_metaselect=false;
                qint64 start,end;
                getSelectionTimes(start,end);
                QDateTime d1 = QDateTime::fromMSecsSinceEpoch(start);
        //        QDateTime d2 = QDateTime::fromMSecsSinceEpoch(end);

                mainwin->getDaily()->addBookmark(start, end, QString("Bookmark at %1").arg(d1.time().toString("HH:mm:ss")));
                m_graphs[m_graph_index]->cancelSelection();
                m_graph_index = -1;
                timedRedraw(0);
            }
            event->accept();
            return;
        }
    }

    if ((m_metaselect) && (event->key() >= Qt::Key_0) && (event->key() <= Qt::Key_9)) {
        int bk = (int)event->key()-Qt::Key_0;
        m_metaselect = false;

        timedRedraw(30);
    }

    if (event->key() == Qt::Key_F3) {
        p_profile->appearance->setLineCursorMode(!p_profile->appearance->lineCursorMode());
        timedRedraw(0);
    }
    if ((event->key() == Qt::Key_F1)) {
        dumpInfo();
    }

    if (event->key() == Qt::Key_Tab) {
        event->ignore();
        return;
    }

    if (event->key() == Qt::Key_PageUp) {
        if (m_scrollbar) {
            m_offsetY -= p_profile->appearance->graphHeight() * 3 * m_scaleY;
            m_scrollbar->setValue(m_offsetY);
            m_offsetY = m_scrollbar->value();
            redraw();
        }
        return;
    } else if (event->key() == Qt::Key_PageDown) {
        if (m_scrollbar) {
            m_offsetY += p_profile->appearance->graphHeight() * 3 * m_scaleY; //p_profile->appearance->graphHeight();

            if (m_offsetY < 0) { m_offsetY = 0; }

            m_scrollbar->setValue(m_offsetY);
            m_offsetY = m_scrollbar->value();
            redraw();
        }
        return;
        //        redraw();
    }

    gGraph *g = nullptr;
    int group = 0;

    // Pick the first valid graph in the primary group
    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->group() == group) {
            if (!m_graphs[i]->isEmpty() && m_graphs[i]->visible()) {
                g = m_graphs[i];
                break;
            }
        }
    }

    if (!g) {
        for (int i = 0; i < m_graphs.size(); i++) {
            if (!m_graphs[i]->isEmpty()) {
                g = m_graphs[i];
                group = g->group();
                break;
            }
        }
    }

    if (!g) { return; }

    g->keyPressEvent(event);

    if (event->key() == Qt::Key_Left) {
        double xx = g->max_x - g->min_x;
        double zoom = 8.0;

        if (event->modifiers() & Qt::ControlModifier) { zoom /= 4; }

        g->min_x -= xx / zoom;;
        g->max_x = g->min_x + xx;

        if (g->min_x < g->rmin_x) {
            g->min_x = g->rmin_x;
            g->max_x = g->rmin_x + xx;
        }

        saveHistory();
        SetXBounds(g->min_x, g->max_x, group);
    } else if (event->key() == Qt::Key_Right) {
        double xx = g->max_x - g->min_x;
        double zoom = 8.0;

        if (event->modifiers() & Qt::ControlModifier) { zoom /= 4; }

        g->min_x += xx / zoom;
        g->max_x = g->min_x + xx;

        if (g->max_x > g->rmax_x) {
            g->max_x = g->rmax_x;
            g->min_x = g->rmax_x - xx;
        }

        saveHistory();
        SetXBounds(g->min_x, g->max_x, group);
    } else if (event->key() == Qt::Key_Up) {
        float zoom = 0.75F;

        if (event->modifiers() & Qt::ControlModifier) { zoom /= 1.5; }

        g->ZoomX(zoom, 0); // zoom in.
    } else if (event->key() == Qt::Key_Down) {
        float zoom = 1.33F;

        if (event->modifiers() & Qt::ControlModifier) { zoom *= 1.5; }

        g->ZoomX(zoom, 0); // Zoom out
    }

    //qDebug() << "Keypress??";
}


void gGraphView::setDay(Day *day)
{

    m_day = day;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]) m_graphs[i]->setDay(day);
    }

    ResetBounds(false);
}
bool gGraphView::isEmpty()
{
    bool res = true;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (!m_graphs.at(i)->isEmpty()) {
            res = false;
            break;
        }
    }

    return res;
}

void gGraphView::refreshTimeout()
{
    if (this->isVisible()) {
        redraw();
    }
}
void gGraphView::timedRedraw(int ms)
{

    if (timer->isActive()) {
        int m = timer->remainingTime();
        if (m > ms) {
            timer->stop();
        } else return;
    }
    timer->setSingleShot(true);
    timer->start(ms);
}
void gGraphView::resetLayout()
{
    int default_height = p_profile->appearance->graphHeight();

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]) m_graphs[i]->setHeight(default_height);
    }

    updateScale();
    redraw();
}
void gGraphView::deselect()
{
    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]) m_graphs[i]->deselect();
    }
}

const quint32 gvmagic = 0x41756728;
const quint16 gvversion = 3;

void gGraphView::SaveSettings(QString title)
{
    QString filename = p_profile->Get("{DataFolder}/") + title.toLower() + ".shg";
    QFile f(filename);
    f.open(QFile::WriteOnly);
    QDataStream out(&f);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)gvmagic;
    out << (quint16)gvversion;

    out << (qint16)size();

    for (qint16 i = 0; i < size(); i++) {
        if (!m_graphs[i]) continue;

        out << m_graphs[i]->name();
        out << m_graphs[i]->height();
        out << m_graphs[i]->visible();
        out << m_graphs[i]->RecMinY();
        out << m_graphs[i]->RecMaxY();
        out << m_graphs[i]->zoomY();
        out << (bool)m_graphs[i]->isPinned();
    }

    f.close();
}

bool gGraphView::LoadSettings(QString title)
{
    QString filename = p_profile->Get("{DataFolder}/") + title.toLower() + ".shg";
    QFile f(filename);

    if (!f.exists()) { return false; }

    f.open(QFile::ReadOnly);
    QDataStream in(&f);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 t1;
    quint16 t2;

    in >> t1;

    if (t1 != gvmagic) {
        qDebug() << "gGraphView" << title << "settings magic doesn't match" << t1 << gvmagic;
        return false;
    }

    in >> t2;

    if (t2 < gvversion) {
        qDebug() << "gGraphView" << title << "settings will be upgraded.";
    }

    qint16 siz;
    in >> siz;
    QString name;
    float hght;
    bool vis;
    EventDataType recminy, recmaxy;
    bool pinned;

    short zoomy = 0;

    QVector<gGraph *> neworder;
    QHash<QString, gGraph *>::iterator gi;

    for (int i = 0; i < siz; i++) {
        in >> name;

        in >> hght;
        in >> vis;
        in >> recminy;
        in >> recmaxy;

        if (gvversion >= 1) {
            in >> zoomy;
        }

        if (gvversion >= 2) {
            in >> pinned;
        }

        gGraph *g = nullptr;

        if (t2 <= 2) {
            // Names were stored as translated strings, so look up title instead.
            g = nullptr;
            for (int z=0; z<m_graphs.size(); ++z) {
                if (m_graphs[z]->title() == name) {
                    g=m_graphs[z];
                    break;
                }
            }
        } else {
            gi = m_graphsbyname.find(name);
            if (gi == m_graphsbyname.end()) {
                qDebug() << "Graph" << name << "has been renamed or removed";
            } else {
                g = gi.value();
            }
        }
        if (g) {
            neworder.push_back(g);
            g->setHeight(hght);
            g->setVisible(vis);
            g->setRecMinY(recminy);
            g->setRecMaxY(recmaxy);
            g->setZoomY(zoomy);
            g->setPinned(pinned);
        }
    }

    if (neworder.size() == m_graphs.size()) {
        m_graphs = neworder;
    }

    f.close();
    updateScale();
    return true;
}

gGraph *gGraphView::findGraph(QString name)
{
    QHash<QString, gGraph *>::iterator i = m_graphsbyname.find(name);

    if (i == m_graphsbyname.end()) { return nullptr; }

    return i.value();
}

gGraph *gGraphView::findGraphTitle(QString title)
{
    for (int i=0; i< m_graphs.size(); ++i) {
        if (m_graphs[i]->title() == title) return m_graphs[i];
    }
    return nullptr;
}

int gGraphView::visibleGraphs()
{
    int cnt = 0;

    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->visible() && !m_graphs[i]->isEmpty()) { cnt++; }
    }

    return cnt;
}

void gGraphView::redraw()
{
#ifdef BROKEN_OPENGL_BUILD
    repaint();
#else
    updateGL();
#endif
}
