/* SleepyHead Logger module implementation
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "logger.h"

QThreadPool * otherThreadPool = NULL;

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
void MyOutputHandler(QtMsgType type, const char *msgtxt)
{

#else
void MyOutputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msgtxt)
{
    Q_UNUSED(context)
#endif

    if (!logger) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        fprintf(stderr, "Pre/Post: %s\n", msgtxt.toLocal8Bit().constData());
#else
        fprintf(stderr, "Pre/Post: %s\n", msgtxt);
#endif
        return;
    }

    QString msg, typestr;

    switch (type) {
    case QtWarningMsg:
        typestr = QString("Warning: ");
        break;

    case QtFatalMsg:
        typestr = QString("Fatal: ");
        break;

    case QtCriticalMsg:
        typestr = QString("Critical: ");
        break;

    default:
        typestr = QString("Debug: ");
        break;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    msg = typestr +
          msgtxt; //+QString(" (%1:%2, %3)").arg(context.file).arg(context.line).arg(context.function);
#else
    msg = typestr + msgtxt;
#endif

    if (logger && logger->isRunning()) {
        logger->append(msg);
    } else {
        fprintf(stderr, "%s\n", msg.toLocal8Bit().data());
    }

    if (type == QtFatalMsg) {
        abort();
    }

}

void initializeLogger()
{
    logger = new LogThread();
    otherThreadPool = new QThreadPool();
    bool b = otherThreadPool->tryStart(logger);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    qInstallMessageHandler(MyOutputHandler);
#else
    qInstallMsgHandler(MyOutputHandler);
#endif
    if (b) {
        qWarning() << "Started logging thread";
    } else {
        qWarning() << "Logging thread did not start correctly";
    }

}

void shutdownLogger()
{
    if (logger) {
        logger->quit();
        otherThreadPool->waitForDone(-1);
        logger = NULL;
    }
    delete otherThreadPool;
}

LogThread * logger = NULL;

void LogThread::append(QString msg)
{
    QString tmp = QString("%1: %2").arg(logtime.elapsed(), 5, 10, QChar('0')).arg(msg);
    //QStringList appears not to be threadsafe
    strlock.lock();
    buffer.append(tmp);
    strlock.unlock();
}

void LogThread::appendClean(QString msg)
{
    strlock.lock();
    buffer.append(msg);
    strlock.unlock();
}


void LogThread::quit() {
    qDebug() << "Shutting down logging thread";
    running = false;
    strlock.lock();
    while (!buffer.isEmpty()) {
        QString msg = buffer.takeFirst();
        fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
    }
    strlock.unlock();
}


void LogThread::run()
{
    running = true;
    do {
        strlock.lock();
        //int r = receivers(SIGNAL(outputLog(QString())));
        while (!buffer.isEmpty()) {
            QString msg = buffer.takeFirst();
                fprintf(stderr, "%s\n", msg.toLocal8Bit().data());
                emit outputLog(msg);
        }
        strlock.unlock();
        QThread::msleep(1000);
    } while (running);
}


