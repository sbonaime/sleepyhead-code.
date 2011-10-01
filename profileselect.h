#ifndef PROFILESELECT_H
#define PROFILESELECT_H

#include <QDialog>
#include <QModelIndex>

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
private slots:
    void on_selectButton_clicked();

    void on_newProfileButton_clicked();

    void on_listView_activated(const QModelIndex &index);

private:
    Ui::ProfileSelect *ui;
    QString m_selectedProfile;
    int m_tries;
};

#endif // PROFILESELECT_H
