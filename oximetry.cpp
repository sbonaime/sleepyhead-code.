#include <QDebug>
#include "oximetry.h"
#include "ui_oximetry.h"
#include "qextserialport/qextserialenumerator.h"

Oximetry::Oximetry(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Oximetry)
{
    ui->setupUi(this);

    on_RefreshPortsButton_clicked();
}

Oximetry::~Oximetry()
{
    delete ui;
}

void Oximetry::on_RefreshPortsButton_clicked()
{
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    ui->SerialPortsCombo->clear();
    for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).friendName.toUpper().contains("USB"))
            ui->SerialPortsCombo->addItem(ports.at(i).physName);
        //qDebug() << "port name:" << ports.at(i).portName;
        qDebug() << "Serial Port:" << ports.at(i).physName << ports.at(i).friendName;
        //qDebug() << "enumerator name:" << ports.at(i).enumName;
        //qDebug() << "vendor ID:" << QString::number(ports.at(i).vendorID, 16);
        //qDebug() << "product ID:" << QString::number(ports.at(i).productID, 16);
        //qDebug() << "===================================";
    }
}
