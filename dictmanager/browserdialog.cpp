/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <QMessageBox>
#include <QIcon>
#include <QTextCodec>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEngineView>
#include <QDebug>

#include "guicommon.h"
#include "filedownloader.h"
#include "scelconverter.h"
#include "browserdialog.h"
#include "ui_browserdialog.h"

/*
 * a typical link looks like this.
 * http://download.pinyin.sogou.com/dict/download_cell.php?id=15207&name=%D6%B2%CE%EF%B4%CA%BB%E3%B4%F3%C8%AB%A1%BE%B9%D9%B7%BD%CD%C6%BC%F6%A1%BF
 */

class WebPage : public QWebEnginePage {
public:
    WebPage(BrowserDialog *dialog) : QWebEnginePage(dialog), m_dialog(dialog) {
    }
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override {
        return m_dialog->linkClicked(url);
    }
private:
    BrowserDialog *m_dialog;
};


BrowserDialog::BrowserDialog(QWidget* parent): QDialog(parent)
    ,m_ui(new Ui::BrowserDialog), m_page(new WebPage(this))
{
    m_ui->setupUi(this);
    m_ui->webView->setPage(m_page);
    m_ui->listWidget->hide();
    setWindowIcon(QIcon::fromTheme("internet-web-browser"));
    setWindowTitle(_("Browse Sogou Cell Dict repository"));

    connect(m_ui->webView, &QWebEngineView::loadProgress, m_ui->progressBar, &QProgressBar::setValue);
    connect(m_ui->webView, &QWebEngineView::loadStarted, m_ui->progressBar, &QProgressBar::show);
    connect(m_ui->webView, &QWebEngineView::loadFinished, m_ui->progressBar, &QProgressBar::hide);
    m_ui->webView->load(QUrl(URL_BASE));
}

BrowserDialog::~BrowserDialog()
{
    delete m_ui;
}

QString BrowserDialog::decodeName(const QByteArray& in)
{
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    if (!codec) {
        return QString();
    }
    QByteArray out = QByteArray::fromPercentEncoding(in);
    return codec->toUnicode(out);
}

bool BrowserDialog::linkClicked(const QUrl& url)
{
    do {
        if (url.host() != DOWNLOAD_HOST_BASE) {
            break;
        }
        if (url.path() != "/dict/download_cell.php") {
            break;
        }
        QUrlQuery query(url);
        QString id = query.queryItemValue("id");
        QByteArray name = query.queryItemValue("name", QUrl::FullyEncoded).toLatin1();
        QString sname = decodeName(name);

        m_name = sname;

        if (!id.isEmpty() && !sname.isEmpty()) {
            download(url);
            return false;
        }
    } while(0);

    if (url.host() != HOST_BASE) {
        QMessageBox::information(this, _("Wrong Link"),
                                 _("No browsing outside pinyin.sogou.com, now redirect to home page."));
        m_ui->webView->load(QUrl(URL_BASE));
        return false;
    } else {
        return true;
    }
}


void BrowserDialog::download(const QUrl& url)
{
    m_ui->webView->stop();
    m_ui->webView->hide();
    m_ui->progressBar->hide();
    m_ui->listWidget->show();

    FileDownloader* downloader = new FileDownloader(this);
    connect(downloader, SIGNAL(message(QMessageBox::Icon,QString)), SLOT(showMessage(QMessageBox::Icon,QString)));
    connect(downloader, SIGNAL(finished(bool)), SLOT(downloadFinished(bool)));
    connect(downloader, SIGNAL(finished(bool)), downloader, SLOT(deleteLater()));

    downloader->download(url);
}

void BrowserDialog::showMessage(QMessageBox::Icon msgLevel, const QString& message)
{
    QString iconName;
    switch(msgLevel) {
        case QMessageBox::Warning:
            iconName = "dialog-warning";
            break;
        case QMessageBox::Critical:
            iconName = "dialog-error";
            break;
        case QMessageBox::Information:
            iconName = "dialog-information";
            break;
        default:
            break;
    }
    QListWidgetItem* item = new QListWidgetItem(QIcon::fromTheme(iconName), message, m_ui->listWidget);
    m_ui->listWidget->addItem(item);
}

void BrowserDialog::downloadFinished(bool succ)
{
    FileDownloader* downloader = qobject_cast< FileDownloader* >(sender());
    if (!succ) {
        return;
    }

    QString fileName = downloader->fileName();

    ScelConverter* converter = new ScelConverter(this);
    connect(converter, SIGNAL(message(QMessageBox::Icon,QString)), SLOT(showMessage(QMessageBox::Icon,QString)));
    connect(converter, SIGNAL(finished(bool)), SLOT(convertFinished(bool)));
    connect(converter, SIGNAL(finished(bool)), converter, SLOT(deleteLater()));
    converter->convert(fileName, m_name.append(".txt"));
}

void BrowserDialog::convertFinished(bool succ)
{
    if (succ) {
        accept();
    }
}

