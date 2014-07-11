/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Reports/Printing Module
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QProgressBar>
#include <QApplication>
#include <cmath>

#include "reports.h"
#include "mainwindow.h"
#include "common_gui.h"

extern QProgressBar *qprogress;
extern MainWindow *mainwin;


Report::Report()
{
}

void Report::PrintReport(gGraphView *gv, QString name, QDate date)
{
    if (!gv) { return; }

    Session *journal = nullptr;
    //QDate d=QDate::currentDate();

    int visgraphs = gv->visibleGraphs();

    if (visgraphs == 0) {
        mainwin->Notify(QObject::tr("There are no graphs visible to print"));
        return;
    }

    QString username = p_profile->Get(QString("_{") + QString(STR_UI_UserName) + "}_");

    bool print_bookmarks = false;

    if (name == STR_TR_Daily) {
        QVariantList book_start;
        journal = mainwin->getDaily()->GetJournalSession(mainwin->getDaily()->getDate());

        if (journal && journal->settings.contains(Bookmark_Start)) {
            book_start = journal->settings[Bookmark_Start].toList();

            if (book_start.size() > 0) {
                if (QMessageBox::question(mainwin, STR_TR_Bookmarks,
                                          QObject::tr("Would you like to show bookmarked areas in this report?"), QMessageBox::Yes,
                                          QMessageBox::No) == QMessageBox::Yes) {
                    print_bookmarks = true;
                }
            }
        }
    }

    QPrinter *printer;

    bool aa_setting = p_profile->appearance->antiAliasing();

    bool force_antialiasing = aa_setting;

    printer = new QPrinter(QPrinter::HighResolution);

#ifdef Q_WS_X11
    printer->setPrinterName("Print to File (PDF)");
    printer->setOutputFormat(QPrinter::PdfFormat);
    QString filename = PREF.Get("{home}/" + name + username + date.toString(Qt::ISODate) + ".pdf");

    printer->setOutputFileName(filename);
#endif
    printer->setPrintRange(QPrinter::AllPages);
    printer->setOrientation(QPrinter::Portrait);
    printer->setFullPage(false); // This has nothing to do with scaling
    printer->setNumCopies(1);
    printer->setPageMargins(10, 10, 10, 10, QPrinter::Millimeter);
    QPrintDialog dialog(printer);
#ifdef Q_OS_MAC
    // QTBUG-17913
    QApplication::processEvents();
#endif

    if (dialog.exec() != QDialog::Accepted) {
        delete printer;
        return;
    }

    mainwin->Notify(
        QObject::tr("This make take some time to complete..\nPlease don't touch anything until it's done."),
        QObject::tr("Printing %1 Report").arg(name), 20000);
    QPainter painter;
    painter.begin(printer);

    GLint gw;
    gw = 2048; // Rough guess.. No GL_MAX_RENDERBUFFER_SIZE in mingw.. :(

    //QSizeF pxres=printer->paperSize(QPrinter::DevicePixel);
    QRect prect = printer->pageRect();
    float ratio = float(prect.height()) / float(prect.width());
    float virt_width = gw;
    float virt_height = virt_width * ratio;
    painter.setWindow(0, 0, virt_width, virt_height);
    painter.setViewport(0, 0, prect.width(), prect.height());
    painter.setViewTransformEnabled(true);

    QFont report_font = *defaultfont;
    QFont medium_font = *mediumfont;
    QFont title_font = *bigfont;
    float normal_height = 30; //fm2.ascent();
    report_font.setPixelSize(normal_height);
    medium_font.setPixelSize(40);
    title_font.setPixelSize(90);
    painter.setFont(report_font);

    //QFontMetrics fm2(*defaultfont);

    qDebug() << "Printer Resolution is" << virt_width << "x" << virt_height;

    const int graphs_per_page = 6;
    float full_graph_height = (virt_height - (normal_height * graphs_per_page)) / float(
                                  graphs_per_page);

    QString title = QObject::tr("%1 Report").arg(name);
    painter.setFont(title_font);
    int top = 0;
    QRectF bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), title,
                                         QTextOption(Qt::AlignHCenter | Qt::AlignTop));
    painter.drawText(bounds, title, QTextOption(Qt::AlignHCenter | Qt::AlignTop));
    top += bounds.height() + normal_height / 2.0;
    painter.setFont(report_font);

    int maxy = 0;

    if (!p_profile->user->firstName().isEmpty()) {
        QString userinfo = STR_TR_Name + QString(":\t %1, %2\n").arg(p_profile->user->lastName()).arg(
                               p_profile->user->firstName());
        userinfo += STR_TR_DOB + QString(":\t%1\n").arg(p_profile->user->DOB().toString(
                        Qt::SystemLocaleShortDate));

        if (!p_profile->doctor->patientID().isEmpty()) { userinfo += STR_TR_PatientID + QString(":\t%1\n").arg(p_profile->doctor->patientID()); }

        userinfo += STR_TR_Phone + QString(":\t%1\n").arg(p_profile->user->phone());
        userinfo += STR_TR_Email + QString(":\t%1\n").arg(p_profile->user->email());

        if (!p_profile->user->address().isEmpty()) { userinfo += "\n" + STR_TR_Address + QString(":\n%1").arg(p_profile->user->address()); }

        QRectF bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), userinfo,
                                             QTextOption(Qt::AlignLeft | Qt::AlignTop));
        painter.drawText(bounds, userinfo, QTextOption(Qt::AlignLeft | Qt::AlignTop));

        if (bounds.height() > maxy) { maxy = bounds.height(); }
    }

    Day *cpap = nullptr, *oxi = nullptr;

    int graph_slots = 0;

    if (name == STR_TR_Daily) {
        cpap = p_profile->GetGoodDay(date, MT_CPAP);
        oxi = p_profile->GetGoodDay(date, MT_OXIMETER);
        QString cpapinfo = date.toString(Qt::SystemLocaleLongDate) + "\n\n";

        if (cpap) {
            time_t f = cpap->first() / 1000L;
            time_t l = cpap->last() / 1000L;
            int tt = qint64(cpap->total_time()) / 1000L;
            int h = tt / 3600;
            int m = (tt / 60) % 60;
            int s = tt % 60;

            cpapinfo += STR_TR_MaskTime + QObject::tr(": %1 hours, %2 minutes, %3 seconds\n").arg(h).arg(
                            m).arg(s);
            cpapinfo += STR_TR_BedTime + ": " + QDateTime::fromTime_t(f).time().toString("HH:mm:ss") + " ";
            cpapinfo += STR_TR_WakeUp + ": " + QDateTime::fromTime_t(l).time().toString("HH:mm:ss") + "\n\n";
            QString submodel;
            cpapinfo += STR_TR_Machine + ": ";

            if (cpap->machine->properties.find(STR_PROP_SubModel) != cpap->machine->properties.end()) {
                submodel = "\n" + cpap->machine->properties[STR_PROP_SubModel];
            }

            cpapinfo += cpap->machine->properties[STR_PROP_Brand] + " " +
                        cpap->machine->properties[STR_PROP_Model] + submodel;
            CPAPMode mode = (CPAPMode)(int)cpap->settings_max(CPAP_Mode);
            cpapinfo += "\n" + STR_TR_Mode + ": ";

            if (mode == MODE_CPAP) {
                EventDataType min = round(cpap->settings_wavg(CPAP_Pressure) * 2) / 2.0;
                cpapinfo += STR_TR_CPAP + " " + QString::number(min) + STR_UNIT_CMH2O;
            } else if (mode == MODE_APAP) {
                EventDataType min = cpap->settings_min(CPAP_PressureMin);
                EventDataType max = cpap->settings_max(CPAP_PressureMax);
                cpapinfo += STR_TR_APAP + " " + QString::number(min) + "-" + QString::number(max) + STR_UNIT_CMH2O;
            } else if (mode == MODE_BIPAP) {
                EventDataType epap = cpap->settings_min(CPAP_EPAP);
                EventDataType ipap = cpap->settings_max(CPAP_IPAP);
                EventDataType ps = cpap->settings_max(CPAP_PS);
                cpapinfo += STR_TR_BiLevel + QString("\n" + STR_TR_EPAP + ": %1 " + STR_TR_IPAP + ": %2 %3\n" +
                                                     STR_TR_PS + ": %4")
                            .arg(epap, 0, 'f', 1).arg(ipap, 0, 'f', 1).arg(STR_UNIT_CMH2O).arg(ps, 0, 'f', 1);
            } else if (mode == MODE_ASV) {
                EventDataType epap = cpap->settings_min(CPAP_EPAP);
                EventDataType low = cpap->settings_min(CPAP_IPAPLo);
                EventDataType high = cpap->settings_max(CPAP_IPAPHi);
                EventDataType psl = cpap->settings_min(CPAP_PSMin);
                EventDataType psh = cpap->settings_max(CPAP_PSMax);
                cpapinfo += STR_TR_ASV + QString("\n" + STR_TR_EPAP + ": %1 " + STR_TR_IPAP + ": %2 - %3 %4\n" +
                                                 STR_TR_PS + ": %5 / %6")
                            .arg(epap, 0, 'f', 1)
                            .arg(low, 0, 'f', 1)
                            .arg(high, 0, 'f', 1)
                            .arg(STR_UNIT_CMH2O)
                            .arg(psl, 0, 'f', 1)
                            .arg(psh, 0, 'f', 1);
            } else { cpapinfo += STR_TR_Unknown; }

            float ahi = (cpap->count(CPAP_Obstructive) + cpap->count(CPAP_Hypopnea) + cpap->count(
                             CPAP_ClearAirway) + cpap->count(CPAP_Apnea));

            if (p_profile->general->calculateRDI()) { ahi += cpap->count(CPAP_RERA); }

            ahi /= cpap->hours();
            float csr = (100.0 / cpap->hours()) * (cpap->sum(CPAP_CSR) / 3600.0);
            float uai = cpap->count(CPAP_Apnea) / cpap->hours();
            float oai = cpap->count(CPAP_Obstructive) / cpap->hours();
            float hi = (cpap->count(CPAP_ExP) + cpap->count(CPAP_Hypopnea)) / cpap->hours();
            float cai = cpap->count(CPAP_ClearAirway) / cpap->hours();
            float rei = cpap->count(CPAP_RERA) / cpap->hours();
            float vsi = cpap->count(CPAP_VSnore) / cpap->hours();
            float fli = cpap->count(CPAP_FlowLimit) / cpap->hours();
//            float sai = cpap->count(CPAP_SensAwake) / cpap->hours();
            float nri = cpap->count(CPAP_NRI) / cpap->hours();
            float lki = cpap->count(CPAP_LeakFlag) / cpap->hours();
            float exp = cpap->count(CPAP_ExP) / cpap->hours();

            int piesize = (2048.0 / 8.0) * 1.3; // 1.5" in size
            //float fscale=font_scale;
            //if (!highres)
            //                fscale=1;

            QString stats;
            painter.setFont(medium_font);

            if (p_profile->general->calculateRDI()) {
                stats = QObject::tr("RDI\t%1\n").arg(ahi, 0, 'f', 2);
            } else {
                stats = QObject::tr("AHI\t%1\n").arg(ahi, 0, 'f', 2);
            }

            QRectF bounds = painter.boundingRect(QRectF(0, 0, virt_width, 0), stats,
                                                 QTextOption(Qt::AlignRight));
            painter.drawText(bounds, stats, QTextOption(Qt::AlignRight));


            mainwin->getDaily()->eventBreakdownPie()->setShowTitle(false);
            mainwin->getDaily()->eventBreakdownPie()->setMargins(0, 0, 0, 0);
            QPixmap ebp;

            if (ahi > 0) {
                ebp = mainwin->getDaily()->eventBreakdownPie()->renderPixmap(piesize, piesize, true);
            } else {
                ebp = QPixmap(":/icons/smileyface.png");
            }

            if (!ebp.isNull()) {
                painter.drawPixmap(virt_width - piesize, bounds.height(), piesize, piesize, ebp);
            }

            mainwin->getDaily()->eventBreakdownPie()->setShowTitle(true);

            cpapinfo += "\n\n";

            painter.setFont(report_font);
            //bounds=painter.boundingRect(QRectF((virt_width/2)-(virt_width/6),top,virt_width/2,0),cpapinfo,QTextOption(Qt::AlignLeft));
            bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), cpapinfo,
                                          QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds, cpapinfo, QTextOption(Qt::AlignHCenter));

            int ttop = bounds.height();

            stats = QObject::tr("AI=%1 HI=%2 CAI=%3 ").arg(oai, 0, 'f', 2).arg(hi, 0, 'f', 2).arg(cai, 0, 'f',
                    2);

            if (cpap->machine->GetClass() == STR_MACH_PRS1) {
                stats += QObject::tr("REI=%1 VSI=%2 FLI=%3 PB/CSR=%4%%")
                         .arg(rei, 0, 'f', 2).arg(vsi, 0, 'f', 2)
                         .arg(fli, 0, 'f', 2).arg(csr, 0, 'f', 2);
            } else if (cpap->machine->GetClass() == STR_MACH_ResMed) {
                stats += QObject::tr("UAI=%1 ").arg(uai, 0, 'f', 2);
            } else if (cpap->machine->GetClass() == STR_MACH_Intellipap) {
                stats += QObject::tr("NRI=%1 LKI=%2 EPI=%3").arg(nri, 0, 'f', 2).arg(lki, 0, 'f', 2).arg(exp, 0,
                         'f', 2);
            }

            bounds = painter.boundingRect(QRectF(0, top + ttop, virt_width, 0), stats,
                                          QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds, stats, QTextOption(Qt::AlignHCenter));
            ttop += bounds.height();

            if (journal) {
                stats = "";

                if (journal->settings.contains(Journal_Weight)) {
                    stats += STR_TR_Weight + QString(" %1 ").arg(weightString(
                                 journal->settings[Journal_Weight].toDouble()));
                }

                if (journal->settings.contains(Journal_BMI)) {
                    stats += STR_TR_BMI + QString(" %1 ").arg(journal->settings[Journal_BMI].toDouble(), 0, 'f', 2);
                }

                if (journal->settings.contains(Journal_ZombieMeter)) {
                    stats += STR_TR_Zombie + QString(" %1/10 ").arg(journal->settings[Journal_ZombieMeter].toDouble(),
                             0, 'f', 0);
                }

                if (!stats.isEmpty()) {
                    bounds = painter.boundingRect(QRectF(0, top + ttop, virt_width, 0), stats,
                                                  QTextOption(Qt::AlignHCenter));

                    painter.drawText(bounds, stats, QTextOption(Qt::AlignHCenter));
                    ttop += bounds.height();
                }

                ttop += normal_height;

                if (journal->settings.contains(Journal_Notes)) {
                    QTextDocument doc;
                    doc.setHtml(journal->settings[Journal_Notes].toString());
                    stats = doc.toPlainText();
                    //doc.drawContents(&painter); // doesn't work as intended..

                    bounds = painter.boundingRect(QRectF(0, top + ttop, virt_width, 0), stats,
                                                  QTextOption(Qt::AlignHCenter));
                    painter.drawText(bounds, stats, QTextOption(Qt::AlignHCenter));
                    bounds.setLeft(virt_width / 4);
                    bounds.setRight(virt_width - (virt_width / 4));

                    QPen pen(Qt::black);
                    pen.setWidth(4);
                    painter.setPen(pen);
                    painter.drawRect(bounds);
                    ttop += bounds.height() + normal_height;
                }
            }


            if (ttop > maxy) { maxy = ttop; }
        } else {
            bounds = painter.boundingRect(QRectF(0, top + maxy, virt_width, 0), cpapinfo,
                                          QTextOption(Qt::AlignCenter));
            painter.drawText(bounds, cpapinfo, QTextOption(Qt::AlignCenter));

            if (maxy + bounds.height() > maxy) { maxy = maxy + bounds.height(); }
        }
    } else if (name == STR_TR_Overview) {
        QDateTime first = QDateTime::fromTime_t((*gv)[0]->min_x / 1000L);
        QDateTime last = QDateTime::fromTime_t((*gv)[0]->max_x / 1000L);
        QString ovinfo = QObject::tr("Reporting from %1 to %2").arg(first.date().toString(
                             Qt::SystemLocaleShortDate)).arg(last.date().toString(Qt::SystemLocaleShortDate));
        QRectF bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), ovinfo,
                                             QTextOption(Qt::AlignHCenter));
        painter.drawText(bounds, ovinfo, QTextOption(Qt::AlignHCenter));

        if (bounds.height() > maxy) { maxy = bounds.height(); }
    } else if (name == STR_TR_Oximetry) {
        QString ovinfo = QObject::tr("Reporting data goes here");
        QRectF bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), ovinfo,
                                             QTextOption(Qt::AlignHCenter));
        painter.drawText(bounds, ovinfo, QTextOption(Qt::AlignHCenter));

        if (bounds.height() > maxy) { maxy = bounds.height(); }
    }

    top += maxy;

    graph_slots = graphs_per_page - ((virt_height - top) / (full_graph_height + normal_height));

    bool first = true;
    QStringList labels;
    QVector<gGraph *> graphs;
    QVector<qint64> start, end;
    qint64 savest, saveet;
    gv->GetXBounds(savest, saveet);
    qint64 st = savest, et = saveet;

    gGraph *g;

    if (name == STR_TR_Daily) {
        if (!print_bookmarks) {
            for (int i = 0; i < gv->size(); i++) {
                g = (*gv)[i];

                if (g->isEmpty()) { continue; }

                if (!g->visible()) { continue; }

                if (cpap) {
                    st = cpap->first();
                    et = cpap->last();
                } else if (oxi) {
                    st = oxi->first();
                    et = oxi->last();
                }

                if (g->title() == STR_TR_FlowRate) {
                    if (!((qAbs(savest - st) < 2000) && (qAbs(saveet - et) < 2000))) {
                        start.push_back(st);
                        end.push_back(et);
                        graphs.push_back(g);
                        labels.push_back(QObject::tr("Entire Day's Flow Waveform"));
                    }

                    start.push_back(savest);
                    end.push_back(saveet);
                    graphs.push_back(g);
                    labels.push_back(QObject::tr("Current Selection"));
                } else {
                    start.push_back(savest);
                    end.push_back(saveet);
                    graphs.push_back(g);
                    labels.push_back("");
                }

            }
        } else {
            const QString EntireDay = QObject::tr("Entire Day");

            if (journal) {
                if (journal->settings.contains(Bookmark_Start)) {
                    QVariantList st1 = journal->settings[Bookmark_Start].toList();
                    QVariantList et1 = journal->settings[Bookmark_End].toList();
                    QStringList notes = journal->settings[Bookmark_Notes].toStringList();
                    gGraph *flow = gv->findGraph(STR_TR_FlowRate),
                            *spo2 = gv->findGraph(STR_TR_SpO2),
                             *pulse = gv->findGraph(STR_TR_PulseRate);


                    if (cpap && flow && !flow->isEmpty() && flow->visible()) {
                        labels.push_back(EntireDay);
                        start.push_back(cpap->first());
                        end.push_back(cpap->last());
                        graphs.push_back(flow);
                    }

                    if (oxi && spo2 && !spo2->isEmpty() && spo2->visible()) {
                        labels.push_back(EntireDay);
                        start.push_back(oxi->first());
                        end.push_back(oxi->last());
                        graphs.push_back(spo2);
                    }

                    if (oxi && pulse && !pulse->isEmpty() && pulse->visible()) {
                        labels.push_back(EntireDay);
                        start.push_back(oxi->first());
                        end.push_back(oxi->last());
                        graphs.push_back(pulse);
                    }

                    for (int i = 0; i < notes.size(); i++) {
                        if (flow && !flow->isEmpty() && flow->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(flow);
                        }

                        if (spo2 && !spo2->isEmpty() && spo2->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(spo2);
                        }

                        if (pulse && !pulse->isEmpty() && pulse->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(pulse);
                        }
                    }
                }
            }

            for (int i = 0; i < gv->size(); i++) {
                gGraph *g = (*gv)[i];

                if (g->isEmpty()) { continue; }

                if (!g->visible()) { continue; }

                if ((g->title() != STR_TR_FlowRate) && (g->title() != STR_TR_SpO2)
                        && (g->title() != STR_TR_PulseRate)) {
                    start.push_back(st);
                    end.push_back(et);
                    graphs.push_back(g);
                    labels.push_back("");
                }
            }
        }
    } else {
        for (int i = 0; i < gv->size(); i++) {
            gGraph *g = (*gv)[i];

            if (g->isEmpty()) { continue; }

            if (!g->visible()) { continue; }

            start.push_back(st);
            end.push_back(et);
            graphs.push_back(g);
            labels.push_back(""); // date range?
        }
    }

    int pages = ceil(float(graphs.size() + graph_slots) / float(graphs_per_page));

    if (qprogress) {
        qprogress->setValue(0);
        qprogress->setMaximum(graphs.size());
        qprogress->show();
    }

    int page = 1;
    int gcnt = 0;

    for (int i = 0; i < graphs.size(); i++) {

        if ((top + full_graph_height + normal_height) > virt_height) {
            top = 0;
            gcnt = 0;
            first = true;

            if (page > pages) {
                break;
            }

            if (!printer->newPage()) {
                qWarning("failed in flushing page to disk, disk full?");
                break;
            }


        }

        if (first) {
            QString footer = QObject::tr("SleepyHead v%1 - http://sleepyhead.sourceforge.net").arg(
                                 VersionString);

            QRectF bounds = painter.boundingRect(QRectF(0, virt_height, virt_width, normal_height), footer,
                                                 QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds, footer, QTextOption(Qt::AlignHCenter));

            QString pagestr = QObject::tr("Page %1 of %2").arg(page).arg(pages);
            QRectF pagebnds = painter.boundingRect(QRectF(0, virt_height, virt_width, normal_height), pagestr,
                                                   QTextOption(Qt::AlignRight));
            painter.drawText(pagebnds, pagestr, QTextOption(Qt::AlignRight));
            first = false;
            page++;
        }

        gGraph *g = graphs[i];
        g->SetXBounds(start[i], end[i]);
        g->deselect();

        QString label = labels[i];

        if (!label.isEmpty()) {
            //label+=":";
            top += normal_height / 3;
            QRectF bounds = painter.boundingRect(QRectF(0, top, virt_width, 0), label,
                                                 QTextOption(Qt::AlignHCenter));
            //QRectF pagebnds=QRectF(0,top,virt_width,normal_height);
            painter.drawText(bounds, label, QTextOption(Qt::AlignHCenter));
            top += bounds.height();
        } else { top += normal_height / 2; }

        p_profile->appearance->setAntiAliasing(force_antialiasing);
        int tmb = g->m_marginbottom;
        g->m_marginbottom = 0;


        //painter.beginNativePainting();
        //g->showTitle(false);
        int hhh = full_graph_height - normal_height;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        QPixmap pm2 = g->renderPixmap(virt_width, hhh, 1);
#else
        QPixmap pm2 = g->renderPixmap(virt_width, hhh, 1);
#endif
        QImage pm = pm2.toImage(); //fscale);
        pm2.detach();
        //g->showTitle(true);
        //painter.endNativePainting();
        g->m_marginbottom = tmb;
        p_profile->appearance->setAntiAliasing(aa_setting);


        if (!pm.isNull()) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            painter.drawImage(QRect(0, top, pm.width(), pm.height()), pm);
#else
            painter.drawImage(0, top, pm);
#endif

            //painter.drawImage(0,top,virt_width,full_graph_height-normal_height,pm);
        }

        top += full_graph_height;

        gcnt++;

        if (qprogress) {
            qprogress->setValue(i);
            QApplication::processEvents();
        }
    }

    gv->SetXBounds(savest, saveet);
    qprogress->hide();
    painter.end();
    delete printer;
    mainwin->Notify(QObject::tr("SleepyHead has finished sending the job to the printer."));
}

