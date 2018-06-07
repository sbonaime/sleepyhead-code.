/* SleepyHead Help Implementation
 *
 * Copyright (c) 2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the source code
 * for more details. */


#include <QtHelp>
#include <QDebug>
#include <QTimer>

#include "help.h"
#include "ui_help.h"

const QString helpNamespace = "jedimark.net.SleepyHead.1.1";

Help::Help(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Help)
{
    ui->setupUi(this);

    helpEngine = new QHelpEngine(QCoreApplication::applicationDirPath() + "/Help/help.qhc", this);
    helpEngine->setupData();

   // QString helpFile = QCoreApplication::applicationDirPath() + "/Help/help_en.qch";

    /*if (!helpEngine->registeredDocumentations().contains(helpFile)) {
        if (helpEngine->registerDocumentation(helpFile)) {
            qDebug() << "Registered" << helpFile;
        } else {
            qDebug() << "Help Error:" << helpEngine->error();
        }
    } */

    QList<QUrl> list = helpEngine->files(helpNamespace, QStringList());

    tabWidget = new QTabWidget;
    tabWidget->setMaximumWidth(250);

    tabWidget->addTab(helpEngine->contentWidget(), tr("Contents"));
    tabWidget->addTab(helpEngine->indexWidget(), tr("Index"));
    tabWidget->addTab(helpEngine->searchEngine()->resultWidget(), tr("Search"));


    helpBrowser = new HelpBrowser(helpEngine);

    if (list.size() == 0) {
        QString msg = tr("<h1>Attention Source Builder</h1><br/><p>No help files detected.. QMake (or you) first need(s) to run <b>qcollectiongenerator</b></p><p>While we are at it, search is broken at the moment, titles are not showing up.</p>");
        helpBrowser->setHtml(msg);
    } else {
        QByteArray index = helpEngine->fileData(QUrl("qthelp://jedimark.net.sleepyhead.1.1/doc/help_en/index.html"));
        helpBrowser->setHtml(index);
    }

    ui->splitter->insertWidget(0, tabWidget);
    ui->splitter->insertWidget(1, helpBrowser);


    QTimer::singleShot(50,this, SLOT(startup()));

    connect(helpEngine->contentWidget(), SIGNAL(linkActivated(QUrl)), helpBrowser, SLOT(setSource(QUrl)));
    connect(helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl, QString)), helpBrowser, SLOT(setSource(QUrl)));
    connect(helpBrowser, SIGNAL(forwardAvailable(bool)), this, SLOT(forwardAvailable(bool)));
    connect(helpBrowser, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
    connect(helpEngine->searchEngine(), SIGNAL(searchingFinished(int)), this, SLOT(on_searchComplete(int)));
    connect(helpEngine->searchEngine(), SIGNAL(indexingFinished()), this, SLOT(indexingFinished()));
    connect(helpEngine->searchEngine()->resultWidget(), SIGNAL(requestShowLink(QUrl)), this, SLOT(requestShowLink(QUrl)));

    ui->forwardButton->setEnabled(false);
    ui->backButton->setEnabled(false);


    searchReady = false;
    helpEngine->searchEngine()->reindexDocumentation();
    helpEngine->setCurrentFilter("SleepyHead 1.1");

}

Help::~Help()
{
    disconnect(helpEngine->searchEngine()->resultWidget(), SIGNAL(requestShowLink(QUrl)), this, SLOT(requestShowLink(QUrl)));
    disconnect(helpEngine->searchEngine(), SIGNAL(indexingFinished()), this, SLOT(indexingFinished()));
    disconnect(helpEngine->searchEngine(), SIGNAL(searchingFinished(int)), this, SLOT(on_searchComplete(int)));

    disconnect(helpEngine->contentWidget(), SIGNAL(linkActivated(QUrl)), helpBrowser, SLOT(setSource(QUrl)));
    disconnect(helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl, QString)), helpBrowser, SLOT(setSource(QUrl)));
    disconnect(helpBrowser, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
    disconnect(helpBrowser, SIGNAL(forwardAvailable(bool)), this, SLOT(forwardAvailable(bool)));

    delete helpEngine;
    delete ui;
}

void Help::startup()
{
    helpEngine->contentWidget()->expandToDepth(0);
}

HelpBrowser::HelpBrowser(QHelpEngine* helpEngine, QWidget* parent):
    QTextBrowser(parent), helpEngine(helpEngine)
{
}

QVariant HelpBrowser::loadResource(int type, const QUrl &name)
{
    QString path = name.path(QUrl::FullyEncoded).replace("/./","/");
    if (!path.startsWith("/doc")) path="/doc"+path;
    QString urlstr = "qthelp://"+helpNamespace+path;
    QByteArray bytes = helpEngine->fileData(urlstr);
    if (bytes.size()>0) return QVariant(bytes);

    if (type == QTextDocument::ImageResource
         && name.scheme().compare(QLatin1String("data"), Qt::CaseInsensitive) == 0) {
       static QRegularExpression re("^image/[^;]+;base64,.+={0,2}$");
       QRegularExpressionMatch match = re.match(name.path());
       if (match.hasMatch())
         return QVariant();
     }
     return QTextBrowser::loadResource(type, name);
}

void Help::on_backButton_clicked()
{
    helpBrowser->backward();
}

void Help::on_forwardButton_clicked()
{
    helpBrowser->forward();
}

void Help::on_homeButton_clicked()
{
    QByteArray index = helpEngine->fileData(QUrl("qthelp://jedimark.net.sleepyhead.1.1/doc/help_en/index.html"));
    helpBrowser->setHtml(index);
}
void Help::on_searchComplete(int)
{
    if (!searchReady) {
        QString html = "<h1>Please wait a bit.. Indexing still in progress</h1>";
        helpBrowser->setText(html);
        return;
    }

    tabWidget->setCurrentWidget(helpEngine->searchEngine()->resultWidget());
    return;
    /*
    QHelpSearchEngine * search = helpEngine->searchEngine();
    QVector<QHelpSearchResult> results = search->searchResults(0, count);

    QString html = "<h1>Search results: ";

    if (results.size() == 0) html += "none";
    html+="</h1><br/>";

    for (QHelpSearchResult & result : results) {
        html += QString("<p><a href=\"%1\"><b>Title: %2</b></a><br/>%3<br/>").arg(result.url().toString()).arg(result.title()).arg(result.snippet());
    }

    helpBrowser->setText(html);
    ui->searchBar->setText(QString()); */
}

void Help::on_searchBar_returnPressed()
{
    QHelpSearchEngine * search = helpEngine->searchEngine();

    QString str=ui->searchBar->text();
    search->search(str);
}

void Help::indexingFinished()
{
    searchReady = true;
}

void Help::forwardAvailable(bool b)
{
    ui->forwardButton->setEnabled(b);
}
void Help::backwardAvailable(bool b)
{
    ui->backButton->setEnabled(b);
}

void Help::requestShowLink(const QUrl & link)
{
    helpBrowser->setSource(link);
}
