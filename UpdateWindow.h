#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QTableWidgetItem>
#include <QMenu>
#include <QMainWindow>

#include "version.h"
#include "updateparser.h"

namespace Ui {
class UpdateWindow;
}

enum RequestMode { RM_None, RM_CheckUpdates, RM_GetFile, RM_UpdateQT };

class UpdateWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit UpdateWindow(QWidget *parent = 0);
    ~UpdateWindow();
    void checkForUpdates();
    void ParseUpdateXML(QIODevice * dev);

protected slots:
    void replyFinished(QNetworkReply * reply);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void dataReceived();
    void requestFile();

private slots:
    void on_CloseButton_clicked();

    void on_upgradeButton_clicked();

    void upgradeNext();

    void on_FinishedButton_clicked();

private:
    UpdateParser updateparser;

    Ui::UpdateWindow *ui;
    QSystemTrayIcon *systray;
    QMenu *systraymenu;
    RequestMode requestmode;
    QTime dltime;
    QString needQtVersion;
    Update *update;
    Release *release;
    QFile file;
    QNetworkAccessManager *netmanager;
    QNetworkReply * reply;
    QList<Update *> updates;
    int current_row;
    bool success;
};

#endif // UPDATEWINDOW_H
