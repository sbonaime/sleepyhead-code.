/*
 Summary Header
 Copyright (c)2013 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL3
*/

#ifndef SUMMARY_H
#define SUMMARY_H

#include <QObject>

class Summary : public QObject
{
    Q_OBJECT
public:
    explicit Summary(QObject *parent = 0);
    
    static QString GenerateHTML();

signals:
    
public slots:
    
};

#endif // SUMMARY_H
