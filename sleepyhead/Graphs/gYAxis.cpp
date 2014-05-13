/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gYAxis Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/gYAxis.h"

#include <QDebug>

#include <math.h>

#include "Graphs/glcommon.h"
#include "Graphs/gGraph.h"
#include "Graphs/gGraphView.h"
#include "SleepLib/profiles.h"

gXGrid::gXGrid(QColor col)
    : Layer(NoChannel)
{
    Q_UNUSED(col)

    m_major_color = QColor(180, 180, 180, 64);
    //m_major_color=QColor(180,180,180,92);
    m_minor_color = QColor(230, 230, 230, 64);
    m_show_major_lines = true;
    m_show_minor_lines = true;
}
gXGrid::~gXGrid()
{
}
void gXGrid::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    int x, y;

    EventDataType miny, maxy;

    if (w.zoomY() == 0 && PROFILE.appearance->allowYAxisScaling()) {
        miny = w.physMinY();
        maxy = w.physMaxY();
    } else {
        miny = w.min_y;
        maxy = w.max_y;

        if (miny < 0) { // even it up if it's starts negative
            miny = -MAX(fabs(miny), fabs(maxy));
        }
    }

    w.roundY(miny, maxy);

    //EventDataType dy=maxy-miny;

    if (height < 0) { return; }

    static QString fd = "0";
    GetTextExtent(fd, x, y);

    double max_yticks = round(height / (y + 14.0)); // plus spacing between lines
    //double yt=1/max_yticks;

    double mxy = MAX(fabs(maxy), fabs(miny));
    double mny = miny;

    if (miny < 0) {
        mny = -mxy;
    }

    double rxy = mxy - mny;

    int myt;
    bool fnd = false;

    for (myt = max_yticks; myt >= 1; myt--) {
        float v = rxy / float(myt);

        if (float(v) == int(v)) {
            fnd = true;
            break;
        }
    }

    if (fnd) { max_yticks = myt; }
    else {
        max_yticks = 2;
    }

    double yt = 1 / max_yticks;

    double ymult = height / rxy;

    double min_ytick = rxy * yt;

    float ty, h;

    if (min_ytick <= 0) {
        qDebug() << "min_ytick error in gXGrid::paint() in" << w.title();
        return;
    }

    if (min_ytick >= 1000000) {
        min_ytick = 100;
    }
    QVector<QLine> majorlines;
    QVector<QLine> minorlines;

    for (double i = miny; i <= maxy + min_ytick - 0.00001; i += min_ytick) {
        ty = (i - miny) * ymult;
        h = top + height - ty;

        if (m_show_major_lines && (i > miny)) {
            majorlines.append(QLine(left, h, left + width, h));
        }

        double z = (min_ytick / 4) * ymult;
        double g = h;

        for (int i = 0; i < 3; i++) {
            g += z;

            if (g > top + height) { break; }

            //if (vertcnt>=maxverts) {
            //  qWarning() << "vertarray bounds exceeded in gYAxis for " << w.title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            //                break;
            //          }
            if (m_show_minor_lines) {// && (i > miny)) {
                minorlines.append(QLine(left, g, left + width, g));
            }
        }
    }
    painter.setPen(QPen(m_major_color,1));
    painter.drawLines(majorlines);
    painter.setPen(QPen(m_minor_color,1));
    painter.drawLines(minorlines);
    w.graphView()->lines_drawn_this_frame += majorlines.size() + minorlines.size();
}



gYAxis::gYAxis(QColor col)
    : Layer(NoChannel)
{
    m_line_color = col;
    m_text_color = col;
    m_yaxis_scale = 1;
}
gYAxis::~gYAxis()
{
}
void gYAxis::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    int x, y; //,yh=0;

    //Todo: clean this up as there is a lot of duplicate code between the sections

    if (0) {//w.graphView()->usePixmapCache()) {
        /*        if (w.invalidate_yAxisImage) {

                    if (!m_image.isNull()) {
                        w.graphView()->deleteTexture(m_textureID);
                        m_image=QImage();
                    }


                    if (height<0) return;
                    if (height>2000) return;

                    int labelW=0;

                    EventDataType miny=w.min_y;
                    EventDataType maxy=w.max_y;

                    if (miny<0) { // even it up if it's starts negative
                        miny=-MAX(fabs(miny),fabs(maxy));
                    }

                    w.roundY(miny,maxy);

                    EventDataType dy=maxy-miny;
                    static QString fd="0";
                    GetTextExtent(fd,x,y);
                    yh=y;

                    m_image=QImage(width,height+y+4,QImage::Format_ARGB32_Premultiplied);

                    m_image.fill(Qt::transparent);
                    QPainter paint(&m_image);


                    double max_yticks=round(height / (y+14.0)); // plus spacing between lines

                    double mxy=MAX(fabs(maxy),fabs(miny));
                    double mny=miny;
                    if (miny<0) {
                        mny=-mxy;
                    }

                    double rxy=mxy-mny;

                    int myt;
                    bool fnd=false;
                    for (myt=max_yticks;myt>2;myt--) {
                        float v=rxy/float(myt);
                        if (v==int(v)) {
                            fnd=true;
                            break;
                        }
                    }
                    if (fnd) max_yticks=myt;
                    double yt=1/max_yticks;

                    double ymult=height/rxy;

                    double min_ytick=rxy*yt;

                    float ty,h;

                    if (min_ytick<=0) {
                        qDebug() << "min_ytick error in gYAxis::paint() in" << w.title();
                        return;
                    }
                    if (min_ytick>=1000000) {
                        min_ytick=100;
                    }

                    //lines=w.backlines();

                    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
                        ty=(i - miny) * ymult;
                        if (dy<5) {
                            fd=Format(i*m_yaxis_scale,2);
                        } else {
                            fd=Format(i*m_yaxis_scale,1);
                        }

                        GetTextExtent(fd,x,y);

                        if (x>labelW) labelW=x;
                        h=(height-2)-ty;
                        h+=yh;
            #ifndef Q_OS_MAC
                        // stupid pixel alignment rubbish, I really should be using floats..
                        h+=1;
            #endif
                        if (h<0)
                            continue;

                        paint.setBrush(Qt::black);
                        paint.drawText(width-8-x,h+y/2,fd);

                        paint.setPen(m_line_color);
                        paint.drawLine(width-4,h,width,h);

                        double z=(min_ytick/4)*ymult;
                        double g=h;
                        for (int i=0;i<3;i++) {
                            g+=z;
                            if (g>height+yh) break;
                            paint.drawLine(width-3,g,width,g);
                        }
                    }
                    paint.end();
                    m_image=QGLWidget::convertToGLFormat(m_image);
                    m_textureID=w.graphView()->bindTexture(m_image,GL_TEXTURE_2D,GL_RGBA,QGLContext::NoBindOption);
                    w.invalidate_yAxisImage=false;
                }

                if (!m_image.isNull()) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_TEXTURE_2D);
                    w.graphView()->drawTexture(QPoint(left,(top+height)-m_image.height()+5),m_textureID);
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_BLEND);
                }
        */
    } else {
        if (height < 0) { return; }

        if (height > 2000) { return; }

        int labelW = 0;

        EventDataType miny;
        EventDataType maxy;

        if (w.zoomY() == 0 && PROFILE.appearance->allowYAxisScaling()) {
            miny = w.physMinY();
            maxy = w.physMaxY();
        } else {

            miny = w.min_y;
            maxy = w.max_y;

            if (miny < 0) { // even it up if it's starts negative
                miny = -MAX(fabs(miny), fabs(maxy));
            }
        }

        w.roundY(miny, maxy);

        EventDataType dy = maxy - miny;

        static QString fd = "0";
        GetTextExtent(fd, x, y);

        double max_yticks = round(height / (y + 14.0)); // plus spacing between lines

        double mxy = MAX(fabs(maxy), fabs(miny));
        double mny = miny;

        if (miny < 0) {
            mny = -mxy;
        }

        double rxy = mxy - mny;

        int myt;
        bool fnd = false;

        for (myt = max_yticks; myt > 2; myt--) {
            float v = rxy / float(myt);

            if (v == int(v)) {
                fnd = true;
                break;
            }
        }

        if (fnd) { max_yticks = myt; }

        double yt = 1 / max_yticks;

        double ymult = height / rxy;

        double min_ytick = rxy * yt;


        //if (dy>5) {
        //    min_ytick=round(min_ytick);
        //} else {

        //}

        float ty, h;

        if (min_ytick <= 0) {
            qDebug() << "min_ytick error in gYAxis::paint() in" << w.title();
            return;
        }

        if (min_ytick >= 1000000) {
            min_ytick = 100;
        }

        QVector<QLine> ticks;

        for (double i = miny; i <= maxy + min_ytick - 0.00001; i += min_ytick) {
            ty = (i - miny) * ymult;

            if (dy < 5) {
                fd = Format(i * m_yaxis_scale, 2);
            } else {
                fd = Format(i * m_yaxis_scale, 1);
            }

            GetTextExtent(fd, x, y); // performance bottleneck..

            if (x > labelW) { labelW = x; }

            h = top + height - ty;

            if (h < top) { continue; }

            w.renderText(fd, left + width - 8 - x, (h + (y / 2.0)), 0, m_text_color, defaultfont);

            ticks.append(QLine(left + width - 4, h, left + width, h));

            double z = (min_ytick / 4) * ymult;
            double g = h;

            for (int i = 0; i < 3; i++) {
                g += z;

                if (g > top + height) { break; }

                ticks.append(QLine(left + width - 3, g, left + width, g));
            }
        }
        painter.setPen(m_line_color);
        painter.drawLines(ticks);
        w.graphView()->lines_drawn_this_frame += ticks.size();

    }
}
const QString gYAxis::Format(EventDataType v, int dp)
{
    return QString::number(v, 'f', dp);
}

bool gYAxis::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    if (!p_profile->appearance->graphTooltips()) {
        return false;
    }

    int x = event->x();
    int y = event->y();

    if (!graph->units().isEmpty()) {
        graph->ToolTip(graph->units(), x, y - 20, 0);
        //   graph->redraw();
    }

    return true;
}

bool gYAxis::mouseDoubleClickEvent(QMouseEvent *event, gGraph *graph)
{
    if (graph) {
        //        int x=event->x();
        //        int y=event->y();
        short z = (graph->zoomY() + 1) % gGraph::maxZoomY;
        graph->setZoomY(z);
        qDebug() << "Mouse double clicked for" << graph->title() << z;
    }

    Q_UNUSED(event);
    return false;
}

const QString gYAxisTime::Format(EventDataType v, int dp)
{
    int h = int(v) % 24;
    int m = int(v * 60) % 60;
    int s = int(v * 3600) % 60;

    char pm[3] = {"am"};

    if (show_12hr) {

        h >= 12 ? pm[0] = 'p' : pm[0] = 'a';
        h %= 12;

        if (h == 0) { h = 12; }
    } else {
        pm[0] = 0;
    }

    if (dp > 2) { return QString().sprintf("%02i:%02i:%02i%s", h, m, s, pm); }

    return QString().sprintf("%i:%02i%s", h, m, pm);
}

const QString gYAxisWeight::Format(EventDataType v, int dp)
{
    Q_UNUSED(dp)
    return weightString(v, m_unitsystem);
}
