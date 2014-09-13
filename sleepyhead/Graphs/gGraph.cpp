/* gGraph Implemntation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/gGraph.h"

#include <QLabel>
#include <QTimer>
#include <cmath>
#include <exception>

#include "mainwindow.h"
#include "Graphs/gGraphView.h"
#include "Graphs/layer.h"
#include "SleepLib/profiles.h"

extern MainWindow *mainwin;

// Graph globals.
QFont *defaultfont = nullptr;
QFont *mediumfont = nullptr;
QFont *bigfont = nullptr;
QHash<QString, QImage *> images;

static bool globalsInitialized = false;

// Graph constants.
static const double zoom_hard_limit = 500.0;

 // Must be called from a thread inside the application.
bool InitGraphGlobals()
{
    if (globalsInitialized) {
        return true;
    }

    if (!PREF.contains("Fonts_Graph_Name")) {
        PREF["Fonts_Graph_Name"] = "Sans Serif";
        PREF["Fonts_Graph_Size"] = 10;
        PREF["Fonts_Graph_Bold"] = false;
        PREF["Fonts_Graph_Italic"] = false;
    }

    if (!PREF.contains("Fonts_Title_Name")) {
        PREF["Fonts_Title_Name"] = "Sans Serif";
        PREF["Fonts_Title_Size"] = 14;
        PREF["Fonts_Title_Bold"] = true;
        PREF["Fonts_Title_Italic"] = false;
    }

    if (!PREF.contains("Fonts_Big_Name")) {
        PREF["Fonts_Big_Name"] = "Serif";
        PREF["Fonts_Big_Size"] = 35;
        PREF["Fonts_Big_Bold"] = false;
        PREF["Fonts_Big_Italic"] = false;
    }

    defaultfont = new QFont(PREF["Fonts_Graph_Name"].toString(),
                            PREF["Fonts_Graph_Size"].toInt(),
                            PREF["Fonts_Graph_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                            PREF["Fonts_Graph_Italic"].toBool()
                           );
    mediumfont = new QFont(PREF["Fonts_Title_Name"].toString(),
                           PREF["Fonts_Title_Size"].toInt(),
                           PREF["Fonts_Title_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                           PREF["Fonts_Title_Italic"].toBool()
                          );
    bigfont = new QFont(PREF["Fonts_Big_Name"].toString(),
                        PREF["Fonts_Big_Size"].toInt(),
                        PREF["Fonts_Big_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                        PREF["Fonts_Big_Italic"].toBool()
                       );

    defaultfont->setStyleHint(QFont::AnyStyle, QFont::OpenGLCompatible);
    mediumfont->setStyleHint(QFont::AnyStyle, QFont::OpenGLCompatible);
    bigfont->setStyleHint(QFont::AnyStyle, QFont::OpenGLCompatible);

    //images["mask"] = new QImage(":/icons/mask.png");
    images["oximeter"] = new QImage(":/icons/cubeoximeter.png");
    images["smiley"] = new QImage(":/icons/smileyface.png");
    //images["sad"] = new QImage(":/icons/sadface.png");

    images["sheep"] = new QImage(":/icons/sheep.png");
    images["brick"] = new QImage(":/icons/brick.png");
    images["nographs"] = new QImage(":/icons/nographs.png");
    images["nodata"] = new QImage(":/icons/nodata.png");

    globalsInitialized = true;
    return true;
}

void DestroyGraphGlobals()
{
    if (!globalsInitialized) {
        return;
    }

    delete defaultfont;
    delete bigfont;
    delete mediumfont;

    for (QHash<QString, QImage *>::iterator i = images.begin(); i != images.end(); i++) {
        delete i.value();
    }

    globalsInitialized = false;
}

gGraph::gGraph(QString name, gGraphView *graphview, QString title, QString units, int height, short group)
    : m_name(name),
      m_graphview(graphview),
      m_title(title),
      m_units(units),
      m_visible(true)
{
    if (height == 0) {
        height = p_profile->appearance->graphHeight();
    }
    if (graphview && graphview->contains(name)) {
        qDebug() << "Trying to duplicate " << name << " when a graph with the same name already exists";
        name+="-1";
    }
    m_min_height = 60;
    m_width = 0;

    m_layers.clear();

    m_snapshot = false;
    f_miny = f_maxy = 0;
    rmin_x = rmin_y = 0;
    rmax_x = rmax_y = 0;
    max_x = max_y = 0;
    min_x = min_y = 0;
    rec_miny = rec_maxy = 0;
    rphysmax_y = rphysmin_y = 0;
    m_zoomY = 0;
    m_selectedDuration = 0;

    if (graphview) {
        graphview->addGraph(this, group);
        timer = new QTimer(graphview);
        connect(timer, SIGNAL(timeout()), SLOT(Timeout()));
    } else {
        timer = new QTimer();
        connect(timer, SIGNAL(timeout()), SLOT(Timeout()));
        // know what I'm doing now.. ;)
     //   qWarning() << "gGraph created without a gGraphView container.. Naughty programmer!! Bad!!!";
    }

    m_margintop = 14;
    m_marginbottom = 5;
    m_marginleft = 0;
    m_marginright = 15;
    m_selecting_area = m_blockzoom = false;
    m_pinned = false;
    m_lastx23 = 0;

    invalidate_yAxisImage = true;
    invalidate_xAxisImage = true;

    m_block_select = false;

    m_enforceMinY = m_enforceMaxY = false;
    m_showTitle = true;
    m_printing = false;
}
gGraph::~gGraph()
{
    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->unref()) {
            delete m_layers[i];
        }
    }

    m_layers.clear();

    if (timer) {
        timer->stop();
        disconnect(timer, 0, 0, 0);
        delete timer;
    }
}
void gGraph::Trigger(int ms)
{
    if (timer->isActive()) { timer->stop(); }

    timer->setSingleShot(true);
    timer->start(ms);
}
void gGraph::Timeout()
{
    deselect();
    m_graphview->timedRedraw(0);
}

void gGraph::deselect()
{
    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        (*l)->deselect();
    }
}
bool gGraph::isSelected()
{
    bool res = false;

    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        res = (*l)->isSelected();

        if (res) { break; }
    }

    return res;
}

bool gGraph::isEmpty()
{
    bool empty = true;

    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        if (!(*l)->isEmpty()) {
            empty = false;
            break;
        }
    }


    return empty;
}


float gGraph::printScaleX() { return m_graphview->printScaleX(); }
float gGraph::printScaleY() { return m_graphview->printScaleY(); }


//void gGraph::drawGLBuf()
//{

//    float linesize = 1;

//    if (m_printing) { linesize = 4; } //ceil(m_graphview->printScaleY());

//    for (int i = 0; i < m_layers.size(); i++) {
//        m_layers[i]->drawGLBuf(linesize);
//    }

//}
void gGraph::setDay(Day *day)
{
    // Don't update for snapshots..
    if (m_snapshot) return;

    m_day = day;

    for (int i = 0; i < m_layers.size(); i++) {
        m_layers[i]->SetDay(day);
    }

    rmin_y = rmax_y = 0;
    ResetBounds();
}

void gGraph::setZoomY(short zoom)
{
    m_zoomY = zoom;
    timedRedraw(0);
}

void gGraph::renderText(QString text, int x, int y, float angle, QColor color, QFont *font, bool antialias)
{
    m_graphview->AddTextQue(text, x, y, angle, color, font, antialias);
}

void gGraph::renderText(QString text, QRectF rect, quint32 flags, float angle, QColor color, QFont *font, bool antialias)
{
    m_graphview->AddTextQue(text, rect, flags, angle, color, font, antialias);
}


void gGraph::paint(QPainter &painter, const QRegion &region)
{
    m_rect = region.boundingRect();
    int originX = m_rect.left();
    int originY = m_rect.top();
    int width = m_rect.width();
    int height = m_rect.height();

    int fw, font_height;
    GetTextExtent("Wg@", fw, font_height);

    if (m_margintop > 0) {
        m_margintop = font_height + (2*printScaleY());
    }

    //m_marginbottom=5;

    left = marginLeft()*printScaleX(), right = marginRight()*printScaleX(), top = marginTop(), bottom = marginBottom() * printScaleY();
    int x = 0, y = 0;

    if (m_showTitle) {
        int title_x, yh;

        painter.setFont(*mediumfont);
        QFontMetrics fm(*mediumfont);

        yh = fm.height();
        //GetTextExtent("Wy@",x,yh,mediumfont); // This gets a better consistent height. should be cached.
        y = yh;
        x = fm.width(title());
        //GetTextExtent(title(),x,y,mediumfont);
        title_x = float(yh) ;

        QString & txt = title();
        graphView()->AddTextQue(txt, marginLeft() + title_x + 8*printScaleX(), originY + height / 2 - y / 2, 90, Qt::black, mediumfont);

        left += graphView()->titleWidth*printScaleX();
    } else { left = 0; }


    if (m_snapshot) {
        QLinearGradient linearGrad(QPointF(100, 100), QPointF(width / 2, 100));
        linearGrad.setColorAt(0, QColor(255, 150, 150,40));
        linearGrad.setColorAt(1, QColor(255,255,255,20));

        painter.fillRect(m_rect, QBrush(linearGrad));
        painter.setFont(*defaultfont);
        painter.setPen(QColor(0,0,0,255));

        QString t = name().section(";", -1);

        QString txt = QObject::tr("Snapshot %1").arg(t);
        QRectF rec = QRect(m_rect.left(),m_rect.top()+6*printScaleY(), m_rect.width(), 0);
        rec = painter.boundingRect(rec, Qt::AlignCenter, txt);

        painter.drawText(rec, Qt::AlignCenter, txt);
        m_margintop += rec.height();
        top = m_margintop;
    }


#ifdef DEBUG_LAYOUT
    QColor col = Qt::red;
    painter.setPen(col);
    painter.drawLine(0, originY, 0, originY + height);
    painter.drawLine(left, originY, left, originY + height);
#endif
    int tmp;

   // left = 0;

    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }

        tmp = ll->minimumHeight();// * m_graphview->printScaleY();

        if (ll->position() == LayerTop) { top += tmp; }
        if (ll->position() == LayerBottom) { bottom += tmp * printScaleY(); }
    }


    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }

        tmp = ll->minimumWidth();
        tmp *= m_graphview->printScaleX();
        tmp *= m_graphview->devicePixelRatio();

        if (ll->position() == LayerLeft) {
            QRect rect(originX + left, originY + top, tmp, height - top - bottom);
            ll->m_rect = rect;
          //  ll->paint(painter, *this, QRegion(rect));
            left += tmp;
#ifdef DEBUG_LAYOUT
            QColor col = Qt::red;
            painter.setPen(col);
            painter.drawLine(originX + left - 1, originY, originX + left - 1, originY + height);
#endif
        }

        if (ll->position() == LayerRight) {
            right += tmp;
            QRect rect(originX + width - right, originY + top, tmp, height - top - bottom);
            ll->m_rect = rect;
            //ll->paint(painter, *this, QRegion(rect));
#ifdef DEBUG_LAYOUT
            QColor col = Qt::red;
            painter.setPen(col);
            painter.drawLine(originX + width - right, originY, originX + width - right, originY + height);
#endif
        }
    }

    bottom = marginBottom() * printScaleY();
    top = marginTop();

    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }

        tmp = ll->minimumHeight();

        if (ll->position() == LayerTop) {
            QRect rect(originX + left, originY + top, width - left - right, tmp);
            ll->m_rect = rect;
            ll->paint(painter, *this, QRegion(rect));
            top += tmp;
        }

        if (ll->position() == LayerBottom) {
            bottom += tmp * printScaleY();
            QRect rect(originX + left, originY + height - bottom, width - left - right, tmp);
            ll->m_rect = rect;
            ll->paint(painter, *this, QRegion(rect));
        }
    }

    if (isPinned()) {
        // Fill the background on pinned graphs
        painter.fillRect(originX + left, originY + top, width - right, height - bottom - top, QBrush(QColor(Qt::white)));
    }

    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }

        if (ll->position() == LayerCenter) {
            QRect rect(originX + left, originY + top, width - left - right, height - top - bottom);
            ll->m_rect = rect;
            ll->paint(painter, *this, QRegion(rect));
        }
    }

    // Draw anything like the YAxis labels afterwards, in case the graph scale was updated during draw
    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }
        if ((ll->position() == LayerLeft) || (ll->position() == LayerRight)) {
            ll->paint(painter, *this, QRegion(ll->m_rect));
        }
    }

    if (m_selection.width() > 0 && m_selecting_area) {
        QColor col(128, 128, 255, 128);
        painter.fillRect(originX + m_selection.x(), originY + top, m_selection.width(), height - bottom - top,QBrush(col));
//        quads()->add(originX + m_selection.x(), originY + top,
//                     originX + m_selection.x() + m_selection.width(), originY + top, col.rgba());
//        quads()->add(originX + m_selection.x() + m_selection.width(), originY + height - bottom,
//                     originX + m_selection.x(), originY + height - bottom, col.rgba());
    }

    if (isPinned() && !printing()) {
        painter.drawPixmap(-5, originY-10, m_graphview->pin_icon);
    }

}

QPixmap gGraph::renderPixmap(int w, int h, bool printing)
{

    QFont *_defaultfont = defaultfont;
    QFont *_mediumfont = mediumfont;
    QFont *_bigfont = bigfont;

    QFont fa = *defaultfont;
    QFont fb = *mediumfont;
    QFont fc = *bigfont;


    m_printing = printing;

    if (printing) {
        fa.setPixelSize(28);
        fb.setPixelSize(32);
        fc.setPixelSize(70);
        graphView()->setPrintScaleX(2.5);
        graphView()->setPrintScaleY(2.2);
    } else {
        graphView()->setPrintScaleX(1);
        graphView()->setPrintScaleY(1);
    }

    defaultfont = &fa;
    mediumfont = &fb;
    bigfont = &fc;

    QPixmap pm(w,h);


    bool pixcaching = p_profile->appearance->usePixmapCaching();
    graphView()->setUsePixmapCache(false);
    p_profile->appearance->setUsePixmapCaching(false);
    QPainter painter(&pm);
    painter.fillRect(0,0,w,h,QBrush(QColor(Qt::white)));
    QRegion region(0,0,w,h);
    paint(painter, region);
    DrawTextQue(painter);
    painter.end();

    graphView()->setUsePixmapCache(pixcaching);
    p_profile->appearance->setUsePixmapCaching(pixcaching);
    graphView()->setPrintScaleX(1);
    graphView()->setPrintScaleY(1);


    defaultfont = _defaultfont;
    mediumfont = _mediumfont;
    bigfont = _bigfont;
    m_printing = false;

    return pm;
}

// Sets a new Min & Max X clipping, refreshing the graph and all it's layers.
void gGraph::SetXBounds(qint64 minx, qint64 maxx)
{
    invalidate_xAxisImage = true;
    min_x = minx;
    max_x = maxx;

    //repaint();
    //m_graphview->redraw();
}

int gGraph::flipY(int y)
{
    return m_graphview->height() - y;
}

void gGraph::ResetBounds()
{
    if (m_snapshot) return;
    invalidate_xAxisImage = true;
    min_x = MinX();
    max_x = MaxX();
    min_y = MinY();
    max_y = MaxY();
}

void gGraph::ToolTip(QString text, int x, int y, ToolTipAlignment align, int timeout)
{
    if (timeout <= 0) {
        timeout = p_profile->general->tooltipTimeout();
    }

    m_graphview->m_tooltip->display(text, x, y, align, timeout);
}

// YAxis Autoscaling code
void gGraph::roundY(EventDataType &miny, EventDataType &maxy)
{

    if (zoomY() == 2) {
        miny = rec_miny;
        maxy = rec_maxy;
        if (maxy > miny) return;
    } else if (zoomY() ==1) {
        miny = physMinY();
        maxy = physMaxY();
        if (maxy > miny) return;
    }
    miny = MinY();
    maxy = MaxY();

    int m, t;
    bool ymin_good = false, ymax_good = false;

    // Have no minx/miny reference, have to create one
    if (maxy == miny) {
        m = ceil(maxy / 2.0);
        t = m * 2;

        if (maxy == t) {
            t += 2;
        }

        if (!ymax_good) {
            maxy = t;
        }

        m = floor(miny / 2.0);
        t = m * 2;

        if (miny == t) {
            t -= 2;
        }

        if (miny >= 0 && t < 0) {
            t = 0;
        }

        if (!ymin_good) {
            miny = t;
        }


        if (miny < 0) {
            EventDataType tmp = qMax(qAbs(miny), qAbs(maxy));
            maxy = tmp;
            miny = -tmp;
        }

        return;
    }

    if (maxy >= 400) {
        m = ceil(maxy / 50.0);
        t = m * 50;

        if (!ymax_good) {
            maxy = t;
        }

        m = floor(miny / 50.0);

        if (!ymin_good) {
            miny = m * 50;
        }
    } else if (maxy >= 30) {
        m = ceil(maxy / 5.0);
        t = m * 5;

        if (!ymax_good) {
            maxy = t;
        }

        m = floor(miny / 5.0);

        if (!ymin_good) {
            miny = m * 5;
        }
    } else {
        if (maxy == miny && maxy == 0) {
            maxy = 0.5;
        } else {
            //maxy*=4.0;
            //miny*=4.0;
            if (!ymax_good) {
                maxy = ceil(maxy);
            }

            if (!ymin_good) {
                miny = floor(miny);
            }

            //maxy/=4.0;
            //miny/=4.0;
        }
    }

    if (miny < 0) {
        EventDataType tmp = qMax(qAbs(miny), qAbs(maxy));
        maxy = tmp;
        miny = -tmp;
    }


    //if (m_enforceMinY) { miny=f_miny; }
    //if (m_enforceMaxY) { maxy=f_maxy; }
}

void gGraph::AddLayer(Layer *l, LayerPosition position, short width, short height, short order, bool movable, short x, short y)
{
    l->setLayout(position, width, height, order);
    l->setMovable(movable);
    l->setPos(x, y);
    l->addref();
    m_layers.push_back(l);
}
void gGraph::redraw()
{
    m_graphview->redraw();
}
void gGraph::timedRedraw(int ms)
{
    m_graphview->timedRedraw(ms);
}

double gGraph::currentTime() const
{
    return m_graphview->currentTime();
}
double gGraph::screenToTime(int xpos)
{
    double w = m_rect.width() - left - right;
    double xx = m_blockzoom ? rmax_x - rmin_x : max_x - min_x;
    double xmult = xx / w;
    double x = xpos - m_rect.left() - left;
    double res = xmult * x;
    res += m_blockzoom ? rmin_x : min_x;
    return res;
}

void gGraph::mouseMoveEvent(QMouseEvent *event)
{
    // qDebug() << m_title << "Move" << event->pos() << m_graphview->pointClicked();
    if (m_rect.width() == 0) return;
    int y = event->y();
    int x = event->x();

    bool doredraw = false;

    timedRedraw(0);


    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->m_rect.contains(x, y))
            if (m_layers[i]->mouseMoveEvent(event, this)) {
                return;
//                doredraw = true;
            }
    }

    y -= m_rect.top();
    x -= m_rect.left();

    int x2 = m_graphview->pointClicked().x() - m_rect.left();

    int w = m_rect.width() - left - right;


    double xx = max_x - min_x;
    double xmult = xx / double(w);

    {
        xmult = (m_blockzoom ? double(rmax_x - rmin_x) : double(max_x - min_x)) / double(w);

        double  a = x;

        if (a < left) a = left;
        if (a > left+w) a = left+w;

        a -= left;
        a *= xmult;
        a += m_blockzoom ? rmin_x : min_x;

        m_currentTime = a;
        m_graphview->setCurrentTime(a);
    }

    if (m_graphview->m_selected_graph == this) {  // Left Mouse button dragging
        if (event->buttons() & Qt::LeftButton) {

            //qDebug() << m_title << "Moved" << x << y << left << right << top << bottom << m_width << h;
            int a1 = MIN(x, x2);
            int a2 = MAX(x, x2);

            if (a1 < left) { a1 = left; }

            if (a2 > left + w) { a2 = left + w; }

            m_selecting_area = true;
            m_selection = QRect(a1 - 1, 0, a2 - a1, m_rect.height());
            double w2 = m_rect.width() - right - left;

            if (m_blockzoom) {
                xmult = (rmax_x - rmin_x) / w2;
            } else {
                xmult = (max_x - min_x) / w2;
            }

            qint64 a = double(a2 - a1) * xmult;
            m_selectedDuration = a;
            float d = double(a) / 86400000.0;
            int h = a / 3600000;
            int m = (a / 60000) % 60;
            int s = (a / 1000) % 60;
            int ms(a % 1000);

            if (d > 1) {
                m_selDurString.sprintf("%1.0f days", d);
            } else {

                m_selDurString.sprintf("%02i:%02i:%02i:%03i", h, m, s, ms);
            }

            ToolTipAlignment align = x >= x2 ? TT_AlignLeft : TT_AlignRight;
            int offset = (x >= x2) ? 20 : - 20;
            ToolTip(m_selDurString, m_rect.left() + x + offset, m_rect.top() + y + 20, align);

            doredraw = true;
        } else if (event->buttons() & Qt::RightButton) {    // Right Mouse button dragging
            m_graphview->setPointClicked(event->pos());
            x -= left;
            x2 -= left;

            if (!m_blockzoom) {
                xx = max_x - min_x;
                xmult = xx / double(w);
                qint64 j1 = xmult * x;
                qint64 j2 = xmult * x2;
                qint64 jj = j2 - j1;
                min_x += jj;
                max_x += jj;

                if (min_x < rmin_x) {
                    min_x = rmin_x;
                    max_x = rmin_x + xx;
                }

                if (max_x > rmax_x) {
                    max_x = rmax_x;
                    min_x = rmax_x - xx;
                }

                m_graphview->SetXBounds(min_x, max_x, m_group, false);
                doredraw = true;
            } else {
                qint64 qq = rmax_x - rmin_x;
                xx = max_x - min_x;

                if (xx == qq) { xx = 1800000; }

                xmult = qq / double(w);
                qint64 j1 = (xmult * x);
                min_x = rmin_x + j1 - (xx / 2);
                max_x = min_x + xx;

                if (min_x < rmin_x) {
                    min_x = rmin_x;
                    max_x = rmin_x + xx;
                }

                if (max_x > rmax_x) {
                    max_x = rmax_x;
                    min_x = rmax_x - xx;
                }

                m_graphview->SetXBounds(min_x, max_x, m_group, false);
                doredraw = true;
            }
        }
    }

    //if (!nolayer) { // no mouse button
//    if (doredraw) {
//        m_graphview->timedRedraw(0);
//    }

    //}
    //if (x>left+m_marginleft && x<m_lastbounds.width()-(right+m_marginright) && y>top+m_margintop && y<m_lastbounds.height()-(bottom+m_marginbottom)) { // main area
    //        x-=left+m_marginleft;
    //        y-=top+m_margintop;
    //        //qDebug() << m_title << "Moved" << x << y << left << right << top << bottom << m_width << m_height;
    //    }
}

bool gGraph::selectingArea() { return m_selecting_area || m_graphview->metaSelect(); }

void gGraph::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();
    int x = event->pos().x();

    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->m_rect.contains(x, y))
            if (m_layers[i]->mousePressEvent(event, this)) {
                return;
            }
    }

    /*
    int w=m_lastbounds.width()-(right+m_marginright);
    //int h=m_lastbounds.height()-(bottom+m_marginbottom);
    //int x2,y2;
    double xx=max_x-min_x;
    //double xmult=xx/w;
    if (x>left+m_marginleft && x<m_lastbounds.width()-(right+m_marginright) && y>top+m_margintop && y<m_lastbounds.height()-(bottom+m_marginbottom)) { // main area
        x-=left+m_marginleft;
        y-=top+m_margintop;
    }*/
    //qDebug() << m_title << "Clicked" << x << y << left << right << top << bottom << m_width << m_height;
}

void gGraph::mouseReleaseEvent(QMouseEvent *event)
{

    int y = event->pos().y();
    int x = event->pos().x();

    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->m_rect.contains(x, y))
            if (m_layers[i]->mouseReleaseEvent(event, this)) {
                return;
            }
    }

    x -= m_rect.left();
    y -= m_rect.top();


    int w = m_rect.width() - left - right; //(m_marginleft+left+right+m_marginright);
    int h = m_rect.height() - bottom; //+m_marginbottom);

    int x2 = m_graphview->pointClicked().x() - m_rect.left();
    int y2 = m_graphview->pointClicked().y() - m_rect.top();


    m_selDurString = QString();

    //qDebug() << m_title << "Released" << min_x << max_x << x << y << x2 << y2 << left << right << top << bottom << m_width << m_height;
    if (m_selecting_area) {
        m_selecting_area = false;
        m_selection.setWidth(0);

        if (m_graphview->horizTravel() > mouse_movement_threshold) {
            x -= left; //+m_marginleft;
            y -= top; //+m_margintop;
            x2 -= left; //+m_marginleft;
            y2 -= top; //+m_margintop;

            if (x < 0) { x = 0; }

            if (x2 < 0) { x2 = 0; }

            if (x > w) { x = w; }

            if (x2 > w) { x2 = w; }

            double xx;
            double xmult;

            if (!m_blockzoom) {
                xx = max_x - min_x;
                xmult = xx / double(w);
                qint64 j1 = min_x + xmult * x;
                qint64 j2 = min_x + xmult * x2;
                qint64 a1 = MIN(j1, j2)
                            qint64 a2 = MAX(j1, j2)

                                        //if (a1<rmin_x) a1=rmin_x;
                if (a2 > rmax_x) { a2 = rmax_x; }

                if (a1 <= rmin_x && a2 <= rmin_x) {
                    //qDebug() << "Foo??";
                } else {
                    if (a2 - a1 < zoom_hard_limit) { a2 = a1 + zoom_hard_limit; }

                    m_graphview->SetXBounds(a1, a2, m_group);
                }
            } else {
                xx = rmax_x - rmin_x;
                xmult = xx / double(w);
                qint64 j1 = rmin_x + xmult * x;
                qint64 j2 = rmin_x + xmult * x2;
                qint64 a1 = MIN(j1, j2)
                qint64 a2 = MAX(j1, j2)

                //if (a1<rmin_x) a1=rmin_x;
                if (a2 > rmax_x) { a2 = rmax_x; }

                if (a1 <= rmin_x && a2 <= rmin_x) {
                    qDebug() << "Foo2??";
                } else  {
                    if (a2 - a1 < zoom_hard_limit) { a2 = a1 + zoom_hard_limit; }
                    m_graphview->SetXBounds(a1, a2, m_group);
                }
            }

            return;
        } else { m_graphview->redraw(); }
    }

    if ((m_graphview->horizTravel() < mouse_movement_threshold) && (x > left && x < w + left
            && y > top && y < h)) {
        // normal click in main area
        if (!m_blockzoom) {
            double zoom;

            if (event->button() & Qt::RightButton) {
                zoom = 1.33;

                if (event->modifiers() & Qt::ControlModifier) { zoom *= 1.5; }

                ZoomX(zoom, x); // Zoom out
                return;
            } else if (event->button() & Qt::LeftButton) {
                zoom = 0.75;

                if (event->modifiers() & Qt::ControlModifier) { zoom /= 1.5; }

                ZoomX(zoom, x); // zoom in.
                return;
            }
        } else {
            x -= left;
            y -= top;
            //w-=m_marginleft+left;
            double qq = rmax_x - rmin_x;
            double xmult;

            double xx = max_x - min_x;
            //if (xx==qq) xx=1800000;

            xmult = qq / double(w);

            if ((xx == qq) || (x == m_lastx23)) {
                double zoom = 1;

                if (event->button() & Qt::RightButton) {
                    zoom = 1.33;

                    if (event->modifiers() & Qt::ControlModifier) { zoom *= 1.5; }
                } else if (event->button() & Qt::LeftButton) {
                    zoom = 0.75;

                    if (event->modifiers() & Qt::ControlModifier) { zoom /= 1.5; }
                }

                xx *= zoom;

                if (xx < qq / zoom_hard_limit) { xx = qq / zoom_hard_limit; }

                if (xx > qq) { xx = qq; }
            }

            double j1 = xmult * x;
            min_x = rmin_x + j1 - (xx / 2.0);
            max_x = min_x + xx;

            if (min_x < rmin_x) {
                min_x = rmin_x;
                max_x = rmin_x + xx;
            } else if (max_x > rmax_x) {
                max_x = rmax_x;
                min_x = rmax_x - xx;
            }

            m_graphview->SetXBounds(min_x, max_x, m_group);
            m_lastx23 = x;
        }
    }

    //m_graphview->redraw();
}


void gGraph::wheelEvent(QWheelEvent *event)
{
    //qDebug() << m_title << "Wheel" << event->x() << event->y() << event->delta();
    //int y=event->pos().y();
    if (event->orientation() == Qt::Horizontal) {

        return;
    }

    int x = event->pos().x() - m_graphview->titleWidth; //(left+m_marginleft);

    if (event->delta() > 0) {
        ZoomX(0.75, x);
    } else {
        ZoomX(1.5, x);
    }

    int y = event->pos().y();
    x = event->pos().x();

    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->m_rect.contains(x, y)) {
            m_layers[i]->wheelEvent(event, this);
        }
    }

}
void gGraph::mouseDoubleClickEvent(QMouseEvent *event)
{
    //mousePressEvent(event);
    //mouseReleaseEvent(event);
    int y = event->pos().y();
    int x = event->pos().x();

    for (int i = 0; i < m_layers.size(); i++) {
        if (m_layers[i]->m_rect.contains(x, y)) {
            m_layers[i]->mouseDoubleClickEvent(event, this);
        }
    }

    //int w=m_lastbounds.width()-(m_marginleft+left+right+m_marginright);
    //int h=m_lastbounds.height()-(bottom+m_marginbottom);
    //int x2=m_graphview->pointClicked().x(),y2=m_graphview->pointClicked().y();
    //    if ((m_graphview->horizTravel()<mouse_movement_threshold) && (x>left+m_marginleft && x<w+m_marginleft+left && y>top+m_margintop && y<h)) { // normal click in main area
    //        if (event->button() & Qt::RightButton) {
    //            ZoomX(1.66,x);  // Zoon out
    //            return;
    //        } else if (event->button() & Qt::LeftButton) {
    //            ZoomX(0.75/2.0,x); // zoom in.
    //            return;
    //        }
    //    } else {
    // Propagate the events to graph Layers
    //    }
    //mousePressEvent(event);
    //mouseReleaseEvent(event);
    //qDebug() << m_title << "Double Clicked" << event->x() << event->y();
}
void gGraph::keyPressEvent(QKeyEvent *event)
{
    for (QVector<Layer *>::iterator i = m_layers.begin(); i != m_layers.end(); i++) {
        (*i)->keyPressEvent(event, this);
    }

    //qDebug() << m_title << "Key Pressed.. implement me" << event->key();
}

void gGraph::keyReleaseEvent(QKeyEvent *event)
{
    if (!m_graphview) return;

    if (m_graphview->selectionInProgress() && m_graphview->metaSelect()) {
        if (!(event->modifiers() & Qt::AltModifier)) {

        }
    }
}


void gGraph::ZoomX(double mult, int origin_px)
{

    int width = m_rect.width() - left - right; //(m_marginleft+left+right+m_marginright);

    if (origin_px == 0) { origin_px = (width / 2); }
    else { origin_px -= left; }

    if (origin_px < 0) { origin_px = 0; }

    if (origin_px > width) { origin_px = width; }


    // Okay, I want it to zoom in centered on the mouse click area..
    // Find X graph position of mouse click
    // find current zoom width
    // apply zoom
    // center on point found in step 1.

    qint64 min = min_x;
    qint64 max = max_x;

    double hardspan = rmax_x - rmin_x;
    double span = max - min;
    double ww = double(origin_px) / double(width);
    double origin = ww * span;
    //double center=0.5*span;
    //double dist=(origin-center);

    double q = span * mult;

    if (q > hardspan) { q = hardspan; }

    if (q < hardspan / zoom_hard_limit) { q = hardspan / zoom_hard_limit; }

    min = min + origin - (q * ww);
    max = min + q;

    if (min < rmin_x) {
        min = rmin_x;
        max = min + q;
    }

    if (max > rmax_x) {
        max = rmax_x;
        min = max - q;
    }

    extern const int max_history;

    m_graphview->SetXBounds(min, max, m_group);
    //updateSelectionTime(max-min);
}

void gGraph::DrawTextQue(QPainter &painter)
{
    m_graphview->DrawTextQue(painter);
}

// margin recalcs..
void gGraph::resize(int width, int height)
{
    invalidate_xAxisImage = true;
    invalidate_yAxisImage = true;

    Q_UNUSED(width);
    Q_UNUSED(height);
    //m_height=height;
    //m_width=width;
}

qint64 gGraph::MinX()
{
    qint64 val = 0, tmp;

    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        if ((*l)->isEmpty()) {
            continue;
        }

        tmp = (*l)->Minx();

        if (!tmp) {
            continue;
        }

        if (!val || tmp < val) {
            val = tmp;
        }
    }

    if (val) { rmin_x = val; }

    return val;
}
qint64 gGraph::MaxX()
{
    //bool first=true;
    qint64 val = 0, tmp;

    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        if ((*l)->isEmpty()) {
            continue;
        }

        tmp = (*l)->Maxx();

        //if (!tmp) continue;
        if (!val || tmp > val) {
            val = tmp;
        }
    }

    if (val) { rmax_x = val; }

    return val;
}

EventDataType gGraph::MinY()
{
    bool first = true;
    EventDataType val = 0, tmp;

    if (m_enforceMinY) {
        return rmin_y = f_miny;
    }

    for (QVector<Layer *>::iterator l = m_layers.begin(); l != m_layers.end(); l++) {
        if ((*l)->isEmpty() || ((*l)->layerType() == LT_Other)) {
            continue;
        }

        tmp = (*l)->Miny();

//        if (tmp == 0 && tmp == (*l)->Maxy()) {
//            continue;
//        }

        if (first) {
            val = tmp;
            first = false;
        } else {
            if (tmp < val) {
                val = tmp;
            }
        }
    }

    return rmin_y = val;
}
EventDataType gGraph::MaxY()
{
    bool first = true;
    EventDataType val = 0, tmp;

    if (m_enforceMaxY) {
        return rmax_y = f_maxy;
    }

    QVector<Layer *>::const_iterator iterEnd = m_layers.constEnd();
    for (QVector<Layer *>::const_iterator iter = m_layers.constBegin(); iter != iterEnd; ++iter) {
        Layer *layer = *iter;
        if (layer->isEmpty() || (layer->layerType() == LT_Other)) {
            continue;
        }

        tmp = layer->Maxy();
//        if (tmp == 0 && layer->Miny() == 0) {
//            continue;
//        }

        if (first) {
            val = tmp;
            first = false;
        } else {
            if (tmp > val) {
                val = tmp;
            }
        }
    }

    return rmax_y = val;
}

EventDataType gGraph::physMinY()
{
    bool first = true;
    EventDataType val = 0, tmp;

    //if (m_enforceMinY) return rmin_y=f_miny;

    QVector<Layer *>::const_iterator iterEnd = m_layers.constEnd();
    for (QVector<Layer *>::const_iterator iter = m_layers.constBegin(); iter != iterEnd; ++iter) {
        Layer *layer = *iter;
        if (layer->isEmpty()) {
            continue;
        }

        tmp = layer->physMiny();
        if (tmp == 0 && layer->physMaxy() == 0) {
            continue;
        }

        if (first) {
            val = tmp;
            first = false;
        } else {
            if (tmp < val) {
                val = tmp;
            }
        }
    }

    return rphysmin_y = val;
}
EventDataType gGraph::physMaxY()
{
    bool first = true;
    EventDataType val = 0, tmp;

    // if (m_enforceMaxY) return rmax_y=f_maxy;

    QVector<Layer *>::const_iterator iterEnd = m_layers.constEnd();
    for (QVector<Layer *>::const_iterator iter = m_layers.constBegin(); iter != iterEnd; ++iter) {
        Layer *layer = *iter;
        if (layer->isEmpty()) {
            continue;
        }

        tmp = layer->physMaxy();
        if (tmp == 0 && layer->physMiny() == 0) {
            continue;
        }

        if (first) {
            val = tmp;
            first = false;
        } else {
            if (tmp > val) {
                val = tmp;
            }
        }
    }

    return rphysmax_y = val;
}

void gGraph::SetMinX(qint64 v)
{
    rmin_x = min_x = v;
}

void gGraph::SetMaxX(qint64 v)
{
    rmax_x = max_x = v;
}

void gGraph::SetMinY(EventDataType v)
{
    rmin_y = min_y = v;
}

void gGraph::SetMaxY(EventDataType v)
{
    rmax_y = max_y = v;
}

Layer *gGraph::getLineChart()
{
    gLineChart *lc;

    for (int i = 0; i < m_layers.size(); i++) {
        lc = dynamic_cast<gLineChart *>(m_layers[i]);

        if (lc) { return lc; }
    }

    return nullptr;
}
int gGraph::minHeight()
{
    int minheight = m_min_height;

    int top = 0;
    int center = 0;
    int bottom = 0;
    for (int i=0; i<m_layers.size(); ++i) {
        int mh = m_layers[i]->minimumHeight();
        mh += m_margintop + m_marginbottom;
        if (mh > minheight) minheight = mh;
    }
    // layers need to set their own too..
    return minheight;
}

void GetTextExtent(QString text, int &width, int &height, QFont *font)
{
#ifdef ENABLE_THREADED_DRAWING
    static QMutex mut;
    mut.lock();
#endif
    QFontMetrics fm(*font);
    //#ifdef Q_OS_WIN32
    QRect r = fm.boundingRect(text);
    width = r.width();
    height = r.height();
    //#else
    //    width=fm.width(text);
    //    height=fm.xHeight()+2; // doesn't work properly on windows..
    //#endif
#ifdef ENABLE_THREADED_DRAWING
    mut.unlock();
#endif
}

int GetXHeight(QFont *font)
{
    QFontMetrics fm(*font);
    return fm.xHeight();
}

void gGraph::dumpInfo() {
    for (int i = 0; i < m_layers.size(); i++) {
        Layer *ll = m_layers[i];

        if (!ll->visible()) { continue; }

        if (ll->position() == LayerCenter) {
            gLineChart *lc = dynamic_cast<gLineChart *>(ll);
            if (lc != nullptr) {
                QString text = lc->getMetaString(currentTime());
                if (!text.isEmpty()) {
                    mainwin->log(text);
                }
            }
        }
    }
}


