/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gDailySummary Graph Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <cmath>
#include <QFontMetrics>
#include "gdailysummary.h"
#include "Graphs/gGraph.h"
#include "Graphs/gGraphView.h"
#include "SleepLib/profiles.h"

gDailySummary::gDailySummary() : Layer(NoChannel)
{
}

void gDailySummary::SetDay(Day *day)
{
    QList<ChannelID> piechans;

    piechans.append(CPAP_ClearAirway);
    piechans.append(CPAP_Obstructive);
    piechans.append(CPAP_Apnea);
    piechans.append(CPAP_Hypopnea);
    piechans.append(CPAP_RERA);
    piechans.append(CPAP_FlowLimit);

    pie_data.clear();
    pie_chan.clear();
    pie_labels.clear();
    pie_total = 0;

    m_day = day;
    if (day) {
        m_minx = m_day->first();
        m_maxx = m_day->last();;

        quint32 zchans = schema::SPAN | schema::FLAG;
        bool show_minors = true;
        if (p_profile->general->showUnknownFlags()) zchans |= schema::UNKNOWN;

        if (show_minors) zchans |= schema::MINOR_FLAG;
        QList<ChannelID> available = day->getSortedMachineChannels(zchans);

        flag_values.clear();
        flag_background.clear();
        flag_foreground.clear();
        flag_labels.clear();
        flag_codes.clear();

        EventDataType val;
        EventDataType hours = day->hours();

        int x,y;
        flag_value_width = flag_label_width = flag_height = 0;
        for (int i=0; i < available.size(); ++i) {
            ChannelID code = available.at(i);
            schema::Channel & chan = schema::channel[code];
            QString str;
            if (chan.type() == schema::SPAN) {
                val = (100.0 / hours)*(day->sum(code)/3600.0);
                str = QString("%1%").arg(val,0,'f',2);
            } else {
                val = day->count(code) / hours;
                str = QString("%1").arg(val,0,'f',2);
            }
            flag_values.push_back(str);
            flag_codes.push_back(code);
            flag_background.push_back(chan.defaultColor());
            flag_foreground.push_back((brightness(chan.defaultColor()) < 0.3) ? Qt::white : Qt::black); // pick a contrasting color
            QString label = chan.fullname();

            if (piechans.contains(code)) {
                pie_data.push_back(val);
                pie_labels.push_back(chan.label());
                pie_chan.append(code);
                pie_total += val;
            }

            flag_labels.push_back(label);
            GetTextExtent(label, x, y, defaultfont);

            // Update maximum text boundaries
            if (y > flag_height) flag_height = y;
            if (x > flag_label_width) flag_label_width  = x;

            GetTextExtent(str, x, y, defaultfont);
            if (x > flag_value_width) flag_value_width = x;
            if (y > flag_height) flag_height = y;
        }

        info_labels.clear();
        info_values.clear();

        ahi = day->calcAHI();

        QDateTime dt = QDateTime::fromMSecsSinceEpoch(day->first());
        info_labels.append(QObject::tr("Date"));
        info_values.append(dt.date().toString(Qt::LocaleDate));
        info_labels.append(QObject::tr("Sleep"));
        info_values.append(dt.time().toString());
        QDateTime wake = QDateTime::fromMSecsSinceEpoch(day->last());
        info_labels.append(QObject::tr("Wake"));
        info_values.append(wake.time().toString());
        int secs = hours * 3600.0;
        int h = secs / 3600;
        int m = secs / 60 % 60;
        int s  = secs % 60;
        info_labels.append(QObject::tr("Hours"));
        info_values.append(QString().sprintf("%ih, %im, %is",h,m,s));

        info_value_width = info_label_width = info_height = 0;

        for (int i=0; i < info_labels.size(); ++i) {
            GetTextExtent(info_labels.at(i), x, y, mediumfont);
            if (y > info_height) info_height = y;
            if (x > info_label_width) info_label_width  = x;

            GetTextExtent(info_values.at(i), x, y, mediumfont);
            if (y > info_height) info_height = y;
            if (x > info_value_width) info_value_width  = x;
        }

        m_minimum_height = flag_values.size() * flag_height;

        m_empty = !(day->channelExists(CPAP_Pressure) || day->channelExists(CPAP_IPAP));


    } else {
        m_minx = m_maxx = 0;
        m_miny = m_maxy = 0;
        m_empty = true;

        m_day = nullptr;
    }
}

bool gDailySummary::isEmpty()
{
    return m_empty;
}


void gDailySummary::paint(QPainter &painter, gGraph &w, const QRegion &region)
{

    QRect rect = region.boundingRect();

    int top = rect.top()-10;
    int left = rect.left();
    int width = rect.width();
    int height = rect.height()+10;

    // Draw bounding box
    painter.setPen(QColor(Qt::black));
 //   painter.drawRect(QRect(left,top,width,height),5,5);


    QRectF rect1, rect2;
    int size;


//    QFontMetrics fm(*mediumfont);
//    top += fm.height();
//    painter.setFont(*mediumfont);
//    size = info_values.size();
//
//    for (int i=0; i < size; ++i) {
//        rect1 = QRect(0,0,200,100), rect2 = QRect(0,0,200,100);
//        rect1 = painter.boundingRect(rect1, info_labels.at(i));
//        w.renderText(info_labels.at(i), column, row, 0, Qt::black, mediumfont);

//        rect2 = painter.boundingRect(rect2, info_values.at(i));
//        w.renderText(info_values.at(i), column, row + rect1.height(), 0, Qt::black, mediumfont);
//        column += qMax(rect1.width(), rect2.width()) + 15;
//    }
//    row += rect1.height()+rect2.height()-5;
//    column = left + 10;

    float row = top + 10;
    float column = left+10;

    rect1 = QRectF(column - 10, row -5, 0, 0);
    painter.setFont(*mediumfont);
    QString txt = QString::number(ahi, 'f', 2);
    QString ahi = QString("%1: %2").arg(STR_TR_AHI).arg(txt);
    rect1 = painter.boundingRect(rect1, Qt::AlignTop || Qt::AlignLeft, ahi);
    rect1.setWidth(rect1.width()*2);
    rect1.setHeight(rect1.height() * 1.5);
    painter.fillRect(rect1, QColor("orange"));
    painter.setPen(Qt::black);
    painter.drawText(rect1, Qt::AlignCenter, ahi);
    painter.drawRoundedRect(rect1, 5, 5);

    column += rect1.width() + 10;

    size = flag_values.size();
    int vis = 0;
    for (int i=0; i < size; ++i) {
        schema::Channel & chan = schema::channel[flag_codes.at(i)];
        if (chan.enabled()) vis++;
    }

    flag_value_width = 0;
    flag_label_width = 0;
    flag_height = 0;

    float hpl = float(height-20) / float(vis);
    QFont font(defaultfont->family());
    font.setPixelSize(hpl*0.75);

    font.setBold(true);
    font.setItalic(true);
    painter.setFont(font);

    for (int i=0; i < size; ++i) {
        rect1 = QRectF(0,0,0,0), rect2 = QRectF(0,0,0,0);

        rect1 = painter.boundingRect(rect1, Qt::AlignLeft | Qt::AlignTop, flag_labels.at(i));
        rect2 = painter.boundingRect(rect2, Qt::AlignLeft | Qt::AlignTop, flag_values.at(i));

        if (rect1.width() > flag_label_width) flag_label_width = rect1.width();
        if (rect2.width() > flag_value_width) flag_value_width = rect2.width();
        if (rect1.height() > flag_height) flag_height = rect1.height();
        if (rect2.height() > flag_height) flag_height = rect2.height();

    }
    flag_height = hpl;


    QRect flag_outline(column -5, row -5, (flag_value_width + flag_label_width + 20 + 4) + 10, (hpl * vis) + 10);
    painter.setPen(QPen(Qt::gray, 1));
    painter.drawRoundedRect(flag_outline, 5, 5);

    font.setBold(false);
    font.setItalic(false);
    painter.setFont(font);


    for (int i=0; i < size; ++i) {
        schema::Channel & chan = schema::channel[flag_codes.at(i)];
        if (!chan.enabled()) continue;
        painter.setPen(flag_foreground.at(i));

        QRectF box(column, floor(row) , (flag_value_width + flag_label_width + 20 + 4), ceil(flag_height));
        painter.fillRect(box, QBrush(flag_background.at(i)));
        if (box.contains(w.graphView()->currentMousePos())) {
            w.ToolTip(chan.description(), w.graphView()->currentMousePos().x()+5, w.graphView()->currentMousePos().y(), TT_AlignLeft);
            font.setBold(true);
            font.setItalic(true);
            painter.setFont(font);
            QRect rect1 = QRect(column+2, row , flag_label_width, ceil(hpl));
            painter.drawText(rect1, Qt::AlignVCenter, flag_labels.at(i));
            QRect rect2 = QRect(column+2 + flag_label_width + 20, row, flag_value_width, ceil(hpl));
            painter.drawText(rect2, Qt::AlignVCenter, flag_values.at(i));
            font.setBold(false);
            font.setItalic(false);
            painter.setFont(font);
        } else {
            QRect rect1 = QRect(column+2, row , flag_label_width, ceil(hpl));
            painter.drawText(rect1, Qt::AlignVCenter, flag_labels.at(i));
            QRect rect2 = QRect(column+2 + flag_label_width + 20, row, flag_value_width, ceil(hpl));
            painter.drawText(rect2, Qt::AlignVCenter, flag_values.at(i));
        }

        row += (flag_height);

    }
    column += 22 + flag_label_width + flag_value_width + 20;
    row = top + 10;


    ////////////////////////////////////////////////////////////////////////////////
    // Pie Chart
    ////////////////////////////////////////////////////////////////////////////////
    painter.setRenderHint(QPainter::Antialiasing);
    QRect pierect(column, row, height-30, height-30);

    float sum = -90.0;

    int slices = pie_data.size();
    EventDataType data;
    for (int i=0; i < slices; ++i) {
        data = pie_data[i];

        if (data == 0) { continue; }

        // Setup the shiny radial gradient
        float len = 360.0 / float(pie_total) * float(data);
        QColor col = schema::channel[pie_chan[i]].defaultColor();

        painter.setPen(QPen(col, 0));
        QRadialGradient gradient(pierect.center(), float(pierect.width()) / 2.0, pierect.center());
        gradient.setColorAt(0, Qt::white);
        gradient.setColorAt(1, col);

        // draw filled pie
        painter.setBrush(gradient);
        painter.setBackgroundMode(Qt::OpaqueMode);
        painter.drawPie(pierect, -sum * 16.0, -len * 16.0);

        // draw outline
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setBrush(QBrush(col,Qt::NoBrush));
        painter.setPen(QPen(QColor(Qt::black),1.5));
        painter.drawPie(pierect, -sum * 16.0, -len * 16.0);
        sum += len;

    }

}

bool gDailySummary::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}
bool gDailySummary::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

bool gDailySummary::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}
