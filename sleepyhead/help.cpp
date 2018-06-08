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

#include "SleepLib/common.h"
#include "help.h"
#include "ui_help.h"

const QString helpNamespace = "jedimark.net.SleepyHead.1.1";

Help::Help(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Help)
{
    ui->setupUi(this);

    helpEngine = new QHelpEngine(appResourcePath() + "/Help/help.qhc", this);
    helpEngine->setupData();

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

    resultWidget = new MyTextBrowser(this);
    resultWidget->setOpenLinks(false);
    tabWidget->addTab(resultWidget, tr("Search"));
    resultWidget->setStyleSheet("a:link, a:visited { color: inherit; text-decoration: none; font-weight: normal;}"
                                "a:hover { background-color: inherit; color: #ff8888; text-decoration:underline; font-weight: normal; }");



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

    connect(helpBrowser, SIGNAL(forwardAvailable(bool)), this, SLOT(forwardAvailable(bool)));
    connect(helpBrowser, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
    connect(helpEngine->contentWidget(), SIGNAL(linkActivated(QUrl)), helpBrowser, SLOT(setSource(QUrl)));
    connect(helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl, QString)), helpBrowser, SLOT(setSource(QUrl)));

    connect(helpEngine->searchEngine(), SIGNAL(searchingFinished(int)), this, SLOT(on_searchComplete(int)));
    connect(helpEngine->searchEngine(), SIGNAL(indexingFinished()), this, SLOT(indexingFinished()));

    connect(resultWidget, SIGNAL(anchorClicked(QUrl)), this, SLOT(requestShowLink(QUrl)));

    ui->forwardButton->setEnabled(false);
    ui->backButton->setEnabled(false);


    searchReady = false;
    helpEngine->searchEngine()->reindexDocumentation();
    helpEngine->setCurrentFilter("SleepyHead 1.1");

}

Help::~Help()
{
    disconnect(resultWidget, SIGNAL(anchorClicked(QUrl)), this, SLOT(requestShowLink(QUrl)));
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

    if (type == QTextDocument::ImageResource && name.scheme().compare(QLatin1String(""), Qt::CaseInsensitive) == 0) {
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
void Help::on_searchComplete(int count)
{
    if (!searchReady) {
        QString html = "<h1>Please wait a bit.. Indexing still in progress</h1>";
        helpBrowser->setText(html);
        return;
    }

    QHelpSearchEngine * search = helpEngine->searchEngine();
    QVector<QHelpSearchResult> results = search->searchResults(0, count);

//    ui->searchBar->blockSignals(true);
    ui->searchBar->setText(QString());
//    ui->searchBar->blockSignals(false);

    QString html = "<html><head><style type=\"text/css\">"
                   "a:link, a:visited { color: inherit; text-decoration: none; font-weight: normal;}"
                   "a:hover { background-color: inherit; color: #ff8888; text-decoration: underline; font-weight: bold; }"
                   "</style></head><body>";

    QString queryString;
#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    QList<QHelpSearchQuery> results = search->query();
    if (results.size()>0) {
        queryString=results[0].wordList.at(0);
    }
#else
    queryString = search->searchInput();
#endif
    QString a = (results.size() == 0) ? tr("No") : QString("%1").arg(results.size());
    html+="<font size=+1><b>"+ tr("%1 result(s) for \"%2\"").arg(a).arg(queryString)+"</b></font>";
    if (results.size()>0) html += " [<font size=-1><a href='clear'>"+tr("clear")+"</a></font>]";

    for (QHelpSearchResult & result : results) {
        QString title = result.url().toString().section("/",-1);
        html += QString("<p><a href=\"%1\"><b>%2:</b> %3</a></p>").arg(result.url().toString()).arg(title).arg(result.snippet());
    }

    html += "</body></html>";
    resultWidget->setText(html);
    tabWidget->setCurrentWidget(resultWidget);
}

void Help::on_searchBar_returnPressed()
{
    QHelpSearchEngine * search = helpEngine->searchEngine();

    QString str=ui->searchBar->text();

 #if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    QList<QHelpSearchQuery> query;
    query.push_back(QHelpSearchQuery(QHelpSearchQuery::FUZZY, QStringList(str)));
    search->search(query);
#else
    search->search(str);
#endif
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
    if (link.toString() == "clear") {
        resultWidget->clear();
    } else {
        helpBrowser->setSource(link);
    }
}
