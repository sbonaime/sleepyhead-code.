#ifndef PROFILESELECT_H
#define PROFILESELECT_H

#include <QDialog>
#include <QModelIndex>
#include <QMenu>

namespace Ui {
    class ProfileSelect;
}

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

private:
    Ui::ProfileSelect *ui;
    QString m_selectedProfile;
    int m_tries;
    QMenu *popupMenu;
};

#endif // PROFILESELECT_H
