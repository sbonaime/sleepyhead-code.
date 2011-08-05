/*
 SleepyHead Preferences Dialog GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QModelIndex>
namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

private slots:
    void on_eventTable_doubleClicked(const QModelIndex &index);

    void on_timeEdit_editingFinished();

    void on_memoryHogCheckbox_toggled(bool checked);

    void on_combineSlider_valueChanged(int value);

    void on_IgnoreSlider_valueChanged(int value);

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
