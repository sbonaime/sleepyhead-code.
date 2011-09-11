/*
 SleepyHead Preferences Dialog GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include "SleepLib/profiles.h"

namespace Ui {
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent, Profile * _profile);
    ~PreferencesDialog();
    void Save();
private slots:
    void on_eventTable_doubleClicked(const QModelIndex &index);
    void on_combineSlider_sliderMoved(int position);

    void on_IgnoreSlider_sliderMoved(int position);

private:
    Ui::PreferencesDialog *ui;
    Profile * profile;
};

#endif // PREFERENCESDIALOG_H
