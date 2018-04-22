#ifndef PROFILESELECTOR_H
#define PROFILESELECTOR_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace Ui {
class ProfileSelector;
}

class MySortFilterProxyModel2:public QSortFilterProxyModel
{
    Q_OBJECT
  public:
    MySortFilterProxyModel2(QObject *parent = 0);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

class ProfileSelector : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileSelector(QWidget *parent = 0);
    ~ProfileSelector();

    void updateProfileList();
    void SelectProfile(QString profname);
    void updateProfileHighlight(QString name);

private slots:
    void on_profileView_doubleClicked(const QModelIndex &index);

    void on_profileFilter_textChanged(const QString &arg1);

    void on_buttonOpenProfile_clicked();

    void on_buttonEditProfile_clicked();

    void on_buttonNewProfile_clicked();

    void on_buttonDestroyProfile_clicked();

private:
    Ui::ProfileSelector *ui;
    QStandardItemModel *model;
    MySortFilterProxyModel2 *proxy;

};

#endif // PROFILESELECTOR_H
