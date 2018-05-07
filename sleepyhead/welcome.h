#ifndef WELCOME_H
#define WELCOME_H

#include <QWidget>

namespace Ui {
class Welcome;
}

class Welcome : public QWidget
{
    Q_OBJECT

public:
    explicit Welcome(QWidget *parent = 0);
    ~Welcome();

private slots:
    void on_dailyButton_clicked();

    void on_overviewButton_clicked();

    void on_statisticsButton_clicked();

    void on_oximetryButton_clicked();

    void on_importButton_clicked();

private:
    QString GenerateCPAPHTML();
    QString GenerateOxiHTML();
    Ui::Welcome *ui;
};

#endif // WELCOME_H
