/*
 Create New Profile Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

namespace Ui {
    class NewProfile;
}

class NewProfile : public QDialog
{
    Q_OBJECT

public:
    explicit NewProfile(QWidget *parent = 0);
    ~NewProfile();
    void skipWelcomeScreen();
    void edit(const QString name);
private slots:
    void on_nextButton_clicked();

    void on_backButton_clicked();

    void on_cpapModeCombo_activated(int index);

    void on_agreeCheckbox_clicked(bool checked);

    void on_passwordEdit1_editingFinished();

    void on_passwordEdit2_editingFinished();

private:
    Ui::NewProfile *ui;
    bool m_editMode;
    int m_firstPage;
    bool m_passwordHashed;
};

#endif // NEWPROFILE_H
