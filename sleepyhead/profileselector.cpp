
#include "profileselector.h"
#include "ui_profileselector.h"

#include "SleepLib/profiles.h"
#include "daily.h"
#include "overview.h"
#include "statistics.h"
#include "mainwindow.h"
#include "newprofile.h"

extern MainWindow * mainwin;

MySortFilterProxyModel2::MySortFilterProxyModel2(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool MySortFilterProxyModel2::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
    QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);
    QModelIndex index5 = sourceModel()->index(sourceRow, 5, sourceParent);

    return (sourceModel()->data(index0).toString().contains(filterRegExp())
            || sourceModel()->data(index1).toString().contains(filterRegExp())
            || sourceModel()->data(index2).toString().contains(filterRegExp())
            || sourceModel()->data(index5).toString().contains(filterRegExp())
           );
}


ProfileSelector::ProfileSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileSelector)
{
    ui->setupUi(this);
    model = nullptr;
    proxy = nullptr;

    ui->versionLabel->setText(VersionString);
}

ProfileSelector::~ProfileSelector()
{
    delete ui;
}


void ProfileSelector::updateProfileList()
{
    QString name;
    int w=0;
    if (proxy) delete proxy;
    if (model) delete model;

    const int columns = 6;
    model = new QStandardItemModel(0, columns, this);
    model->setHeaderData(0, Qt::Horizontal, tr("Profile"));
    model->setHeaderData(1, Qt::Horizontal, tr("Ventilator Brand"));
    model->setHeaderData(2, Qt::Horizontal, tr("Ventilator Model"));
    model->setHeaderData(3, Qt::Horizontal, tr("Other Data"));
    model->setHeaderData(4, Qt::Horizontal, tr("Last Imported"));
    model->setHeaderData(5, Qt::Horizontal, tr("Name"));

    ui->profileView->setStyleSheet("QHeaderView::section { background-color:lightgrey }");

    int row = 0;
    int sel = -1;

    QFontMetrics fm(ui->profileView->font());

    QMap<QString, Profile *>::iterator pi;
    for (pi = Profiles::profiles.begin(); pi != Profiles::profiles.end(); pi++) {
        Profile *prof = pi.value();
        name = pi.key();

        if (AppSetting->profileName() == name) {
            sel = row;
        }

        Machine * mach = prof->GetMachine(MT_CPAP);  // only interested in last cpap machine...
        if (!mach) {
            qDebug() << "Couldn't find machine info for" << name;
        }

        model->insertRows(row, 1, QModelIndex());
        // Problem: Can't access profile details until it's loaded.
        QString usersname;
        if (!prof->user->lastName().isEmpty()) {
            usersname = tr("%1, %2").arg(prof->user->lastName()).arg(prof->user->firstName());
        }

        model->setData(model->index(row, 0, QModelIndex()), name);

        model->setData(model->index(row, 0, QModelIndex()), name, Qt::UserRole+2);
        model->setData(model->index(row, 5, QModelIndex()), usersname);
        if (mach) {
            model->setData(model->index(row, 1, QModelIndex()), mach->brand());
            model->setData(model->index(row, 2, QModelIndex()), mach->series()+" "+mach->model());
            model->setData(model->index(row, 4, QModelIndex()), mach->lastImported().toString(Qt::SystemLocaleShortDate));
        }
        QBrush bg = QColor(Qt::black);
        if (prof == p_profile) {
            bg = QBrush(Qt::red);
        }
        for (int i=0; i<columns; i++) {
            model->setData(model->index(row, i, QModelIndex()), bg, Qt::ForegroundRole);
        }

        QRect rect = fm.boundingRect(name);
        if (rect.width() > w) w = rect.width();

        // Profile fonts arern't loaded yet.. Using generic font.
        //item->setFont(font);
        //model->appendRow(item);
        row++;
    }
    w+=20;
//    ui->profileView->setMinimumWidth(w);

    proxy = new MySortFilterProxyModel2(this);
    proxy->setSourceModel(model);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);


    ui->profileView->setModel(proxy);
    ui->profileView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->profileView->setSelectionMode(QAbstractItemView::SingleSelection);

    QHeaderView *headerView = ui->profileView->horizontalHeader();
    headerView->setStretchLastSection(true);
    headerView->setSectionResizeMode(QHeaderView::Stretch);

    QPalette* palette = new QPalette();
    palette->setColor(QPalette::Highlight,QColor("#3a7fc2"));
    palette->setColor(QPalette::HighlightedText, QColor("white"));

    ui->profileView->setPalette(*palette);


}

void ProfileSelector::updateProfileHighlight(QString name)
{
    QBrush bg = QColor(Qt::black);
    for (int row=0;row < model->rowCount(); row++) {
        for (int i=0; i<model->columnCount(); i++) {
            model->setData(model->index(row, i, QModelIndex()), bg, Qt::ForegroundRole);
        }
    }
    bg = QBrush(Qt::red);
    for (int row=0;row < proxy->rowCount(); row++) {
        if (proxy->data(proxy->index(row, 0, QModelIndex())).toString().compare(name)==0) {
            for (int i=0; i<proxy->columnCount(); i++) {
                proxy->setData(proxy->index(row, i, QModelIndex()), bg, Qt::ForegroundRole);
            }
            break;
        }
    }
}

void ProfileSelector::SelectProfile(QString profname)
{
    qDebug() << "Selecting new profile" << profname;

    Profile * prof = Profiles::profiles[profname];

    if (prof != p_profile) {

        // Unselect everything in ProfileView

        mainwin->OpenProfile(profname);
        updateProfileHighlight(profname);
    }

}

void ProfileSelector::on_profileView_doubleClicked(const QModelIndex &index)
{
    QModelIndex idx = proxy->index(index.row(), 0, QModelIndex());
    QString profname = proxy->data(idx, Qt::UserRole+2).toString();

    SelectProfile(profname);
}

void ProfileSelector::on_profileFilter_textChanged(const QString &arg1)
{
    QRegExp regExp("*"+arg1+"*", Qt::CaseInsensitive, QRegExp::Wildcard);
    proxy->setFilterRegExp(regExp);
}

void ProfileSelector::on_buttonOpenProfile_clicked()
{
    if (ui->profileView->currentIndex().isValid()) {
        QString name = proxy->data(proxy->index(ui->profileView->currentIndex().row(), 0, QModelIndex()), Qt::UserRole+2).toString();
        qDebug() << "Opening" << name;
        SelectProfile(name);
    }
}

void ProfileSelector::on_buttonEditProfile_clicked()
{
    if (ui->profileView->currentIndex().isValid()) {
        QString name = proxy->data(proxy->index(ui->profileView->currentIndex().row(), 0, QModelIndex()), Qt::UserRole+2).toString();
        qDebug() << "Editing" << name;

        Profile * prof = Profiles::profiles[name];
        //SelectProfile(name); // may not be necessary...

        NewProfile *newprof = new NewProfile(this);
        newprof->edit(name);
        newprof->setWindowModality(Qt::ApplicationModal);
        newprof->setModal(true);
        if (newprof->exec() != NewProfile::Rejected) {
            QString usersname;
            if (!prof->user->lastName().isEmpty()) {
                usersname = tr("%1, %2").arg(prof->user->lastName()).arg(prof->user->firstName());
            }

            proxy->setData(proxy->index(ui->profileView->currentIndex().row(), 5, QModelIndex()), usersname);
        }

        delete newprof;
    }
}

void ProfileSelector::on_buttonNewProfile_clicked()
{
    if (p_profile)
        mainwin->CloseProfile();

    NewProfile *newprof = new NewProfile(this);
    newprof->skipWelcomeScreen();
    newprof->setWindowModality(Qt::ApplicationModal);
    newprof->setModal(true);
    if (newprof->exec() == NewProfile::Accepted) {
        p_profile = Profiles::Get(AppSetting->profileName());
        if (p_profile != nullptr) {
            QString name = p_profile->user->userName();
            p_profile = nullptr;
            SelectProfile(name);
        } else {
            qWarning() << AppSetting->profileName() << "yielded a null profile";
            p_profile=nullptr;
        }
        updateProfileList();
    }
    delete newprof;
}

void ProfileSelector::on_buttonDestroyProfile_clicked()
{
    if (ui->profileView->currentIndex().isValid()) {
        QString name = proxy->data(proxy->index(ui->profileView->currentIndex().row(), 0, QModelIndex()), Qt::UserRole+2).toString();
        qDebug() << "Destroying" << name;
    }
}
