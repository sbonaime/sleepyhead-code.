#include <QLabel>
#include <QColorDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QProcess>
#include <QDesktopServices>
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "SleepLib/machine_common.h"

extern QFont * defaultfont;
extern QFont * mediumfont;
extern QFont * bigfont;

PreferencesDialog::PreferencesDialog(QWidget *parent,Profile * _profile) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    profile(_profile)
{
    ui->setupUi(this);

    ui->tabWidget->removeTab(3);

    Q_ASSERT(profile!=NULL);
    ui->tabWidget->setCurrentIndex(0);
    int i=ui->unitCombo->findText((*profile)["UnitSystem"].toString());
    if (i<0) i=0;
    ui->unitCombo->setCurrentIndex(i);

    //i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    //ui->timeZoneCombo->setCurrentIndex(i);

    QTime t=(*profile)["DaySplitTime"].toTime();
    ui->timeEdit->setTime(t);
    int val;

    val=(*profile)["CombineCloserSessions"].toInt();
    ui->combineSlider->setValue(val);
    if (val>0) {
        ui->combineLCD->display(val);
    } else ui->combineLCD->display(tr("OFF"));


    val=(*profile)["IgnoreShorterSessions"].toInt();
    ui->IgnoreSlider->setValue(val);

    ui->applicationFont->setCurrentFont(QApplication::font());
    ui->applicationFont->setFont(QApplication::font());
    ui->applicationFontSize->setValue(QApplication::font().pointSize());
    ui->applicationFontBold->setChecked(QApplication::font().weight()==QFont::Bold);
    ui->applicationFontItalic->setChecked(QApplication::font().italic());

    ui->graphFont->setCurrentFont(*defaultfont);
    ui->graphFont->setFont(*defaultfont);
    ui->graphFontSize->setValue(defaultfont->pointSize());
    ui->graphFontBold->setChecked(defaultfont->weight()==QFont::Bold);
    ui->graphFontItalic->setChecked(defaultfont->italic());

    ui->titleFont->setCurrentFont(*mediumfont);
    ui->titleFont->setFont(*mediumfont);
    ui->titleFontSize->setValue(mediumfont->pointSize());
    ui->titleFontBold->setChecked(mediumfont->weight()==QFont::Bold);
    ui->titleFontItalic->setChecked(mediumfont->italic());

    ui->bigFont->setCurrentFont(*bigfont);
    ui->bigFont->setFont(*bigfont);
    ui->bigFontSize->setValue(bigfont->pointSize());
    ui->bigFontBold->setChecked(bigfont->weight()==QFont::Bold);
    ui->bigFontItalic->setChecked(bigfont->italic());

    if (!(*profile).Exists("SkipEmptyDays")) (*profile)["SkipEmptyDays"]=true;
    ui->skipEmptyDays->setChecked((*profile)["SkipEmptyDays"].toBool());

    if (!(*profile).Exists("SquareWavePlots")) (*profile)["SquareWavePlots"]=true;
    ui->squareWavePlots->setChecked((*profile)["SquareWavePlots"].toBool());

    if (!PREF.Exists("Updates_AutoCheck")) PREF["Updates_AutoCheck"]=true;
    ui->automaticallyCheckUpdates->setChecked(PREF["Updates_AutoCheck"].toBool());

    if (!PREF.Exists("Updates_CheckFrequency")) PREF["Updates_CheckFrequency"]=3;
    ui->updateCheckEvery->setValue(PREF["Updates_CheckFrequency"].toInt());
    if (PREF.Exists("Updates_LastChecked")) {
        RefreshLastChecked();
    } else ui->updateLastChecked->setText("Never");

    if (val>0) {
        ui->IgnoreLCD->display(val);
    } else ui->IgnoreLCD->display(tr("OFF"));

    ui->overlayFlagsCombo->setCurrentIndex((*profile)["AlwaysShowOverlayBars"].toInt());
    ui->useAntiAliasing->setChecked((*profile)["UseAntiAliasing"].toBool());
    ui->memoryHogCheckbox->setChecked((*profile)["MemoryHog"].toBool());
    ui->useGraphSnapshots->setChecked((*profile)["EnableGraphSnapshots"].toBool());
    ui->intentionalLeakEdit->setValue((*profile)["IntentionalLeak"].toDouble());
    ui->useMultithreading->setChecked((*profile)["EnableMultithreading"].toBool());

    ui->oximetryGroupBox->setChecked((*profile)["EnableOximetry"].toBool());
    ui->oximetrySync->setChecked((*profile)["SyncOximetry"].toBool());
    ui->oximetryType->setCurrentIndex(ui->oximetryType->findText((*profile)["OximeterType"].toString(),Qt::MatchExactly));

    ui->eventTable->setColumnWidth(0,40);
    ui->eventTable->setColumnWidth(1,55);
    ui->eventTable->setColumnHidden(3,true);
    int row=0;
    QTableWidgetItem *item;
    QHash<QString, schema::Channel *>::iterator ci;
    for (ci=schema::channel.names.begin();ci!=schema::channel.names.end();ci++) {
        if (ci.value()->type()==schema::DATA) {
            ui->eventTable->insertRow(row);
            int id=ci.value()->id();
            ui->eventTable->setItem(row,3,new QTableWidgetItem(QString::number(id)));
            item=new QTableWidgetItem(ci.value()->description());
            ui->eventTable->setItem(row,2,item);
            QCheckBox *c=new QCheckBox(ui->eventTable);
            c->setChecked(true);
            QLabel *pb=new QLabel(ui->eventTable);
            pb->setText("foo");
            ui->eventTable->setCellWidget(row,0,c);
            ui->eventTable->setCellWidget(row,1,pb);


            QColor a=ci.value()->defaultColor();//(rand() % 255, rand() % 255, rand() % 255, 255);
            QPalette p(a,a,a,a,a,a,a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }
/*    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }*/

    QTreeWidget *tree=ui->graphTree;
    tree->clear();
    tree->setColumnCount(1); // 1 visible common.. (1 hidden)

    QTreeWidgetItem *daily=new QTreeWidgetItem((QTreeWidget *)0,QStringList("Daily Graphs"));
    QTreeWidgetItem *overview=new QTreeWidgetItem((QTreeWidget *)0,QStringList("Overview Graphs"));
    tree->insertTopLevelItem(0,daily);
    tree->insertTopLevelItem(0,overview);
    QTreeWidgetItem *it=new QTreeWidgetItem(daily,QStringList("Event Flags"));//,QTreeWidgetItem::UserType);
    it->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    it->setCheckState(0,Qt::Checked);
    daily->addChild(it);
    //QTreeWidgetItem *root=NULL;//new QTreeWidgetItem((QTreeWidget *)0,QStringList("Stuff"));
    //=new QTreeWidgetItem(root,l);
    //ui->graphTree->setModel(
    tree->sortByColumn(0,Qt::AscendingOrder);

}


PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_eventTable_doubleClicked(const QModelIndex &index)
{
    int row=index.row();
    int col=index.column();
    bool ok;
    int id=ui->eventTable->item(row,3)->text().toInt(&ok);
    if (col==1) {
        QWidget *w=ui->eventTable->cellWidget(row,col);
        QColorDialog a;
        QColor color=w->palette().background().color();
        a.setCurrentColor(color);
        if (a.exec()==QColorDialog::Accepted) {
            QColor c=a.currentColor();
            QPalette p(c,c,c,c,c,c,c);
            w->setPalette(p);
            m_new_colors[id]=c;
            //qDebug() << "Color accepted" << col << id;
        }
    }
}

void PreferencesDialog::Save()
{
    bool needs_restart=false;

    (*profile)["UnitSystem"]=ui->unitCombo->currentText();
    //(*profile)["TimeZone"]=ui->timeZoneCombo->currentText();

    if (((*profile)["CombineCloserSessions"].toInt()!=ui->combineSlider->value()) ||
        ((*profile)["IgnoreShorterSessions"].toInt()!=ui->IgnoreSlider->value())) {
        needs_restart=true;
    }
    (*profile)["CombineCloserSessions"]=ui->combineSlider->value();
    (*profile)["IgnoreShorterSessions"]=ui->IgnoreSlider->value();

    (*profile)["MemoryHog"]=ui->memoryHogCheckbox->isChecked();

    if ((*profile)["DaySplitTime"].toTime()!=ui->timeEdit->time()) {
        needs_restart=true;
    }

    (*profile)["DaySplitTime"]=ui->timeEdit->time();

    (*profile)["AlwaysShowOverlayBars"]=ui->overlayFlagsCombo->currentIndex();
    (*profile)["UseAntiAliasing"]=ui->useAntiAliasing->isChecked();
    (*profile)["MemoryHog"]=ui->memoryHogCheckbox->isChecked();
    (*profile)["EnableGraphSnapshots"]=ui->useGraphSnapshots->isChecked();
    (*profile)["IntentionalLeak"]=ui->intentionalLeakEdit->value();
    (*profile)["EnableMultithreading"]=ui->useMultithreading->isChecked();
    (*profile)["EnableOximetry"]=ui->oximetryGroupBox->isChecked();
    (*profile)["SyncOximetry"]=ui->oximetrySync->isChecked();
    (*profile)["OximeterType"]=ui->oximetryType->currentText();

    if (ui->squareWavePlots->isChecked() != (*profile)["SquareWavePlots"].toBool()) {
        needs_restart=true;
    }
    (*profile)["SquareWavePlots"]=ui->squareWavePlots->isChecked();

    (*profile)["SkipEmptyDays"]=ui->skipEmptyDays->isChecked();
    PREF["Updates_AutoCheck"]=ui->automaticallyCheckUpdates->isChecked();
    PREF["Updates_CheckFrequency"]=ui->updateCheckEvery->value();

    PREF["Fonts_Application_Name"]=ui->applicationFont->currentText();
    PREF["Fonts_Application_Size"]=ui->applicationFontSize->value();
    PREF["Fonts_Application_Bold"]=ui->applicationFontBold->isChecked();
    PREF["Fonts_Application_Italic"]=ui->applicationFontItalic->isChecked();

    PREF["Fonts_Graph_Name"]=ui->graphFont->currentText();
    PREF["Fonts_Graph_Size"]=ui->graphFontSize->value();
    PREF["Fonts_Graph_Bold"]=ui->graphFontBold->isChecked();
    PREF["Fonts_Graph_Italic"]=ui->graphFontItalic->isChecked();

    PREF["Fonts_Title_Name"]=ui->titleFont->currentText();
    PREF["Fonts_Title_Size"]=ui->titleFontSize->value();
    PREF["Fonts_Title_Bold"]=ui->titleFontBold->isChecked();
    PREF["Fonts_Title_Italic"]=ui->titleFontItalic->isChecked();

    PREF["Fonts_Big_Name"]=ui->bigFont->currentText();
    PREF["Fonts_Big_Size"]=ui->bigFontSize->value();
    PREF["Fonts_Big_Bold"]=ui->bigFontBold->isChecked();
    PREF["Fonts_Big_Italic"]=ui->bigFontItalic->isChecked();

    QFont font=ui->applicationFont->currentFont();
    font.setPointSize(ui->applicationFontSize->value());
    font.setWeight(ui->applicationFontBold->isChecked()?QFont::Bold : QFont::Normal);
    font.setItalic(ui->applicationFontItalic->isChecked());
    QApplication::setFont(font);

    *defaultfont=ui->graphFont->currentFont();
    defaultfont->setPointSize(ui->graphFontSize->value());
    defaultfont->setWeight(ui->graphFontBold->isChecked()?QFont::Bold : QFont::Normal);
    defaultfont->setItalic(ui->graphFontItalic->isChecked());

    *mediumfont=ui->titleFont->currentFont();
    mediumfont->setPointSize(ui->titleFontSize->value());
    mediumfont->setWeight(ui->titleFontBold->isChecked()?QFont::Bold : QFont::Normal);
    mediumfont->setItalic(ui->titleFontItalic->isChecked());

    *bigfont=ui->bigFont->currentFont();
    bigfont->setPointSize(ui->bigFontSize->value());
    bigfont->setWeight(ui->bigFontBold->isChecked()?QFont::Bold : QFont::Normal);
    bigfont->setItalic(ui->bigFontItalic->isChecked());

    for (QHash<int,QColor>::iterator i=m_new_colors.begin();i!=m_new_colors.end();i++) {
        schema::Channel &chan=schema::channel[i.key()];
        if (!chan.isNull()) {
            qDebug() << "TODO: Change" << chan.name() << "color to" << i.value();
            chan.setDefaultColor(i.value());
        }
    }
    qDebug() << "TODO: Save channels.xml to update channel data";

    profile->Save();
    PREF.Save();

    if (needs_restart) {
        if (QMessageBox::question(this,"Restart Required","One or more of the changes you have made will require this application to be restarted, in order for these changes to come into effect.\nWould you like do this now?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            QProcess proc;
            QStringList args;
            QString apppath;
    #ifdef Q_OS_MAC
            // In Mac OS the full path of aplication binary is:
            //    <base-path>/myApp.app/Contents/MacOS/myApp
            //apppath=QApplication::instance()->applicationDirPath()+"/../../../SleepyHead.app";
            apppath=QApplication::instance()->applicationDirPath().section("/",0,-3);
            if (proc.startDetached("open",QStringList() << apppath)) {
                QApplication::instance()->exit();
            } else {
                if (QDesktopServices::openUrl(apppath)) {
                    QApplication::instance()->exit();
                } else QMessageBox::warning(this,"Gah!","If you can read this, two seperate application restart commands didn't work. Mark want's to know the following string:"+apppath,QMessageBox::Ok);
            }
            /*qDebug() << "Hi Jimbo! :)";
            qDebug() << "applicationFilePath:" << QApplication::instance()->applicationFilePath();
            qDebug() << "applicationDirPath:" << QApplication::instance()->applicationDirPath();
            qDebug() << "Chopped String:" << apppath;
            qDebug() << "That last one should end in SleepyHead.app"; */

            //qDebug() << "Would restart on mac if this was correct" << args;
            //qDebug() << "repeating applicationDirPath for clarity: " << QApplication::instance()->applicationDirPath();
    #else
            apppath=QApplication::instance()->applicationFilePath();
            if (QDesktopServices::openUrl(apppath)) {
            //if (proc.startDetached(QApplication::instance()->applicationFilePath(),args)) {
                QApplication::instance()->exit();
            }
    #endif
        }
    }
}

void PreferencesDialog::on_combineSlider_valueChanged(int position)
{
    if (position>0) {
        ui->combineLCD->display(position);
    } else ui->combineLCD->display(tr("OFF"));
}

void PreferencesDialog::on_IgnoreSlider_valueChanged(int position)
{
    if (position>0) {
        ui->IgnoreLCD->display(position);
    } else ui->IgnoreLCD->display(tr("OFF"));
}

void PreferencesDialog::on_useGraphSnapshots_toggled(bool checked)
{
    if (checked
        && !(*profile)["EnableGraphSnapshots"].toBool()
        && QMessageBox::question(this,"Warning","The Graph Snapshots feature (used by the Pie Chart in Daily stats panel) has been known to not work on some older computers.\n\nIf you experience a crash because of it, you will have to remove your SleepApp folder and recreate your profile.\n\n(I'm fairly sure this Qt bug is fixed now, but this has not been tested enough. If you have previously seen the pie chart, it's perfectly ok.)\n\nAre you sure you want to enable it?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::No) {
        ui->useGraphSnapshots->setChecked(false);
    }
}

#include "mainwindow.h"
extern MainWindow * mainwin;
void PreferencesDialog::RefreshLastChecked()
{
    ui->updateLastChecked->setText(PREF["Updates_LastChecked"].toDateTime().toString(Qt::SystemLocaleLongDate));
}

void PreferencesDialog::on_checkForUpdatesButton_clicked()
{
    mainwin->statusBar()->showMessage("Checking for Updates");
    ui->updateLastChecked->setText("Checking for Updates");
    mainwin->CheckForUpdates();
}
