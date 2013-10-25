/*
 Profile Select Header (Login Screen)
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef PROFILESELECT_H
#define PROFILESELECT_H

#include <QDialog>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QMenu>

namespace Ui {
    class ProfileSelect;
}

/*! \class ProfileSelect
    \brief Simple Login Window providing a list of all profiles to select from
    */
class ProfileSelect : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileSelect(QWidget *parent = 0);
    ~ProfileSelect();

    QString selectedProfile();
    void QuickLogin();
private slots:
    void on_selectButton_clicked();

    void on_newProfileButton_clicked();

    void on_listView_activated(const QModelIndex &index);
    void earlyExit();

    void openProfile();
    void editProfile();
    void deleteProfile();

    void on_listView_customContextMenuRequested(const QPoint &pos);

    void on_pushButton_clicked();

private:
    Ui::ProfileSelect *ui;
    QString m_selectedProfile;
    int m_tries;
    QMenu *popupMenu;
    QStandardItemModel *model;
};

#endif // PROFILESELECT_H
