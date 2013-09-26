/*
 Reports Header
 Copyright (c)2013 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL3
*/

#ifndef REPORTS_H
#define REPORTS_H
#include "Graphs/gGraphView.h"

class Report
{
public:
    Report();

    /*! \fn void PrintReport gGraphView *gv,QString name, QDate date=QDate::currentDate());
        \brief Prepares a report using gGraphView object, and sends to a created QPrinter object
        \param gGraphView *gv  GraphView Object containing which graph set to print
        \param QString name   Report Title
        \param QDate date
        */
    static void PrintReport(gGraphView *gv,QString name, QDate date=QDate::currentDate());
};

#endif // REPORTS_H
