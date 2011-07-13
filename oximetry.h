#ifndef OXIMETRY_H
#define OXIMETRY_H

#include <QWidget>

namespace Ui {
    class Oximetry;
}

class Oximetry : public QWidget
{
    Q_OBJECT

public:
    explicit Oximetry(QWidget *parent = 0);
    ~Oximetry();

private slots:
    void on_RefreshPortsButton_clicked();

private:
    Ui::Oximetry *ui;
};

#endif // OXIMETRY_H
