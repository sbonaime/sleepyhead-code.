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
#include <QGLFramebufferObject>
#include <QGLPixelBuffer>
#include <QLabel>
#include <QPixmapCache>
#include <QTimer>

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

// for profiling purposes, a count of lines drawn in a single frame
int lines_drawn_this_frame = 0;
int quads_drawn_this_frame = 0;

extern QLabel *qstatus2;

gToolTip::gToolTip(gGraphView *graphview)
    : m_graphview(graphview)
{

    m_pos.setX(0);
    m_pos.setY(0);
    m_visible = false;
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

void gToolTip::display(QString text, int x, int y, int timeout)
{
    if (timeout <= 0) {
        timeout = p_profile->general->tooltipTimeout();
    }

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

void gToolTip::paint()     //actually paints it.
{
    if (!m_visible) { return; }

    int x = m_pos.x();
    int y = m_pos.y();

    QPainter painter(m_graphview);

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

    lines_drawn_this_frame += 4;
    quads_drawn_this_frame += 1;

    QBrush brush(QColor(255, 255, 128, 230));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(QColor(0, 0, 0, 255));

    painter.drawRoundedRect(rect, 5, 5);
    painter.setBrush(Qt::black);

    painter.setFont(*defaultfont);

    painter.drawText(rect, Qt::AlignCenter, m_text);

    painter.end();
}

void gToolTip::timerDone()
{
    m_visible = false;
    m_graphview->redraw();
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
                g->paint(g->m_lastbounds.x(), g->m_lastbounds.y(), g->m_lastbounds.width(),
                         g->m_lastbounds.height());
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
    //for (int i=0;i<m_graphs.size();i++) {
    //delete m_graphs[i];
    //}
    m_graphs.clear();
    m_graphsbytitle.clear();
}
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

// Render all qued text via QPainter method
void gGraphView::DrawTextQue(QPainter &painter)
{
    int w, h;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    int dpr = devicePixelRatio();
#endif

    for (int i = 0; i < m_textque_items; i++) {
        TextQue &q = m_textque[i];
        painter.setBrush(q.color);
        painter.setRenderHint(QPainter::TextAntialiasing, q.antialias);
        QFont font = *q.font;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        int fs = font.pointSize();

        if (fs > 0) {
            font.setPointSize(fs * dpr);
        } else {
            font.setPixelSize(font.pixelSize()*dpr);
        }

#endif
        painter.setFont(font);

        if (q.angle == 0) { // normal text

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            painter.drawText(q.x * dpr, q.y * dpr, q.text);
#else
            painter.drawText(q.x, q.y, q.text);
#endif
        } else { // rotated text
            w = painter.fontMetrics().width(q.text);
            h = painter.fontMetrics().xHeight() + 2;

            painter.translate(q.x, q.y);
            painter.rotate(-q.angle);
            painter.drawText(floor(-w / 2.0), floor(-h / 2.0), q.text);
            painter.rotate(+q.angle);
            painter.translate(-q.x, -q.y);
        }

        q.text.clear();
    }

    m_textque_items = 0;
}

QImage gGraphView::pbRenderPixmap(int w, int h)
{
    QImage pm = QImage();
    QGLFormat pbufferFormat = format();
    QGLPixelBuffer pbuffer(w, h, pbufferFormat, this);

    if (pbuffer.isValid()) {
        pbuffer.makeCurrent();
        initializeGL();
        resizeGL(w, h);
        glClearColor(255, 255, 255, 255);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderGraphs();
        glFlush();
        pm = pbuffer.toImage();
        pbuffer.doneCurrent();
        QPainter painter(&pm);
        DrawTextQue(painter);
        painter.end();
    }

    return pm;
}

QImage gGraphView::fboRenderPixmap(int w, int h)
{
    QImage pm = QImage();

    if (fbo_unsupported) {
        return pm;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    float dpr = devicePixelRatio();
    w *= dpr;
    h *= dpr;
#endif

    if ((w > max_fbo_width) || (h > max_fbo_height)) {
        qWarning() <<
                   "gGraphView::fboRenderPixmap called with dimensiopns exceeding maximum frame buffer object size";
        return pm;
    }

    if (!fbo) {
        fbo = new QGLFramebufferObject(max_fbo_width, max_fbo_height,
                                       QGLFramebufferObject::Depth); //NoAttachment);
    }

    if (fbo && fbo->isValid()) {
        makeCurrent();

        if (fbo->bind()) {
            initializeGL();
            resizeGL(w, h);
            glClearColor(255, 255, 255, 255);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderGraphs(); // render graphs sans text
            glFlush();
            fbo->release();
            pm = fbo->toImage().copy(0, max_fbo_height - h, w, h);
            doneCurrent();

            QPainter painter(&pm);
            DrawTextQue(painter); //Just use this on mac to
            painter.end();

        }
    } else {
        delete fbo;
        fbo = nullptr;
        fbo_unsupported = true;
    }

    return pm;
}

gGraphView::gGraphView(QWidget *parent, gGraphView *shared)
  : QGLWidget(QGLFormat(QGL::Rgba | QGL::DoubleBuffer | QGL::NoOverlay), parent, shared),
    m_offsetY(0), m_offsetX(0), m_scaleY(1.0), m_scrollbar(nullptr)
{
    m_shared = shared;
    m_sizer_index = m_graph_index = 0;
    m_textque_items = 0;
    m_button_down = m_graph_dragging = m_sizer_dragging = false;
    m_lastypos = m_lastxpos = 0;
    m_horiz_travel = 0;
    pixmap_cache_size = 0;
    m_minx = m_maxx = 0;
    m_day = nullptr;
    m_selected_graph = nullptr;
    cubetex = 0;

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

    lines = new gVertexBuffer(100000, GL_LINES); // big fat shared line list
    backlines = new gVertexBuffer(10000, GL_LINES); // big fat shared line list
    quads = new gVertexBuffer(1024, GL_QUADS); // big fat shared line list
    quads->forceAntiAlias(true);
    frontlines = new gVertexBuffer(20000, GL_LINES);

    //vlines=new gVertexBuffer(20000,GL_LINES);

    //stippled->setSize(1.5);
    //stippled->forceAntiAlias(false);
    //lines->setSize(1.5);
    //backlines->setSize(1.5);

    setFocusPolicy(Qt::StrongFocus);
    m_showsplitter = true;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(refreshTimeout()));
    print_scaleY = print_scaleX = 1.0;

    redrawtimer = new QTimer(this);
    //redrawtimer->setInterval(80);
    //redrawtimer->start();
    connect(redrawtimer, SIGNAL(timeout()), SLOT(updateGL()));

    //cubeimg.push_back(images["brick"]);
    //cubeimg.push_back(images[""]);

    m_fadingOut = false;
    m_fadingIn = false;
    m_inAnimation = false;
    m_limbo = false;
    m_fadedir = false;
    m_blockUpdates = false;
    use_pixmap_cache = true;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    m_dpr = this->windowHandle()->devicePixelRatio();
#else
    m_dpr = 1;
#endif
}

gGraphView::~gGraphView()
{
    timer->stop();
    redrawtimer->stop();

    disconnect(redrawtimer, 0, 0, 0);
    disconnect(timer, 0, 0, 0);

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

    QHash<QString, myPixmapCache *>::iterator it;

    for (it = pixmap_cache.begin(); it != pixmap_cache.end(); it++) {
        delete(*it);
    }

    pixmap_cache.clear();

    delete m_tooltip;
    m_graphs.clear();

    delete frontlines;
    delete lines;
    delete backlines;
    delete quads;

    if (m_scrollbar) {
        this->disconnect(m_scrollbar, SIGNAL(sliderMoved(int)), 0, 0);
    }

    delete timer;
    delete redrawtimer;
}

bool gGraphView::usePixmapCache()
{
    //use_pixmap_cache is an overide setting
    return use_pixmap_cache & PROFILE.appearance->usePixmapCaching();
}


void gGraphView::DrawTextQue()
{
    const qint64 expire_after_ms = 4000; // expire string pixmaps after this many milliseconds
    //const qint64 under_limit_cache_bonus=30000; // If under the limit, give a bonus to the millisecond timeout.
    const qint32 max_pixmap_cache = 4 *
                                    1048576; // Maximum size of pixmap cache (it can grow over this, but only temporarily)

    quint64 ti = 0, exptime = 0;
    int w, h;
    QHash<QString, myPixmapCache *>::iterator it;
    QPainter painter;

    // Purge the Pixmap cache of any old text strings
    if (usePixmapCache()) {
        // Current time in milliseconds since epoch.
        ti = QDateTime::currentDateTime().toMSecsSinceEpoch();

        if (pixmap_cache_size >
                max_pixmap_cache) { // comment this if block out to only cleanup when past the maximum cache size
            // Expire any strings not used
            QList<QString> expire;

            exptime = expire_after_ms;

            // Uncomment the next line to allow better use of pixmap cache memory
            //if (pixmap_cache_size < max_pixmap_cache) exptime+=under_limit_cache_bonus;

            for (it = pixmap_cache.begin(); it != pixmap_cache.end(); it++) {
                if ((*it)->last_used < (ti - exptime)) {
                    expire.push_back(it.key());
                }
            }


            // TODO: Force expiry if over an upper memory threshold.. doesn't appear to be necessary.

            for (int i = 0; i < expire.count(); i++) {
                const QString key = expire.at(i);
                // unbind the texture
                myPixmapCache *pc = pixmap_cache[key];
                deleteTexture(pc->textureID);
                QImage &pm = pc->image;
                pixmap_cache_size -= pm.width() * pm.height() * (pm.depth() / 8);
                // free the pixmap
                //delete pc->pixmap;

                // free the myPixmapCache object
                delete pc;

                // pull the dead record from the cache.
                pixmap_cache.remove(key);
            }
        }
    }

    //glPushAttrib(GL_COLOR_BUFFER_BIT);
    painter.begin(this);

    int buf = 4;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    float dpr = devicePixelRatio();
    buf *= dpr;
#endif

    // process the text drawing queue
    for (int i = 0; i < m_textque_items; i++) {
        TextQue &q = m_textque[i];

        // can do antialiased text via texture cache fine on mac
        if (usePixmapCache()) {
            // Generate the pixmap cache "key"
            QString hstr = QString("%4:%5:%6%7").arg(q.text).arg(q.color.name()).arg(q.font->key()).arg(
                               q.antialias);

            QImage pm;

            it = pixmap_cache.find(hstr);
            myPixmapCache *pc = nullptr;

            if (it != pixmap_cache.end()) {
                pc = (*it);

            } else {
                // not found.. create the image and store it in a cache

                //This is much slower than other text rendering methods, but caching more than makes up for the speed decrease.
                pc = new myPixmapCache;
                pc->last_used = ti; // set the last_used value.

                QFontMetrics fm(*q.font);
                QRect rect = fm.boundingRect(q.text);
                w = fm.width(q.text);
                h = fm.height();

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
                w *= dpr;
                h *= dpr;
#endif

                rect.setWidth(w);
                rect.setHeight(h);

                pm = QImage(w + buf, h + buf, QImage::Format_ARGB32_Premultiplied);

                pm.fill(Qt::transparent);

                QPainter imgpainter(&pm);

                QBrush b(q.color);
                imgpainter.setBrush(b);

                QFont font = *q.font;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
                int fs = font.pointSize();

                if (fs > 0) {
                    font.setPointSize(fs * dpr);
                } else {
                    font.setPixelSize(font.pixelSize()*dpr);
                }

#endif
                imgpainter.setFont(font);

                imgpainter.setRenderHint(QPainter::TextAntialiasing, q.antialias);
                imgpainter.drawText(buf / 2, h, q.text);
                imgpainter.end();

                pc->image = pm;
                pixmap_cache_size += pm.width() * pm.height() * (pm.depth() / 8);
                pixmap_cache[hstr] = pc;

            }

            if (pc) {
                pc->last_used = ti;
                int h = pc->image.height();
                int w = pc->image.width();

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
                h /= dpr;
                w /= dpr;
#endif

                if (q.angle != 0) {
                    float xxx = q.x - h - (h / 2);
                    float yyy = q.y + w / 2 + buf / 2;

                    painter.translate(xxx, yyy);
                    painter.rotate(-q.angle);
                    painter.drawImage(QRect(0, h / 2, w, h), pc->image, pc->image.rect());
                    painter.rotate(+q.angle);
                    painter.translate(-xxx, -yyy);
                } else {
                    painter.drawImage(QRect(q.x - buf / 2, q.y - h + buf / 2, w, h), pc->image, pc->image.rect());
                }
            }
        } else {
            // Just draw the fonts..
            QBrush b(q.color);
            painter.setBrush(b);
            painter.setFont(*q.font);

            if (q.angle == 0) {
                // *********************************************************
                // Holy crap this is slow
                // The following line is responsible for 77% of drawing time
                // *********************************************************

                painter.drawText(q.x, q.y, q.text);
            } else {
                QBrush b(q.color);
                painter.setBrush(b);
                painter.setFont(*q.font);

                w = painter.fontMetrics().width(q.text);
                h = painter.fontMetrics().xHeight() + 2;

                painter.translate(q.x, q.y);
                painter.rotate(-q.angle);
                painter.drawText(floor(-w / 2.0), floor(-h / 2.0), q.text);
                painter.rotate(+q.angle);
                painter.translate(-q.x, -q.y);
            }
        }

        q.text.clear();
        //q.text.squeeze();
    }

    if (!usePixmapCache()) {
        painter.end();
    }

    //qDebug() << "rendered" << m_textque_items << "text items";
    m_textque_items = 0;
}

void gGraphView::AddTextQue(QString &text, short x, short y, float angle, QColor color,
                            QFont *font, bool antialias)
{
#ifdef ENABLED_THREADED_DRAWING
    text_mutex.lock();
#endif

    if (m_textque_items >= textque_max) {
        DrawTextQue();
    }

    TextQue &q = m_textque[m_textque_items++];
#ifdef ENABLED_THREADED_DRAWING
    text_mutex.unlock();
#endif
    q.text = text;
    q.x = x;
    q.y = y;
    q.angle = angle;
    q.color = color;
    q.font = font;
    q.antialias = antialias;
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

        if (!m_graphsbytitle.contains(g->title())) {
            m_graphsbytitle[g->title()] = g;
        } else {
            qDebug() << "Can't have to graphs with the same title in one GraphView!!";
        }

        // updateScrollBar();
    }
}
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

    //th-=m_offsetY;
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
void gGraphView::resizeEvent(QResizeEvent *e)
{
    QGLWidget::resizeEvent(e); // This ques a redraw event..

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

void gGraphView::selectionTime()
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
        d=PROFILE.countDays(MT_CPAP,d1,d2); */

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

    if (PROFILE.general->linkGroups()) {
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
        d=PROFILE.countDays(MT_CPAP,d1,d2); */

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
        if (PROFILE.general->linkGroups() || (m_graphs[i]->group() == group)) {
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
void gGraphView::updateScale()
{
    float th = totalHeight(); // height of all graphs
    float h = height();     // height of main widget

    if (th < h) {
        m_scaleY = h / th;  // less graphs than fits on screen, so scale to fit
    } else {
        m_scaleY = 1.0;
    }

    updateScrollBar();
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
void gGraphView::initializeGL()
{
    setAutoFillBackground(false);
    setAutoBufferSwap(false);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    if (cubeimg.size() > 0) {
        cubetex = bindTexture(*cubeimg[0]);
    }

    //    texid.resize(images.size());
    //    for(int i=0;i<images.size();i++) {
    //        texid[i]=bindTexture(*images[i]);
    //    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
}

void gGraphView::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    float dpr = devicePixelRatio();
    glOrtho(0, w / dpr, h / dpr, 0, -1, 1);
#else
    glOrtho(0, w, h, 0, -1, 1);
#endif
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void gGraphView::renderCube(float alpha)
{
    if (cubeimg.size() == 0) { return; }

    //    glPushMatrix();
    float w = width();
    float h = height();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    float dpr = devicePixelRatio();
    w *= dpr;
    h *= dpr;
#endif

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)w / (GLfloat)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClearDepth(1.0f); */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // This code has been shamelessly pinched of the interwebs..
    // When I'm feeling more energetic, I'll change it to a textured sheep or something.
    static float rotqube = 0;

    static float xpos = 0, ypos = 7;

    glLoadIdentity();

    glAlphaFunc(GL_GREATER, 0.1F);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);

    //int imgcount=cubeimg.size();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double xx = 0.0, yy = 0.0;

    // set this to 0 to make the cube stay in the center of the screen
    if (1) {
        xx = sin(M_PI / 180.0 * xpos) * 2; // ((4.0/width()) * m_mouse.rx())-2.0;
        yy = cos(M_PI / 180.0 * ypos) * 2; //2-((4.0/height()) * m_mouse.ry());
        xpos += 1;
        ypos += 1.32F;

        if (xpos > 360) { xpos -= 360.0F; }

        if (ypos > 360) { ypos -= 360.0F; }
    }


    //m_mouse.x();
    glTranslatef(xx, 0.0f, -7.0f + yy);
    glRotatef(rotqube, 0.0f, 1.0f, 0.0f);
    glRotatef(rotqube, 1.0f, 1.0f, 1.0f);


    int i = 0;
    glEnable(GL_TEXTURE_2D);
    cubetex = bindTexture(*cubeimg[0]);

    //glBindTexture(GL_TEXTURE_2D, cubetex); //texid[i % imgcount]);
    i++;
    glColor4f(1, 1, 1, alpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
    glEnd();
    // Back Face
    //bindTexture(*cubeimg[i % imgcount]);
    //glBindTexture(GL_TEXTURE_2D, texid[i % imgcount]);
    i++;
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
    glEnd();
    // Top Face
    //bindTexture(*cubeimg[i % imgcount]);
    //    glBindTexture(GL_TEXTURE_2D, texid[i % imgcount]);
    i++;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
    glEnd();
    // Bottom Face
    //bindTexture(*cubeimg[i % imgcount]);
    //glBindTexture(GL_TEXTURE_2D, texid[i % imgcount]);
    i++;
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);	// Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);	// Top Left Of The Texture and Quad
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
    glEnd();
    // Right face
    //bindTexture(*cubeimg[i % imgcount]);
    //    glBindTexture(GL_TEXTURE_2D, texid[i % imgcount]);
    i++;
    glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
    glEnd();
    // Left Face
    //GLuint tex=bindTexture(*images["mask"]);
    //glBindTexture(GL_TEXTURE_2D, tex);
    i++;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
    glEnd();

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);

    rotqube += 0.9f;

    // Restore boring 2D reality..
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, width(), height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //  glPopMatrix();
}

bool gGraphView::renderGraphs()
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

            if (m_showsplitter) {
                // draw the splitter handle
                QColor ca = QColor(128, 128, 128, 255);
                backlines->add(0, py + h, w, py + h, ca.rgba());
                ca = QColor(192, 192, 192, 255);
                backlines->add(0, py + h + 1, w, py + h + 1, ca.rgba());
                ca = QColor(90, 90, 90, 255);
                backlines->add(0, py + h + 2, w, py + h + 2, ca.rgba());
            }

        }

        py = ceil(py + h + graphSpacer);
    }

    // Physically draw the unpinned graphs
    int s = m_drawlist.size();

    for (int i = 0; i < s; i++) {
        gGraph *g = m_drawlist.at(0);
        m_drawlist.pop_front();
        g->paint(g->m_rect.x(), g->m_rect.y(), g->m_rect.width(), g->m_rect.height());
    }

    backlines->draw();

    for (int i = 0; i < m_graphs.size(); i++) {
        m_graphs[i]->drawGLBuf();
    }

    quads->draw();
    lines->draw();

    // can't draw snapshot text using this DrawTextQue function
    // TODO: Find a better solution for detecting when in snapshot mode
    if (m_graphs.size() > 1) {
        DrawTextQue();

        // Draw a gradient behind pinned graphs
        //   glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(0.85, 0.85, 1.0, 1.0); // Gradient End
        glVertex2f(0, pinned_height);
        glVertex2f(0, 0);
        glColor4f(1.0, 1.0, 1.0, 1.0); // Gradient start
        glVertex2f(width(), 0);
        glVertex2f(width(), pinned_height);
        glEnd();

        // glDisable(GL_BLEND);
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

            if (m_showsplitter) {
                // draw the splitter handle
                QColor ca = QColor(128, 128, 128, 255);
                backlines->add(0, py + h, w, py + h, ca.rgba());
                ca = QColor(192, 192, 192, 255);
                backlines->add(0, py + h + 1, w, py + h + 1, ca.rgba());
                ca = QColor(90, 90, 90, 255);
                backlines->add(0, py + h + 2, w, py + h + 2, ca.rgba());
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
            g->paint(g->m_rect.x(), g->m_rect.y(), g->m_rect.width(), g->m_rect.height());
        }

#ifdef ENABLED_THREADED_DRAWING
    }
#endif
    //int elapsed=time.elapsed();
    //QColor col=Qt::black;


    backlines->draw();

    for (int i = 0; i < m_graphs.size(); i++)
    {
        m_graphs[i]->drawGLBuf();
    }

    quads->draw();
    lines->draw();


    //    lines->setSize(linesize);

    //   DrawTextQue();
    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);

    return numgraphs > 0;
}
void gGraphView::fadeOut()
{
    if (!PROFILE.ExistsAndTrue("AnimationsAndTransitions")) { return; }

    //if (m_fadingOut) {
    //        return;
    //    }
    //if (m_inAnimation) {
    //        m_inAnimation=false;
    //  }
    //clone graphs to shapshot graphview object, render, and then fade in, before switching back to normal mode
    /*gGraphView *sg=mainwin->snapshotGraph();
    sg->trashGraphs();
    sg->setFixedSize(width(),height());
    sg->m_graphs=m_graphs;
    sg->showSplitter(); */

    //bool restart=false;
    //if (!m_inAnimation)
    //  restart=true;

    bool b = m_inAnimation;
    m_inAnimation = false;

    previous_day_snapshot = renderPixmap(width(), height(), false);
    m_inAnimation = b;
    //m_fadingOut=true;
    //m_fadingIn=false;
    //m_inAnimation=true;
    //m_limbo=false;
    //m_animationStarted.start();
    //  updateGL();
}
void gGraphView::fadeIn(bool dir)
{
    static bool firstdraw = true;
    m_tooltip->cancel();

    if (firstdraw || !PROFILE.ExistsAndTrue("AnimationsAndTransitions")) {
        updateGL();
        firstdraw = false;
        return;
    }

    if (m_fadingIn) {
        m_fadingIn = false;
        m_inAnimation = false;
        updateGL();
        return;
        // previous_day_snapshot=current_day_snapshot;
    }

    m_inAnimation = false;
    current_day_snapshot = renderPixmap(width(), height(), false);
    //    qDebug() << current_day_snapshot.depth() << "bit image depth";
    //    if (current_day_snapshot.hasAlpha()){
    //        qDebug() << "Snapshots are not storing alpha channel needed for texture blending";
    //    }
    m_inAnimation = true;

    m_animationStarted.start();
    m_fadingIn = true;
    m_limbo = false;
    m_fadedir = dir;
    updateGL();

}

void gGraphView::paintGL()
{
#ifdef DEBUG_EFFICIENCY
    QElapsedTimer time;
    time.start();
#endif

    if (redrawtimer->isActive()) {
        redrawtimer->stop();
    }

    bool something_fun = PROFILE.ExistsAndTrue("AnimationsAndTransitions");

    if (width() <= 0) { return; }

    if (height() <= 0) { return; }

    glClearColor(255, 255, 255, 255);
    //glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool numgraphs = true;
    const int animTimeout = 200;
    float phase = 0;

    int elapsed = 0;

    if (m_inAnimation || m_fadingIn) {
        elapsed = m_animationStarted.elapsed();

        if (elapsed > animTimeout) {
            if (m_fadingOut) {
                m_fadingOut = false;
                m_animationStarted.start();
                elapsed = 0;
                m_limbo = true;
            } else if (m_fadingIn) {
                m_fadingIn = false;
                m_inAnimation = false; // end animation
                m_limbo = false;
                m_fadingOut = false;
            }

            //
        } else {
            phase = float(elapsed) / float(animTimeout); //percentage of way through animation timeslot

            if (phase > 1.0) { phase = 1.0; }

            if (phase < 0) { phase = 0; }
        }

        if (m_inAnimation) {
            if (m_fadingOut) {
                //  bindTexture(previous_day_snapshot);
            } else if (m_fadingIn) {
                //int offset,offset2;
                float aphase;
                aphase = 1.0 - phase;
                /*if (m_fadedir) { // forwards
                    //offset2=-width();
                    //offset=0;
                    aphase=phase;
                    phase=1.0-phase;
                } else { // backwards
                    aphase=phase;
                    phase=phase
                    //offset=-width();
                    //offset2=0;//-width();
                }*/
                //offset=0; offset2=0;

                glEnable(GL_BLEND);

                glDisable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(aphase, aphase, aphase, aphase);

                bindTexture(previous_day_snapshot);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(0, 0);
                glTexCoord2f(1.0f, 1.0f);
                glVertex2f(width(), 0);
                glTexCoord2f(1.0f, 0.0f);
                glVertex2f(width(), height());
                glTexCoord2f(0.0f, 0.0f);
                glVertex2f(0, height());
                glEnd();

                glColor4f(phase, phase, phase, phase);
                //              glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                bindTexture(current_day_snapshot);
                glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(0, 0);
                glTexCoord2f(1.0f, 1.0f);
                glVertex2f(width(), 0);
                glTexCoord2f(1.0f, 0.0f);
                glVertex2f(width(), height());
                glTexCoord2f(0.0f, 0.0f);
                glVertex2f(0, height());
                glEnd();

                glDisable(GL_ALPHA_TEST);
                glDisable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }

    // Need a really good condition/excuse to switch this on.. :-}
    bool bereallyannoying = false;

    if (!m_inAnimation || (!m_fadingIn)) {
        // Not in animation sequence, draw graphs like normal
        if (bereallyannoying) {
            renderCube(0.7F);
        }

        numgraphs = renderGraphs();

        if (!numgraphs) { // No graphs drawn?
            int x, y;
            GetTextExtent(m_emptytext, x, y, bigfont);
            int tp;

            if (something_fun && this->isVisible()) {// Do something fun instead
                if (!bereallyannoying) {
                    renderCube();
                }

                tp = height() - (y / 2);
            } else {
                tp = height() / 2 + y / 2;
            }

            // Then display the empty text message
            QColor col = Qt::black;
            AddTextQue(m_emptytext, (width() / 2) - x / 2, tp, 0.0, col, bigfont);

        }

        DrawTextQue();
    }

    m_tooltip->paint();

#ifdef DEBUG_EFFICIENCY
    const int rs = 10;
    static double ring[rs] = {0};
    static int rp = 0;

    // Show FPS and draw time
    if (m_showsplitter && PROFILE.general->showDebug()) {
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
        ss = "Debug Mode " + QString::number(fps, 'f', 1) +
             "fps " + QString::number(lines_drawn_this_frame, 'f', 0) +
             " lines " + QString::number(quads_drawn_this_frame, 'f', 0) +
             " quads " + QString::number(pixmap_cache.count(), 'f', 0) +
             " strings " + QString::number(pixmap_cache_size / 1024.0, 'f', 1) +
             "Kb";

        int w, h;
        GetTextExtent(ss, w,
                      h); // this uses tightBoundingRect, which is different on Mac than it is on Windows & Linux.
        QColor col = Qt::white;
        quads->add(width() - m_graphs[0]->marginRight(), 0, width() - m_graphs[0]->marginRight(), w,
                   width(), w, width(), 0, col.rgba());
        quads->draw();
        //renderText(0,0,0,ss,*defaultfont);

        //     int xx=3;
#ifndef Q_OS_MAC
        //   if (usePixmapCache()) xx+=4; else xx-=3;
#endif
        AddTextQue(ss, width(), w / 2, 90, col, defaultfont);
        DrawTextQue();
    }

#endif

    swapBuffers(); // Dump to screen.

    if (this->isVisible()) {
        if (m_limbo || m_inAnimation || (something_fun && (bereallyannoying || !numgraphs))) {
            redrawtimer->setInterval(1000.0 / 50);
            redrawtimer->setSingleShot(true);
            redrawtimer->start();
        }
    }
}

void gGraphView::setCubeImage(QImage *img)
{
    cubeimg.clear();
    cubeimg.push_back(img);

    //cubetex=bindTexture(*img);
    glBindTexture(GL_TEXTURE_2D, 0);
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
                //           QPoint p(x,y);
                //           QMouseEvent e(event->type(),p,event->button(),event->buttons(),event->modifiers());
                m_graphs[i]->mouseMoveEvent(event);


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
        if (m_graphs[i]->isEmpty()
                || !m_graphs[i]->visible()
                || !m_graphs[i]->isPinned()) {
            continue;
        }

        h = m_graphs[i]->height() * m_scaleY;
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
                }

                {
                    // send event to graph..
                    m_point_clicked = QPoint(event->x(), event->y());
                    //QMouseEvent e(event->type(),m_point_clicked,event->button(),event->buttons(),event->modifiers());
                    m_button_down = true;
                    m_horiz_travel = 0;
                    m_graph_index = i;
                    m_selected_graph = m_graphs[i];
                    m_graphs[i]->mousePressEvent(event);
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

            if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || m_graphs[i]->isPinned()) { continue; }

            h = m_graphs[i]->height() * m_scaleY;

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
                        // send event to graph..
                        m_point_clicked = QPoint(event->x(), event->y());
                        //QMouseEvent e(event->type(),m_point_clicked,event->button(),event->buttons(),event->modifiers());
                        m_button_down = true;
                        m_horiz_travel = 0;
                        m_graph_index = i;
                        m_selected_graph = m_graphs[i];
                        m_graphs[i]->mousePressEvent(event);
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

    // Handle pinned graphs first
    for (int i = 0; i < m_graphs.size(); i++) {
        if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || !m_graphs[i]->isPinned()) {
            continue;
        }

        h = m_graphs[i]->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
            this->setCursor(Qt::SplitVCursor);
            done = true;
        } else if ((y >= py + 1) && (y <= py + h)) {

            //            if (!m_sizer_dragging && !m_graph_dragging) {
            //                m_graphs[i]->mouseReleaseEvent(event);
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

            if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || m_graphs[i]->isPinned()) {
                continue;
            }

            h = m_graphs[i]->height() * m_scaleY;

            if (py > height()) {
                break;    // we are done.. can't draw anymore
            }

            if ((y >= py + h - 1) && (y < (py + h + graphSpacer))) {
                this->setCursor(Qt::SplitVCursor);
            } else if ((y >= py + 1) && (y <= py + h)) {

                //            if (!m_sizer_dragging && !m_graph_dragging) {
                //                m_graphs[i]->mouseReleaseEvent(event);
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
    if (m_button_down) {
        m_button_down = false;
        m_graphs[m_graph_index]->mouseReleaseEvent(event);
    }
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
        if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || !m_graphs[i]->isPinned()) {
            continue;
        }

        h = m_graphs[i]->height() * m_scaleY;
        pinned_height += h + graphSpacer;

        if (py > height()) {
            break;    // we are done.. can't draw anymore
        }

        if ((py + h + graphSpacer) >= 0) {
            if ((y >= py) && (y <= py + h)) {
                if (x < titleWidth) {
                    // What to do when double clicked on the graph title ??

                    m_graphs[i]->mouseDoubleClickEvent(event);
                    // pin the graph??
                    m_graphs[i]->setPinned(false);
                    redraw();
                } else {
                    // send event to graph..
                    m_graphs[i]->mouseDoubleClickEvent(event);
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

            if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible() || m_graphs[i]->isPinned()) {
                continue;
            }

            h = m_graphs[i]->height() * m_scaleY;

            if (py > height()) {
                break;
            }

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py) && (y <= py + h)) {
                    if (x < titleWidth) {
                        // What to do when double clicked on the graph title ??
                        m_graphs[i]->mouseDoubleClickEvent(event);

                        m_graphs[i]->setPinned(true);
                        redraw();
                    } else {
                        // send event to graph..
                        m_graphs[i]->mouseDoubleClickEvent(event);
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

    if ((event->modifiers() & Qt::ControlModifier)) {
        int x = event->x();
        int y = event->y();

        float py = -m_offsetY;
        float h;

        for (int i = 0; i < m_graphs.size(); i++) {

            if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) { continue; }

            h = m_graphs[i]->height() * m_scaleY;

            if (py > height()) {
                break;
            }

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py) && (y <= py + h)) {
                    if (x < titleWidth) {
                        // What to do when ctrl+wheel is used on the graph title ??
                    } else {
                        // send event to graph..
                        m_graphs[i]->wheelEvent(event);
                    }
                } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                    // What to do when the wheel is used on the resize handle?
                }

            }

            py += h;
            py += graphSpacer; // do we want the extra spacer down the bottom?
        }
    } else {
        int scrollDampening = PROFILE.general->scrollDampening();

        if (event->orientation() == Qt::Vertical) { // Vertical Scrolling
            if (horizScrollTime.elapsed() < scrollDampening) {
                return;
            }

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

            SetXBounds(g->min_x, g->max_x, group);
        }
    }
}

void gGraphView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab) {
        event->ignore();
        return;
    }

    if (event->key() == Qt::Key_PageUp) {
        m_offsetY -= PROFILE.appearance->graphHeight() * 3 * m_scaleY;
        m_scrollbar->setValue(m_offsetY);
        m_offsetY = m_scrollbar->value();
        updateGL();
        return;
    } else if (event->key() == Qt::Key_PageDown) {
        m_offsetY += PROFILE.appearance->graphHeight() * 3 * m_scaleY; //PROFILE.appearance->graphHeight();

        if (m_offsetY < 0) { m_offsetY = 0; }

        m_scrollbar->setValue(m_offsetY);
        m_offsetY = m_scrollbar->value();
        updateGL();
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
        m_graphs[i]->setDay(day);
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
    if (!timer->isActive()) {
        timer->setSingleShot(true);
        timer->start(ms);
    }
}
void gGraphView::resetLayout()
{
    int default_height = PROFILE.appearance->graphHeight();

    for (int i = 0; i < m_graphs.size(); i++) {
        m_graphs[i]->setHeight(default_height);
    }

    updateScale();
    redraw();
}
void gGraphView::deselect()
{
    for (int i = 0; i < m_graphs.size(); i++) {
        m_graphs[i]->deselect();
    }
}

const quint32 gvmagic = 0x41756728;
const quint16 gvversion = 2;

void gGraphView::SaveSettings(QString title)
{
    QString filename = PROFILE.Get("{DataFolder}/") + title.toLower() + ".shg";
    QFile f(filename);
    f.open(QFile::WriteOnly);
    QDataStream out(&f);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)gvmagic;
    out << (quint16)gvversion;

    out << (qint16)size();

    for (qint16 i = 0; i < size(); i++) {
        out << m_graphs[i]->title();
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
    QString filename = PROFILE.Get("{DataFolder}/") + title.toLower() + ".shg";
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

    if (t2 != gvversion) {
        qDebug() << "gGraphView" << title << "version doesn't match";
        return false;
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

        gi = m_graphsbytitle.find(name);

        if (gi == m_graphsbytitle.end()) {
            qDebug() << "Graph" << name << "has been renamed or removed";
        } else {
            gGraph *g = gi.value();
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
    QHash<QString, gGraph *>::iterator i = m_graphsbytitle.find(name);

    if (i == m_graphsbytitle.end()) { return nullptr; }

    return i.value();
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
    if (!m_inAnimation) {
        updateGL();
    }
}
